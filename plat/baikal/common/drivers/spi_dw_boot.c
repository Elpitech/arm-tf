/*
 * Designware SPI core controller driver (refer pxa2xx_spi.c)
 *
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <spi_dw.h>
#include <spi_dw_list.h>
#include <spi_dw_reg.h>

#define ADR_MODE_3BYTE 3
#define ADR_MODE_4BYTE 4

static int adr_mode = 0;

// ------------------
// TIMEOUT
// ------------------
#define INIT()  timeout = 1000000
#define TRY()   if(!timeout--) goto exit
static int timeout;

// ------------------
// CS-GPIO
// ------------------
#define GPIO_DATA  *(volatile uint32_t*) ((intptr_t) (GPIO_BASE + 0x00))
#define GPIO_DIR   *(volatile uint32_t*) ((intptr_t) (GPIO_BASE + 0x04))
#define CS_BOOT   20
#define CS_NORMAL 24

static void config_cs (int line)
{
    GPIO_DIR  |= (1<<line);
    GPIO_DATA |= (1<<line);
}

static void set_cs (int line)
{
    GPIO_DATA &= ~(1<<line);
}

static void clear_cs (int line)
{
    GPIO_DATA |= (1<<line);
}


// ------------------
// LOW-LEVEL
// ------------------
static int transfer (int port, int line,
    void *cmd_, uint32_t cmd_len,
    void *tx_ , uint32_t tx_len ,
    void *rx_ , uint32_t rx_len )
{
    int err = -1;
    uint8_t *cmd = cmd_;
    uint8_t *tx = tx_;
    uint8_t *rx = rx_;
    uint8_t *cmdend = (void*) ((intptr_t)cmd + (intptr_t)cmd_len);
    uint8_t *txend  = (void*) ((intptr_t)tx  + (intptr_t)tx_len);
    uint8_t *rxend  = (void*) ((intptr_t)rx  + (intptr_t)rx_len);

    line = (1<<line);

    if(!cmd || !cmd_len){
        return -1;
    }

    config_cs(CS_NORMAL);
    set_cs(CS_NORMAL);

    SPI_SSIENR(port) = SSIENR_SSI_DE;
    SPI_SER(port) = 0;                              /* disable all line's */
    SPI_CTRLR0(port) &= ~SPI_TMOD_MASK;

    if(rx_len) SPI_CTRLR0(port) |= (SPI_TMOD_EPROMREAD << SPI_TMOD_OFFSET);  /* mode: read  */
    else       SPI_CTRLR0(port) |= (SPI_TMOD_TO        << SPI_TMOD_OFFSET);  /* mode: write */

    switch ((SPI_CTRLR0(port) & SPI_TMOD_MASK) >> SPI_TMOD_OFFSET)
    {
        case SPI_TMOD_TO:
            SPI_SSIENR (port) = SSIENR_SSI_EN;      /* ebable fifo */
            INIT();
            while ((cmd != cmdend) && (SPI_SR(port) & SPI_SR_TFNF)){           /* push cmd */
                TRY();
                SPI_DR(port) = *cmd;
                cmd++;
            }

            INIT();
            while (tx != txend && (SPI_SR(port) & SPI_SR_TFNF)){             /* push tx */
                TRY();
                SPI_DR(port) = *tx;
                tx++;
            }

            SPI_SER(port) = line;                   /* start sending */

            INIT();
            while ((tx != txend)){
                TRY();
                if (SPI_SR(port) & SPI_SR_TFNF){
                    SPI_DR(port) = *tx;
                    tx++;
                    SPI_SER(port) = line;           /* restart if dropped */
                }
            }

            /* wait */
            INIT();
            while (!(SPI_SR(port) & SPI_SR_TFE))  TRY();
            while ( (SPI_SR(port) & SPI_SR_BUSY)) TRY();
            break;

        case SPI_TMOD_EPROMREAD:
            if (!rx  || !rx_len || rx_len > SPI_AUTO_READ_SIZE){
                goto exit;
            }

            SPI_CTRLR1 (port) = rx_len - 1;         /* set read size */
            SPI_SSIENR (port) = SSIENR_SSI_EN;      /* ebable fifo */

            INIT();
            while ((cmd != cmdend) && (SPI_SR(port) & SPI_SR_TFNF)) {          /* push cmd */
                TRY();
                SPI_DR(port) = *cmd;
                cmd++;
            }
            SPI_SER(port) = line;                   /* start sending */

            INIT();
            while ((rx != rxend)) {    /* read incoming data */
                TRY();
                if (SPI_SR(port) & SPI_SR_RFNE){
                    *rx = SPI_DR(port);
                    rx++;
                }
            }

            /* wait */
            INIT();
            while (!(SPI_SR(port) & SPI_SR_TFE))  TRY();
            while ( (SPI_SR(port) & SPI_SR_BUSY)) TRY();
            break;

        case SPI_TMOD_TR:
        case SPI_TMOD_RO:
        default:
            goto exit;
    }
    err = 0;

exit:
    clear_cs(CS_NORMAL);
    return err;
}

static int exec (int port, int line,
    uint8_t cmd_op,
    uint32_t address,
    void *buf,
    uint32_t lenbuf)
{
    uint8_t cmd [SPI_CMD_LEN];
    uint8_t *in =0, *out =0;
    uint32_t lencmd =0, lenin =0, lenout =0;

    /* Save the SPI flash instruction. */
    cmd[0] = cmd_op;
    lencmd += 1;

    /* Prepare arguments for the SPI transaction. */
    switch (cmd_op)
    {
        case CMD_FLASH_RDID:    /* Read identification. */
        case CMD_FLASH_RDSR:    /* Read Status Register */
        case CMD_FLASH_RDEAR:   /* Read Extended Address Register */
        case CMD_FLASH_RFSR:    /* Read Flag Status Register */
            out = buf;
            lenout = lenbuf;
            break;

        case CMD_FLASH_READ:    /* Read Data Bytes */
            out = buf;
            lenout = lenbuf;
        case CMD_FLASH_SSE:     /* SubSector Erase */
        case CMD_FLASH_SE:      /* Sector Erase */
            if (adr_mode == ADR_MODE_4BYTE) {
              SPI_SET_ADDRESS_4BYTE(address, cmd);
              lencmd += SPI_ADR_LEN_4BYTE;
            } else if (adr_mode == ADR_MODE_3BYTE) {
              SPI_SET_ADDRESS_3BYTE(address, cmd);
              lencmd += SPI_ADR_LEN_3BYTE;
            } else {
              return -1;
            }
            break;

        case CMD_FLASH_WREAR:
        case CMD_FLASH_WRSR:    /* Write Status Register */
            in = buf;
            lenin = 1;
            break;

        case CMD_FLASH_EN4BYTEADDR:
        case CMD_FLASH_EX4BYTEADDR:
        case CMD_FLASH_WRDI:    /* Write Disable */
        case CMD_FLASH_WREN:    /* Write Enable */
        case CMD_FLASH_BE:      /* Bulk Erase */
            break;

        case CMD_FLASH_PP:      /* Page Program */
            if(lenbuf > SPI_PAGE_SIZE){
                return -1;
            }
            in = buf;
            lenin = lenbuf;
            if(adr_mode == ADR_MODE_4BYTE){
              SPI_SET_ADDRESS_4BYTE(address, cmd);
              lencmd += SPI_ADR_LEN_4BYTE;
            } else if(adr_mode == ADR_MODE_3BYTE) {
              SPI_SET_ADDRESS_3BYTE(address, cmd);
              lencmd += SPI_ADR_LEN_3BYTE;
            } else {
              return -1;
            }
            break;

        default:
            return -1;
    }

    /* Execute the SPI transaction */
    return transfer(port, line,
        cmd, lencmd,
        in,  lenin,
        out, lenout
    );
}


// ------------------
// SUPPORT
// ------------------
static int status (int port, int line, void *status_)
{
    return exec (port, line, CMD_FLASH_RDSR, 0, status_, 1);
}

static int flag_status (int port, int line, void *status_)
{
    return exec (port, line, CMD_FLASH_RFSR, 0, status_, 1);
}

static int get_mode (int port, int line)
{
    int flag;
    if (flag_status(port,line,&flag))
        return 0;

    return (flag & SPI_FLAG_4BYTE)? ADR_MODE_4BYTE: ADR_MODE_3BYTE;
}

static int wren (int port, int line)
{
    int err;
    err = exec (port, line, CMD_FLASH_WREN, 0, 0, 0);
    if(err){
        return err;
    }

    uint8_t st;
    err = status(port, line, &st);
    if(err){
        return err;
    }
    return (st & SPI_FLASH_SR_WEL)? 0:1;
}

static int wait (int port, int line)
{
    uint8_t st;
    int err = -1;

    INIT();
    do {
        TRY();
        if(status(port, line, &st))
            goto exit;
    } while (st & SPI_FLASH_SR_WIP);
    err = 0;
exit:
    return err;
}





// ------------------
// EXTERNAL
// ------------------
int  dw_spi_erase   (int port, int line, uint32_t adr, size_t size, size_t sector_size)
{
    int err;

    if (!sector_size) {
        return -1;
    }

    if (!adr_mode) {
        adr_mode = get_mode(port,line);
    }

    while (size) {

        err = wren(port,line);
        if(err){
            return err;
        }

        err = exec(port,line,CMD_FLASH_SE,adr,0,0);
        if(err){
            return err;
        }

        err = wait(port,line);
        if(err){
            return err;
        }

        adr  += sector_size;
        size -= (size > sector_size)? sector_size : size;
    }
    return 0;
}

int  dw_spi_write   (int port, int line, uint32_t adr, void *data, size_t size)
{
    int err;
    int part;
    char *pdata = data;

    if (!adr_mode) {
        adr_mode = get_mode(port,line);
    }

    while (size) {
        part = (size > SPI_PAGE_SIZE)? SPI_PAGE_SIZE : size;

        /* fix size */
        int p1 = adr/SPI_PAGE_SIZE;            /* page number */
        int p2 = (adr+part)/SPI_PAGE_SIZE;
        if (p1 != p2){                         /* page overflow ? */
            p2 *= SPI_PAGE_SIZE;               /* page base address */
            part = p2 - adr;
        }

        err = wren(port, line);
        if(err){
            return err;
        }

        err = exec(port, line, CMD_FLASH_PP, adr, pdata, part);
        if(err){
            return err;
        }

        err = wait(port, line);
        if(err){
            return err;
        }

        adr   += part;
        pdata += part;
        size  -= part;
    }

    return 0;
}


int  dw_spi_read    (int port, int line, uint32_t adr, void *data, size_t size)
{
    int err;
    int part;
    char *pdata = data;

    if (!adr_mode) {
        adr_mode = get_mode(port,line);
    }

    while (size) {

        part = (size > SPI_MAX_READ)? SPI_MAX_READ : size;

        err = exec(port, line, CMD_FLASH_READ, adr, pdata, part);
        if(err){
            return err;
        }

        adr   += part;
        pdata += part;
        size  -= part;
    }
    return 0;
}



int dw_spi_3byte (int port, int line)
{
    int err = wren(port,line);
    if(err) {
        return err;
    }
    err = exec(port, line, CMD_FLASH_EX4BYTEADDR, 0, 0, 0);
    if(err){
        return err;
    }
    return 0;
}


int dw_spi_4byte (int port, int line)
{
    int err = wren(port,line);
    if(err){
        return err;
    }
    err = exec(port, line, CMD_FLASH_EN4BYTEADDR, 0, 0, 0);
    if(err){
        return err;
    }
    return 0;
}


static void dw_spi_init_regs (int port)
{
    /* disable device */
    SPI_SSIENR (port) = SSIENR_SSI_DE;

    /* crt0 */
    SPI_CTRLR0(port) = 0;
    SPI_CTRLR0(port) |= (SPI_DFS(8) << SPI_DFS_OFFSET);

    /* other */
    SPI_CTRLR1(port) = 0;
    SPI_BAUDR(port) = SPI_BAUDR_DEFAULT;
    SPI_TXFTLR(port) = 0;
    SPI_RXFTLR(port) = 0;
    SPI_IMR(port) = 0;
    SPI_SER(port) = 0;
}


static int compare (const uint8_t *s1, const uint8_t *s2, uint32_t count)
{
    while (count-- > 0) {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}


const struct flash_info *dw_get_info (int port, int line)
{
    const struct flash_info *info;
    uint8_t id [SPI_NOR_MAX_ID_LEN];

    // read
    if (exec (port, line, CMD_FLASH_RDID, 0, id, SPI_NOR_MAX_ID_LEN)){
        ERROR("error while reading JEDEC ID\n");
        return NULL;
    }

    // find
    int k;
    for (k = 0; k < COUNTOF(spi_nor_ids) - 1; k++){
        info = &spi_nor_ids[k];
        if (!compare(info->id, id, SPI_NOR_MAX_ID_LEN)){
            INFO("SPI Flash: sector size %u KiB, sector count %u, total size %u MiB\n",
		 info->sector_size / 1024, info->n_sectors,
		 ((info->sector_size / 1024) * info->n_sectors) / 1024);

            return info;
        }
    }
    ERROR("unrecognized JEDEC id bytes: %02x%02x%02x\n", id[0], id[1], id[2]);
    return NULL;
}


void dw_spi_init (int port, int line)
{
    dw_spi_init_regs(port);

    const struct flash_info *info;
    info = dw_get_info(port,line);
    if(!info)
        return;

    uint64_t total = info->sector_size * info->n_sectors;
    if (total > 16*1024*1024)
        dw_spi_4byte(port,line);
    else
        dw_spi_3byte(port,line);

}

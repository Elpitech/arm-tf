#ifndef __SPI_DW_REG_H__
#define __SPI_DW_REG_H__

#include <lib/mmio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Registers */
#define SPI_PORT(p)              (SPI_BASE + (p)*SPI_OFFSET)
#define SPI_CTRLR0(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x00))   /* Control Register 0  */
#define SPI_CTRLR1(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x04))   /* Control Register 1  */
#define SPI_SSIENR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x08))   /* SSI Enable Register */
#define SPI_MWCR(p)             *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x0c))   /* Microwire Control Register.  */
#define SPI_SER(p)              *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x10))   /* Slave Enable Register.  */
#define SPI_BAUDR(p)            *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x14))   /* Baud Rate Select.  */
#define SPI_TXFTLR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x18))   /* Transmit FIFO Threshold Level.  */
#define SPI_RXFTLR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x1c))   /* Receive FIFO Threshold level.  */
#define SPI_TXFLR(p)            *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x20))   /* Transmit FIFO Level Register */
#define SPI_RXFLR(p)            *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x24))   /* Receive FIFO Level Register */
#define SPI_SR(p)               *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x28))   /* Status Register */
#define SPI_IMR(p)              *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x2c))   /* Interrupt Mask Register */
#define SPI_IS(p)               *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x30))   /* Interrupt Status Register */
#define SPI_RISR(p)             *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x34))   /* Raw Interrupt StatusRegister */
#define SPI_TXOICR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x38))   /* Transmit FIFO Overflow Interrupt Clear Register */
#define SPI_RXOICR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x3c))   /* Receive FIFO Overflow Interrupt Clear Register */
#define SPI_RXUICR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x40))   /* Receive FIFO Underflow Interrupt Clear Register */
#define SPI_MSTICR(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x44))   /* Multi-Master Interrupt Clear Register */
#define SPI_ICR(p)              *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x48))   /* Interrupt Clear Register */
#define SPI_DMACR(p)            *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x4c))   /* DMA Control Register.  */
#define SPI_DMATDLR(p)          *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x50))   /* DMA Transmit Data Level.  */
#define SPI_DMARDLR(p)          *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x54))   /* DMA Receive Data Level.  */
#define SPI_IDR(p)              *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x58))   /* Identification Register.  */
#define SPI_SSI_VERSION_ID(p)   *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x5c))   /* coreKit Version ID Register */
#define SPI_DR(p)               *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0x60))   /* A 16-bit read/write buffer for the transmit/receive FIFOs. */
#define SPI_DR35(p)             *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0xec))   /* A 16-bit read/write buffer for the transmit/receive FIFOs. */
#define SPI_RX_SAMPLE_DLY(p)    *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0xf0))   /* RX Sample Delay. */
#define SPI_RSVD_0(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0xf4))   /* RSVD_0 - Reserved address location */
#define SPI_RSVD_1(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0xf8))   /* RSVD_1 - Reserved address location */
#define SPI_RSVD_2(p)           *(volatile uint32_t*) ((intptr_t) (SPI_PORT(p) + 0xfc))   /* RSVD_2 - Reserved address location */


/* ------- */
/* CTRL 0  */
/* ------- */

/* Slave output enable. */
#define SPI_SLV_OE_OFFSET       10
#define SPI_SLV_MASK            (0x1 << SPI_SLV_OE_OFFSET)
#define SPI_SLV_OE              1

/* Transfer mode bits */
#define SPI_TMOD_OFFSET         8
#define SPI_TMOD_MASK           (0x3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TR             0x0
#define SPI_TMOD_TO             0x1
#define SPI_TMOD_RO             0x2
#define SPI_TMOD_EPROMREAD      0x3

/* Serial Clock Polarity. */
#define SPI_SCPOL_OFFSET        7
#define SPI_SCPOL_MASK          (0x3 << SPI_SCPOL_OFFSET)
#define SPI_SCPOL_LOW           0
#define SPI_SCPOL_HIGH          1

/* Serial Clock Phase. */
#define SPI_SCPH_OFFSET         6
#define SPI_SCPH_MASK           (0x3 << SPI_SCPH_OFFSET)
#define SPI_SCPH_MIDDLE         0
#define SPI_SCPH_START          1

/* Frame format. */
#define SPI_FRF_OFFSET          4
#define SPI_FRF_MASK            (0x3 << SPI_FRF_OFFSET)
#define SPI_FRF_SPI             0x0
#define SPI_FRF_SSP             0x1
#define SPI_FRF_MICROWIRE       0x2
#define SPI_FRF_RESV            0x3

/* Data Frame Size */
#define SPI_DFS_OFFSET          0
#define SPI_DFS_MASK            (0x3 << SPI_DFS_OFFSET)
#define SPI_DFS(x)              (x - 1)

/* ------- */
/* STATUS  */
/* ------- */
#define SPI_SR_DCOL             (1 << 6)    /* Data Collision Error */
#define SPI_SR_TXE              (1 << 5)    /* Transmition Error. */
#define SPI_SR_RFF              (1 << 4)    /* Receive FIFO Full */
#define SPI_SR_RFNE             (1 << 3)    /* Receive FIFO Not Empty */
#define SPI_SR_TFE              (1 << 2)    /* Transmit FIFO Empty */
#define SPI_SR_TFNF             (1 << 1)    /* Transmit FIFO Not Full */
#define SPI_SR_BUSY             (1 << 0)    /* SSI Busy Flag. */

/* Define the SPI bits of the status register. */
#define SPI_FLASH_SR_WIP        (1 << 0)    /* Write In Progress */
#define SPI_FLASH_SR_WEL        (1 << 1)    /* Write Enable Latch */

/* Table 7: Flag Status Register */
#define SPI_FLAG_PE             (1 << 7)    /* P/E Controller (not WIP) */
#define SPI_FLAG_ER_SUSPEND     (1 << 6)    /* Erase Suspend */
#define SPI_FLAG_ERASE          (1 << 5)    /* Erase */
#define SPI_FLAG_PROGRAM        (1 << 4)    /* Program */
#define SPI_FLAG_VPP            (1 << 3)    /* VPP */
#define SPI_FLAG_PR_SUSPEND     (1 << 2)    /* Program Suspend */
#define SPI_FLAG_PROTECTION     (1 << 1)    /* Protection */
#define SPI_FLAG_4BYTE          (1 << 0)    /* 4-byte Address Enabling */

/* ISR, IMR, RISR */
#define SPI_INT_TXEI            (1 << 0)    /* empty */
#define SPI_INT_TXOI            (1 << 1)    /* overflow */
#define SPI_INT_RXUI            (1 << 2)    /* underflow */
#define SPI_INT_RXOI            (1 << 3)    /* overflow */
#define SPI_INT_RXFI            (1 << 4)    /* full */
#define SPI_INT_MSTI            (1 << 5)    /* multi-master */

/* ------- */
/* SSIENR  */
/* ------- */
#define SSIENR_SSI_DE 0
#define SSIENR_SSI_EN 1


/* -------- */
/* COMMANDS */
/* -------- */
/* RDNVCR Read NV Configuration Register B5h (2) */
/* WRNVCR Write NV Configuration Register B1h (2) */
/* RDVCR Read Volatile Configuration Register 85h (1 to ∞) */
/* WRVCR Write Volatile Configuration Register 81h (1) */
/* RDVECR Read Volatile Enhanced Configuration Register 65h (1 to ∞) */
/* WRVECR Write Volatile Enhanced Configuration Register 61h (1) */
#define CMD_FLASH_RDID          0x9F /* (0, 1-20) Read identification. */
#define CMD_FLASH_READ          0x03 /* (3, 1-∞ ) Read Data Bytes */
#define CMD_FLASH_WREN          0x06 /* (0, 0   ) Write Enable */
#define CMD_FLASH_WRDI          0x04 /* (0, 0   ) Write Disable */
#define CMD_FLASH_PP            0x02 /* (3, 256 ) Page Program */
#define CMD_FLASH_SSE           0x20 /* (3, 0   ) SubSector Erase */
#define CMD_FLASH_SE            0xD8 /* (3, 0   ) Sector Erase */
#define CMD_FLASH_RDSR          0x05 /* (0, 1   ) Read Status Register */
#define CMD_FLASH_WRSR          0x01 /* (0, 1-∞ ) Write Status Register */
#define CMD_FLASH_RDLR          0xE8 /* (3, 1-∞ ) Read Lock Register */
#define CMD_FLASH_WRLR          0xE5 /* (3, 1   ) Write Lock Register */
#define CMD_FLASH_RFSR          0x70 /* (1 to ∞)  Read Flag Status Register */
#define CMD_FLASH_CLFSR         0x50 /* (0) Clear Flag Status Register */
#define CMD_FLASH_BE            0xC7 /* (0) Bulk Erase */
#define CMD_FLASH_RSTEN         0x66  /* Reset Enable */
#define CMD_FLASH_RST           0x99  /* Reset Memory */
#define CMD_FLASH_EN4BYTEADDR   0xB7  /* Enter 4-byte address mode */
#define CMD_FLASH_EX4BYTEADDR   0xE9  /* Exit 4-byte address mode */
#define CMD_FLASH_WREAR         0xC5  /* Write Extended Address Register */
#define CMD_FLASH_RDEAR         0xC8  /* Read Extended Address Register */

/* -------- */
/* const    */
/* -------- */
#define SPI_PAGE_SIZE           256      /* (3, 256 ) Page Program */
#define SPI_MAX_READ            1024    //0x10000   // 64
#define SPI_FIFO_SIZE           64
#define SPI_BAUDR_DEFAULT       32  //20+ //0x1000    // 6-
#define SPI_BAUDR_MAX           0xFFFE
#define SPI_ID_SIZE             20
#define SPI_AUTO_READ_SIZE      0x10000


#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))
#define DIVIDE(x,n) (((x) + (n) - 1) / (n))   /* round up */


/* ----------- */
/* put address */
/* ----------- */
#define SPI_ADR_LEN_4BYTE 4
#define SPI_SET_ADDRESS_4BYTE(a,b)      \
    ({                                  \
        uint8_t* _b = (void*)(b);       \
        _b[1] = (((a) >> 8*3) & 0xFF);  \
        _b[2] = (((a) >> 8*2) & 0xFF);  \
        _b[3] = (((a) >> 8*1) & 0xFF);  \
        _b[4] = (((a) >> 8*0) & 0xFF);  \
    })
#define SPI_ADR_LEN_3BYTE 3
#define SPI_SET_ADDRESS_3BYTE(a,b)      \
    ({                                  \
        uint8_t* _b = (void*)(b);       \
        _b[1] = (((a) >> 8*2) & 0xFF);  \
        _b[2] = (((a) >> 8*1) & 0xFF);  \
        _b[3] = (((a) >> 8*0) & 0xFF);  \
    })
#define SPI_CMD_LEN (1+SPI_ADR_LEN_4BYTE)


#endif /* __SPI_DW_REG_H__ */

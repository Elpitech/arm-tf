/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPI_FLASH_IDS_H
#define SPI_FLASH_IDS_H

#define FLASH_INFO(jedec, log2size, log2count)	\
	.id[0] = ((jedec) >> 16) & 0xff,	\
	.id[1] = ((jedec) >>  8) & 0xff,	\
	.id[2] = ((jedec) >>  0) & 0xff,	\
	.sector_log2size  = (log2size),		\
	.sector_log2count = (log2count)

const struct flash_info spi_flash_ids[] = {
	{FLASH_INFO(0x010212, 16,  3)}, /* S25SL004A    */
	{FLASH_INFO(0x010213, 16,  4)}, /* S25SL008A    */
	{FLASH_INFO(0x010214, 16,  5)}, /* S25SL016A    */
	{FLASH_INFO(0x010215, 16,  6)}, /* S25FL032P    */
	{FLASH_INFO(0x010216, 16,  7)}, /* S25FL064P    */
	{FLASH_INFO(0x010220, 18,  8)}, /* S25FL512S    */
	{FLASH_INFO(0x014013, 16,  3)}, /* S25FL204K    */
	{FLASH_INFO(0x014014, 16,  4)}, /* S25FL208K    */
	{FLASH_INFO(0x014015, 16,  5)}, /* S25FL116K    */
	{FLASH_INFO(0x014016, 16,  6)}, /* S25FL132K    */
	{FLASH_INFO(0x014017, 16,  7)}, /* S25FL164K    */
	{FLASH_INFO(0x016017, 16,  7)}, /* S25FL064L    */
	{FLASH_INFO(0x016018, 16,  8)}, /* S25FL128L    */
	{FLASH_INFO(0x016019, 16,  9)}, /* S25FL256L    */
	{FLASH_INFO(0x047f27, 17,  0)}, /* MB85RS1MT    */
	{FLASH_INFO(0x1c2016, 16,  6)}, /* EN25P32      */
	{FLASH_INFO(0x1c2017, 16,  7)}, /* EN25P64      */
	{FLASH_INFO(0x1c3014, 16,  4)}, /* EN25Q80A     */
	{FLASH_INFO(0x1c3016, 16,  6)}, /* EN25Q32B     */
	{FLASH_INFO(0x1c3017, 16,  7)}, /* EN25Q64      */
	{FLASH_INFO(0x1c3116, 16,  6)}, /* EN25F32      */
	{FLASH_INFO(0x1c3817, 16,  7)}, /* EN25S64      */
	{FLASH_INFO(0x1c7015, 16,  5)}, /* EN25QH16     */
	{FLASH_INFO(0x1c7016, 16,  6)}, /* EN25QH32     */
	{FLASH_INFO(0x1c7017, 16,  7)}, /* EN25QH64     */
	{FLASH_INFO(0x1c7018, 16,  8)}, /* EN25QH128    */
	{FLASH_INFO(0x1c7019, 16,  9)}, /* EN25QH256    */
	{FLASH_INFO(0x1f0400, 16,  3)}, /* AT26F004     */
	{FLASH_INFO(0x1f2500, 16,  4)}, /* AT45DB081D   */
	{FLASH_INFO(0x1f4216, 16,  6)}, /* AT25SL321    */
	{FLASH_INFO(0x1f4401, 16,  3)}, /* AT25DF041A   */
	{FLASH_INFO(0x1f4501, 16,  4)}, /* AT26DF081A   */
	{FLASH_INFO(0x1f4601, 16,  5)}, /* AT26DF161A   */
	{FLASH_INFO(0x1f4700, 16,  6)}, /* AT26DF321    */
	{FLASH_INFO(0x1f4701, 16,  6)}, /* AT25DF321A   */
	{FLASH_INFO(0x1f4800, 16,  7)}, /* AT25DF641    */
	{FLASH_INFO(0x1f6601, 15,  2)}, /* AT25FS010    */
	{FLASH_INFO(0x1f6604, 16,  3)}, /* AT25FS040    */
	{FLASH_INFO(0x202010, 15,  1)}, /* M25P05       */
	{FLASH_INFO(0x202011, 15,  2)}, /* M25P10       */
	{FLASH_INFO(0x202012, 16,  2)}, /* M25P20       */
	{FLASH_INFO(0x202013, 16,  3)}, /* M25P40       */
	{FLASH_INFO(0x202014, 16,  4)}, /* M25P80       */
	{FLASH_INFO(0x202015, 16,  5)}, /* M25P16       */
	{FLASH_INFO(0x202016, 16,  6)}, /* M25P32       */
	{FLASH_INFO(0x202017, 16,  7)}, /* M25P64       */
	{FLASH_INFO(0x202018, 18,  6)}, /* M25P128      */
	{FLASH_INFO(0x204011, 16,  1)}, /* M45PE10      */
	{FLASH_INFO(0x204014, 16,  4)}, /* M45PE80      */
	{FLASH_INFO(0x204015, 16,  5)}, /* M45PE16      */
	{FLASH_INFO(0x206316, 16,  6)}, /* M25PX32-S1   */
	{FLASH_INFO(0x207114, 16,  4)}, /* M25PX80      */
	{FLASH_INFO(0x207115, 16,  5)}, /* M25PX16      */
	{FLASH_INFO(0x207116, 16,  6)}, /* M25PX32      */
	{FLASH_INFO(0x207117, 16,  7)}, /* M25PX64      */
	{FLASH_INFO(0x207316, 16,  6)}, /* M25PX32-S0   */
	{FLASH_INFO(0x208012, 16,  2)}, /* M25PE20      */
	{FLASH_INFO(0x208014, 16,  4)}, /* M25PE80      */
	{FLASH_INFO(0x208015, 16,  5)}, /* M25PE16      */
	{FLASH_INFO(0x20ba16, 16,  6)}, /* N25Q032      */
	{FLASH_INFO(0x20ba17, 16,  7)}, /* N25Q064      */
	{FLASH_INFO(0x20ba18, 16,  8)}, /* N25Q128A13   */
	{FLASH_INFO(0x20ba19, 16,  9)}, /* N25Q256A     */
	{FLASH_INFO(0x20ba20, 16, 10)}, /* N25Q512AX3   */
	{FLASH_INFO(0x20ba21, 16, 11)}, /* N25Q00A      */
	{FLASH_INFO(0x20ba22, 16, 12)}, /* MT25QL02G    */
	{FLASH_INFO(0x20bb15, 16,  5)}, /* N25Q016A     */
	{FLASH_INFO(0x20bb16, 16,  6)}, /* N25Q032A     */
	{FLASH_INFO(0x20bb17, 16,  7)}, /* N25Q064A     */
	{FLASH_INFO(0x20bb18, 16,  8)}, /* N25Q128A11   */
	{FLASH_INFO(0x20bb19, 16,  9)}, /* N25Q256AX1   */
	{FLASH_INFO(0x20bb20, 16, 10)}, /* N25Q512A     */
	{FLASH_INFO(0x20bb21, 16, 11)}, /* N25Q00A      */
	{FLASH_INFO(0x20bb22, 16, 12)}, /* MT25QU02G    */
	{FLASH_INFO(0x2c5b1a, 17,  9)}, /* MT35XU512ABA */
	{FLASH_INFO(0x2c5b1c, 17, 11)}, /* MT35XU02G    */
	{FLASH_INFO(0x621612, 16,  2)}, /* SST25WF020A  */
	{FLASH_INFO(0x621613, 16,  3)}, /* SST25WF040B  */
	{FLASH_INFO(0x7f9d20, 15,  1)}, /* IS25CD512    */
	{FLASH_INFO(0x7f9d46, 16,  6)}, /* PM25LQ032    */
	{FLASH_INFO(0x898911, 16,  5)}, /* 160S33B      */
	{FLASH_INFO(0x898912, 16,  6)}, /* 320S33B      */
	{FLASH_INFO(0x898913, 16,  7)}, /* 640S33B      */
	{FLASH_INFO(0x8c2016, 16,  6)}, /* F25L32PA     */
	{FLASH_INFO(0x8c4116, 16,  6)}, /* F25L32QA     */
	{FLASH_INFO(0x8c4117, 16,  7)}, /* F25L64QA     */
	{FLASH_INFO(0x9d4013, 16,  3)}, /* IS25LQ040B   */
	{FLASH_INFO(0x9d6014, 16,  4)}, /* IS25LP080D   */
	{FLASH_INFO(0x9d6015, 16,  5)}, /* IS25LP016D   */
	{FLASH_INFO(0x9d6016, 16,  6)}, /* IS25LP032    */
	{FLASH_INFO(0x9d6017, 16,  7)}, /* IS25LP064    */
	{FLASH_INFO(0x9d6018, 16,  8)}, /* IS25LP128    */
	{FLASH_INFO(0x9d6019, 16,  9)}, /* IS25LP256    */
	{FLASH_INFO(0x9d7016, 16,  6)}, /* IS25WP032    */
	{FLASH_INFO(0x9d7017, 16,  7)}, /* IS25WP064    */
	{FLASH_INFO(0x9d7018, 16,  8)}, /* IS25WP128    */
	{FLASH_INFO(0x9d7019, 16,  9)}, /* IS25WP256    */
	{FLASH_INFO(0xbf2501, 16,  0)}, /* SST25WF512   */
	{FLASH_INFO(0xbf2502, 16,  1)}, /* SST25WF010   */
	{FLASH_INFO(0xbf2503, 16,  2)}, /* SST25WF020   */
	{FLASH_INFO(0xbf2504, 16,  3)}, /* SST25WF040   */
	{FLASH_INFO(0xbf2505, 16,  4)}, /* SST25WF080   */
	{FLASH_INFO(0xbf2541, 16,  5)}, /* SST25VF016B  */
	{FLASH_INFO(0xbf254a, 16,  6)}, /* SST25VF032B  */
	{FLASH_INFO(0xbf254b, 16,  7)}, /* SST25VF064C  */
	{FLASH_INFO(0xbf258d, 16,  3)}, /* SST25VF040B  */
	{FLASH_INFO(0xbf258e, 16,  4)}, /* SST25VG080B  */
	{FLASH_INFO(0xbf2641, 16,  5)}, /* SST26VF016B  */
	{FLASH_INFO(0xbf2643, 16,  7)}, /* SST26VF064B  */
	{FLASH_INFO(0xbf2651, 16,  5)}, /* SST26WF016B  */
	{FLASH_INFO(0xc22010, 16,  0)}, /* MX25L512E    */
	{FLASH_INFO(0xc22012, 16,  2)}, /* MX25L2005A   */
	{FLASH_INFO(0xc22013, 16,  3)}, /* MX25L4005A   */
	{FLASH_INFO(0xc22014, 16,  4)}, /* MX25L8005    */
	{FLASH_INFO(0xc22015, 16,  5)}, /* MX25L1606E   */
	{FLASH_INFO(0xc22016, 16,  6)}, /* MX25L3205D   */
	{FLASH_INFO(0xc22017, 16,  7)}, /* MX25L6405D   */
	{FLASH_INFO(0xc22018, 16,  8)}, /* MX25L12805D  */
	{FLASH_INFO(0xc22019, 16,  9)}, /* MX25L25635E  */
	{FLASH_INFO(0xc2201a, 16, 10)}, /* MX66L51235L  */
	{FLASH_INFO(0xc2201b, 16, 11)}, /* MX66L1G45G   */
	{FLASH_INFO(0xc22314, 16,  4)}, /* MX25V8045F   */
	{FLASH_INFO(0xc22532, 16,  2)}, /* MX25U2033E   */
	{FLASH_INFO(0xc22533, 16,  3)}, /* MX25U4035    */
	{FLASH_INFO(0xc22534, 16,  4)}, /* MX25U8035    */
	{FLASH_INFO(0xc22536, 16,  6)}, /* MX25U3235F   */
	{FLASH_INFO(0xc22537, 16,  7)}, /* MX25U6435F   */
	{FLASH_INFO(0xc22538, 16,  8)}, /* MX25U12835F  */
	{FLASH_INFO(0xc22539, 16,  9)}, /* MX25U25645G  */
	{FLASH_INFO(0xc2253a, 16, 10)}, /* MX25U51245G  */
	{FLASH_INFO(0xc2253c, 16, 12)}, /* MX66U2G45G   */
	{FLASH_INFO(0xc22618, 16,  8)}, /* MX25L12855E  */
	{FLASH_INFO(0xc22619, 16,  9)}, /* MX25L25655E  */
	{FLASH_INFO(0xc2261b, 16, 11)}, /* MX66L1G55G   */
	{FLASH_INFO(0xc22815, 16,  5)}, /* MX25R1635F   */
	{FLASH_INFO(0xc22816, 16,  6)}, /* MX25R3235F   */
	{FLASH_INFO(0xc29e16, 16,  6)}, /* MX25L3255E   */
	{FLASH_INFO(0xc84015, 16,  5)}, /* GD25Q16      */
	{FLASH_INFO(0xc84016, 16,  6)}, /* GD25Q32      */
	{FLASH_INFO(0xc84017, 16,  7)}, /* GD25Q64      */
	{FLASH_INFO(0xc84018, 16,  8)}, /* GD25Q128     */
	{FLASH_INFO(0xc84019, 16,  9)}, /* GD25Q256     */
	{FLASH_INFO(0xc86016, 16,  6)}, /* GD25LQ32C    */
	{FLASH_INFO(0xc86017, 16,  7)}, /* GD25LQ64C    */
	{FLASH_INFO(0xc86018, 16,  8)}, /* GD25LQ128    */
	{FLASH_INFO(0xef3010, 16,  0)}, /* W25X05       */
	{FLASH_INFO(0xef3011, 16,  1)}, /* W25X10       */
	{FLASH_INFO(0xef3012, 16,  2)}, /* W25X20       */
	{FLASH_INFO(0xef3013, 16,  3)}, /* W25X40       */
	{FLASH_INFO(0xef3014, 16,  4)}, /* W25X80       */
	{FLASH_INFO(0xef3015, 16,  5)}, /* W25X16       */
	{FLASH_INFO(0xef3016, 16,  6)}, /* W25X32       */
	{FLASH_INFO(0xef3017, 16,  7)}, /* W25X64       */
	{FLASH_INFO(0xef4012, 16,  2)}, /* W25Q20CL     */
	{FLASH_INFO(0xef4013, 16,  3)}, /* S25FL004K    */
	{FLASH_INFO(0xef4014, 16,  4)}, /* S25FL008K    */
	{FLASH_INFO(0xef4015, 16,  5)}, /* S25FL016K    */
	{FLASH_INFO(0xef4016, 16,  6)}, /* W25Q32       */
	{FLASH_INFO(0xef4017, 16,  7)}, /* W25Q64       */
	{FLASH_INFO(0xef4018, 16,  8)}, /* W25Q128      */
	{FLASH_INFO(0xef4019, 16,  9)}, /* W25Q256      */
	{FLASH_INFO(0xef5012, 16,  2)}, /* W25Q20BW     */
	{FLASH_INFO(0xef5014, 16,  4)}, /* W25Q80BW     */
	{FLASH_INFO(0xef6012, 16,  2)}, /* W25Q20EW     */
	{FLASH_INFO(0xef6015, 16,  5)}, /* W25Q16DW     */
	{FLASH_INFO(0xef6016, 16,  6)}, /* W25Q32DW     */
	{FLASH_INFO(0xef6017, 16,  7)}, /* W25Q64DW     */
	{FLASH_INFO(0xef6018, 16,  8)}, /* W25Q128FW    */
	{FLASH_INFO(0xef6019, 16,  9)}, /* W25Q256FW    */
	{FLASH_INFO(0xef7015, 16,  5)}, /* W25Q16JV     */
	{FLASH_INFO(0xef7016, 16,  6)}, /* W25Q32JV     */
	{FLASH_INFO(0xef7017, 16,  7)}, /* W25Q64JV     */
	{FLASH_INFO(0xef7018, 16,  8)}, /* W25Q128JV    */
	{FLASH_INFO(0xef7019, 16,  9)}, /* W25Q256JVM   */
	{FLASH_INFO(0xef7119, 16, 10)}, /* W25M512JV    */
	{FLASH_INFO(0xef8016, 16,  6)}  /* W25Q32JWM    */
};

#endif /* SPI_FLASH_IDS_H */

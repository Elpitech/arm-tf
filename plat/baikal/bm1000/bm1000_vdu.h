/**

 Copyright (C) 2019-2020 Baikal Electronics JSC

 Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

 Parts of this file were based on sources as follows:

 Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef __BAIKAL_VDU_H
#define __BAIKAL_VDU_H

#define MMAVLSP_VDU_BASE               0x202D0000
#define MMXGBE_VDU_BASE                0x30260000

// Controller Register Offsets
#define BAIKAL_VDU_REG_CR1             0x000
#define BAIKAL_VDU_REG_HTR             0x008
#define BAIKAL_VDU_REG_VTR1            0x00C
#define BAIKAL_VDU_REG_VTR2            0x010
#define BAIKAL_VDU_REG_PCTR            0x014
#define BAIKAL_VDU_REG_ISR             0x018
#define BAIKAL_VDU_REG_IMR             0x01C
#define BAIKAL_VDU_REG_IVR             0x020
#define BAIKAL_VDU_REG_ISCR            0x024
#define BAIKAL_VDU_REG_DBAR            0x028
#define BAIKAL_VDU_REG_DEAR            0x030
#define BAIKAL_VDU_REG_HVTER           0x044
#define BAIKAL_VDU_REG_HPPLOR          0x048
#define BAIKAL_VDU_REG_GPIOR           0x1F8
#define BAIKAL_VDU_REG_CIR             0x1FC
#define BAIKAL_VDU_REG_MRR             0xFFC

/**********************************************************************/

// Register components (register bits)

// This should make life easier to program specific settings in the different 
// registers by simplifying the setting up of the individual bits of each 
// register and then assembling the final register value.

/**********************************************************************/

// Register: HTR
#define HOR_AXIS_PANEL(hbp,hfp,hsw,hor_res) (uint32_t) \
		((((uint32_t)(hsw) - 1) << 24) | \
		(uint32_t)((hbp) << 16) | \
		((uint32_t)((hor_res) / 16) << 8) | \
		((uint32_t)(hfp) << 0))

// Register: VTR1
#define VER_AXIS_PANEL(vbp,vfp,vsw) (uint32_t) \
		(((uint32_t)(vbp) << 16) | \
		((uint32_t)(vfp) << 8) | \
		((uint32_t)(vsw) << 0))

// Register: HVTER
#define TIMINGS_EXT(hbp,hfp,hsw,vbp,vfp,vsw) (uint32_t) \
		(((uint32_t)(vsw / 256) << 24) | \
		((uint32_t)(hsw / 256) << 16) | \
		((uint32_t)(vbp / 256) << 12) | \
		((uint32_t)(vfp / 256) << 8) | \
		((uint32_t)(hbp / 256) << 4) | \
		((uint32_t)(hfp / 256) << 4))

#define BAIKAL_VDU_CR1_FDW_4_WORDS        (0 << 16)
#define BAIKAL_VDU_CR1_FDW_8_WORDS        (1 << 16)
#define BAIKAL_VDU_CR1_FDW_16_WORDS       (2 << 16)
#define BAIKAL_VDU_CR1_OPS_LCD18          (0 << 13)
#define BAIKAL_VDU_CR1_OPS_LCD24          (1 << 13)
#define BAIKAL_VDU_CR1_OPS_555            (1 << 12)
#define BAIKAL_VDU_CR1_VSP                (1 << 11)
#define BAIKAL_VDU_CR1_HSP                (1 << 10)
#define BAIKAL_VDU_CR1_DEP                (1 << 8)
#define BAIKAL_VDU_CR1_BGR                (1 << 5)
#define BAIKAL_VDU_CR1_BPP_MASK           CTL_BIT_MASK(4, 2)
#define BAIKAL_VDU_CR1_BPP1               (0 << 2)
#define BAIKAL_VDU_CR1_BPP2               (1 << 2)
#define BAIKAL_VDU_CR1_BPP4               (2 << 2)
#define BAIKAL_VDU_CR1_BPP8               (3 << 2)
#define BAIKAL_VDU_CR1_BPP16              (4 << 2)
#define BAIKAL_VDU_CR1_BPP18              (5 << 2)
#define BAIKAL_VDU_CR1_BPP24              (6 << 2)
#define BAIKAL_VDU_CR1_LCE                (1 << 0)

#define BAIKAL_VDU_HPPLOR_HPOE            (1 << 31)

#define BAIKAL_VDU_PCTR_PCR               (1 << 10)
#define BAIKAL_VDU_PCTR_PCI               (1 << 9)

#define BAIKAL_VDU_MRR_DEAR_RQ_MASK       CTL_BIT_MASK(2, 0)
#define BAIKAL_VDU_MRR_OUTSTND_RQ(x)      ((x >> 1) << 0)

#define BAIKAL_VDU_INTR_BAU               (1 << 7)
#define BAIKAL_VDU_INTR_VCT               (1 << 6)

#define BAIKAL_VDU_ISCR_VSC_OFF           0x0
#define BAIKAL_VDU_ISCR_VSC_VSW           0x4
#define BAIKAL_VDU_ISCR_VSC_VBP           0x5
#define BAIKAL_VDU_ISCR_VSC_VACTIVE       0x6
#define BAIKAL_VDU_ISCR_VSC_VFP           0x7

#define BAIKAL_VDU_GPIOR_UHD_SNGL_PORT    (0 << 18)
#define BAIKAL_VDU_GPIOR_UHD_DUAL_PORT    (1 << 18)
#define BAIKAL_VDU_GPIOR_UHD_QUAD_PORT    (2 << 18)
#define BAIKAL_VDU_GPIOR_UHD_ENB          (1 << 17)

#define BAIKAL_VDU_PERIPH_ID              0x0090550F

#endif /* __BAIKAL_VDU_H */

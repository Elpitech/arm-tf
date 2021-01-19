#ifndef BE_A57_LCRU_H
#define BE_A57_LCRU_H

#include <stdint.h>

/* CA57 block */
#define BK_CA57_PLL_REG_CTL             0x0000
#define BK_CA57_PLL_REG_CTL1            0x0004
#define BK_CA57_PLL_REG_CTL2            0x0008
#define BK_CA57_PLL_REG_CTL3            0x000C
#define BK_CA57_PLL_REG_CTL4            0x0010
#define BK_CA57_PLL_REG_CTL5            0x0014
#define BK_CA57_PLL_REG_CTL6            0x0018
#define BK_CA57_CLKEN_REG               0x0800
#define BK_CA57_PCLKDBGEN_REG           0x0804
#define BK_CA57_SCLKEN_REG              0x0808
#define BK_CA57_ATCLKEN_REG             0x080C
#define BK_CA57_CNTCLKEN_REG            0x0810
#define BK_CA57_RST_REG                 0x0814
#define BK_CA57_SYS_CFG_REG             0x0818
#define BK_CA57_IDAFF_REG               0x081C
#define BK_CA57_RVBARADDR_0_L_REG       0x0820
#define BK_CA57_RVBARADDR_0_H_REG       0x0824
#define BK_CA57_RVBARADDR_1_L_REG       0x0828
#define BK_CA57_RVBARADDR_1_H_REG       0x082C
#define BK_CA57_BROADCAST_CFG_REG       0x0830
#define BK_CA57_CLUSTER_PWR_CTL_REG     0x0834
#define BK_CA57_CLUSTER_PWR_DMN_REG     0x0838
#define BK_CA57_DBG_CTL_REG             0x083C
#define BK_CA57_CHI_CTL_REG             0x0840
#define BK_CA57_LCRU_IRQ_EN_REG         0x0844
#define BK_CA57_LCRU_IRQ_CLEAR_REG      0x0848
#define BK_CA57_LCRU_IRQ_STATUS_REG     0x084C
#define BK_CA57_DBGROMADDR_REG          0x0850
#define BK_CA57_DBGROMADDR_CTL_REG      0x0854
#define BK_CA57_TSBLPI_CTL_REG          0x0858
#define BK_CA57_SECURE_CTL_REG          0x0C00

#define PLL_CTL_EN              (1 << 0)
#define PLL_CTL_RST             (1 << 1)
#define PLL_CTL_BYPASS          (1 << 30)
#define PLL_CTL_LOCK            (1u << 31)

#define PLL_CTL6_SWEN           (1 << 0)
#define PLL_CTL6_SWRST          (1 << 1)

#define RST_CPUORESET0          (1 << 0)
#define RST_CPUORESET1          (1 << 1)
#define RST_CORERESET0          (1 << 8)
#define RST_CORERESET1          (1 << 9)


#define RST_L2RESET	        (1 << 16)
#define RST_BRDGRST             (1 << 24)
#define RST_BRDGRST2            (1 << 25)
#define RST_BRDGRST3            (1 << 26)
#define RST_BRDGRST4            (1 << 27)
#define RST_PRESETDBG           (1 << 28)
#define RST_GICRST		(1 << 29)

#define RST_CPUORESET           RST_CPUORESET0
#define RST_CORERESET           RST_CORERESET0

int be_ca57_lcru_enable_core(u_register_t mpidr);

#endif

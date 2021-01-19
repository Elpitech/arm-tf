/*
 * Copyright (c) 2018-2020, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bm1000_ca57_lcru.h>
#include <common/debug.h>
#include <drivers/arm/gicv3.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>
#include <platform_def.h>

/* FIXME: It is included in order to implement a workaround
 *	  to set the GICD_CTRL.EnableGrp1NS bit */
#include "../../../drivers/arm/gic/v3/gicv3_private.h"

#ifdef BE_MITX
#include <bm1000_i2c.h>

#define TPL_MITX_BMC_I2C_BUS			0
#define TPL_MITX_BMC_I2C_ADDR			0x08

#define TPL_MITX_BMC_REG_PWROFF_RQ		0x05
#define TPL_MITX_BMC_REG_PWROFF_RQ_OFF		0x01
#define TPL_MITX_BMC_REG_PWROFF_RQ_RESET	0x02
#endif

/*
 * The secure entry point to be used on warm reset.
 */
static unsigned long secure_entrypoint;

/* Make composite power state parameter till power level 0 */
#if PSCI_EXTENDED_STATE_ID
#define baikal_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type) \
		(((lvl0_state) << PSTATE_ID_SHIFT) | \
		 ((type) << PSTATE_TYPE_SHIFT))
#else
#define baikal_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type) \
		(((lvl0_state) << PSTATE_ID_SHIFT) | \
		 ((pwr_lvl) << PSTATE_PWR_LVL_SHIFT) | \
		 ((type) << PSTATE_TYPE_SHIFT))
#endif /* PSCI_EXTENDED_STATE_ID */

#define baikal_make_pwrstate_lvl1(lvl1_state, lvl0_state, pwr_lvl, type) \
		(((lvl1_state) << PLAT_LOCAL_PSTATE_WIDTH) | \
		 baikal_make_pwrstate_lvl0(lvl0_state, pwr_lvl, type))

/*
 *  The table storing the valid idle power states. Ensure that the
 *  array entries are populated in ascending order of state-id to
 *  enable us to use binary search during power state validation.
 *  The table must be terminated by a NULL entry.
 */
static const unsigned int baikal_pm_idle_states[] = {
	/* State-id - 0x01 */
	baikal_make_pwrstate_lvl1(PLAT_LOCAL_STATE_RUN, PLAT_LOCAL_STATE_RET,
				MPIDR_AFFLVL0, PSTATE_TYPE_STANDBY),
	/* State-id - 0x02 */
	baikal_make_pwrstate_lvl1(PLAT_LOCAL_STATE_RUN, PLAT_LOCAL_STATE_OFF,
				MPIDR_AFFLVL0, PSTATE_TYPE_POWERDOWN),
	/* State-id - 0x22 */
	baikal_make_pwrstate_lvl1(PLAT_LOCAL_STATE_OFF, PLAT_LOCAL_STATE_OFF,
				MPIDR_AFFLVL1, PSTATE_TYPE_POWERDOWN),
	0,
};

/*******************************************************************************
 * Platform handler called to check the validity of the power state
 * parameter. The power state parameter has to be a composite power state.
 ******************************************************************************/
static int bm1000_validate_power_state(unsigned int power_state,
				psci_power_state_t *req_state)
{
	unsigned i;
	unsigned state_id;

	assert(req_state);

	/*
	 *  Currently we are using a linear search for finding the matching
	 *  entry in the idle power state array. This can be made a binary
	 *  search if the number of entries justify the additional complexity.
	 */
	for (i = 0; !!baikal_pm_idle_states[i]; i++) {
		if (power_state == baikal_pm_idle_states[i])
			break;
	}

	/* Return error if entry not found in the idle state array */
	if (!baikal_pm_idle_states[i])
		return PSCI_E_INVALID_PARAMS;

	i = 0;
	state_id = psci_get_pstate_id(power_state);

	/* Parse the State ID and populate the state info parameter */
	while (state_id) {
		req_state->pwr_domain_state[i++] = state_id &
						PLAT_LOCAL_PSTATE_MASK;
		state_id >>= PLAT_LOCAL_PSTATE_WIDTH;
	}

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure
 * entrypoint.
 ******************************************************************************/
static int bm1000_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	if ((entrypoint >= NS_DRAM1_BASE) &&
	    (entrypoint < (NS_DRAM1_BASE + NS_DRAM1_SIZE)))
		return PSCI_E_SUCCESS;
	if ((entrypoint >= NS_DRAM0_BASE) &&
	    (entrypoint < (NS_DRAM0_BASE + NS_DRAM0_SIZE)))
		return PSCI_E_SUCCESS;
	return PSCI_E_INVALID_ADDRESS;
}

/*******************************************************************************
 * Platform handler called when a CPU is about to enter standby.
 ******************************************************************************/
static void bm1000_cpu_standby(plat_local_state_t cpu_state)
{
	assert(cpu_state == PLAT_LOCAL_STATE_RET);

	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	dsb();
	wfi();
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be turned on. The
 * mpidr determines the CPU to be turned on.
 ******************************************************************************/
static int bm1000_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned pos = plat_core_pos_by_mpidr(mpidr);
	uint64_t *hold_base = (uint64_t *)PLAT_BAIKAL_HOLD_BASE;

	hold_base[pos] = PLAT_BAIKAL_HOLD_STATE_GO;
	be_ca57_lcru_enable_core(mpidr);
	sev();

	return rc;
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void bm1000_pwr_domain_off(const psci_power_state_t *target_state)
{
	assert(0);
}

/*******************************************************************************
 * Platform handler called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void bm1000_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	assert(0);
}

/*******************************************************************************
 * Platform handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
static void bm1000_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] ==
					PLAT_LOCAL_STATE_OFF);

	/* TODO: This setup is needed only after a cold boot */
	//gicv2_pcpu_distif_init();

	/* Enable the gic cpu interface */
	//gicv2_cpuif_enable();
	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());

	// FIXME: An ugly workaround to set the GICD_CTRL.EnableGrp1NS bit
	gicd_set_ctlr(GICD_BASE, CTLR_ENABLE_G1NS_BIT, RWP_TRUE);

	gicv3_cpuif_enable(plat_my_core_pos());
//	assert(plat_my_core_pos() != 2);
}

/*******************************************************************************
 * Platform handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 ******************************************************************************/
static void bm1000_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	assert(0);
}

/*******************************************************************************
 * Platform handlers to shutdown/reboot the system
 ******************************************************************************/
static void __dead2 bm1000_system_off(void)
{
#ifdef BE_MITX
	static const uint8_t offreq[] = {
		TPL_MITX_BMC_REG_PWROFF_RQ,
		TPL_MITX_BMC_REG_PWROFF_RQ_OFF
	};

	INFO("%s\n", __func__);
	i2c_txrx(TPL_MITX_BMC_I2C_BUS,
		 TPL_MITX_BMC_I2C_ADDR, &offreq, sizeof (offreq), NULL, 0);
	wfi();
	ERROR("%s: operation not handled\n", __func__);
	panic();
#else
	ERROR("%s: operation not supported\n", __func__);
	while (1) {
		wfi();
	}
#endif
}

static void __dead2 bm1000_system_reset(void)
{
#ifdef BE_MITX
	static const uint8_t rstreq[] = {
		TPL_MITX_BMC_REG_PWROFF_RQ,
		TPL_MITX_BMC_REG_PWROFF_RQ_RESET
	};

	INFO("%s\n", __func__);
	i2c_txrx(TPL_MITX_BMC_I2C_BUS,
		 TPL_MITX_BMC_I2C_ADDR, &rstreq, sizeof (rstreq), NULL, 0);
	wfi();
	ERROR("%s: operation not handled\n", __func__);
	panic();
#else
	ERROR("%s: operation not supported\n", __func__);
	while (1) {
		wfi();
	}
#endif
}

static const plat_psci_ops_t plat_baikal_psci_pm_ops = {
	.cpu_standby = bm1000_cpu_standby,
	.pwr_domain_on = bm1000_pwr_domain_on,
	.pwr_domain_off = bm1000_pwr_domain_off,
	.pwr_domain_suspend = bm1000_pwr_domain_suspend,
	.pwr_domain_on_finish = bm1000_pwr_domain_on_finish,
	.pwr_domain_suspend_finish = bm1000_pwr_domain_suspend_finish,
	.system_off = bm1000_system_off,
	.system_reset = bm1000_system_reset,
	.validate_power_state = bm1000_validate_power_state,
	.validate_ns_entrypoint = bm1000_validate_ns_entrypoint
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	uintptr_t *mailbox = (void *)(PLAT_BAIKAL_TRUSTED_MAILBOX_BASE);

	*mailbox = sec_entrypoint;
	secure_entrypoint = (unsigned long)sec_entrypoint;
	*psci_ops = &plat_baikal_psci_pm_ops;

	return 0;
}

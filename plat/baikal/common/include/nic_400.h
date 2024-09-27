/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NIC_400_H
#define NIC_400_H

#define NIC_SECURITY(base, node)	((base) + 0x08 + 0x04 * ((node) - 2))
#define NIC_SECURITY_REGION0_NONSECURE	(1 << 0)

#endif /* NIC_400_H */

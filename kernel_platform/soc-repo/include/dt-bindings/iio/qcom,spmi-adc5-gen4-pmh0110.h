/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _DT_BINDINGS_QCOM_SPMI_VADC_PMH0110_H
#define _DT_BINDINGS_QCOM_SPMI_VADC_PMH0110_H

#include <dt-bindings/iio/qcom,spmi-vadc.h>

/* ADC channels for PMH0110_ADC for PMIC5 Gen4 */
#define PMH0110_ADC5_GEN4_OFFSET_REF(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_OFFSET_REF)
#define PMH0110_ADC5_GEN4_1P25VREF(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_1P25VREF)
#define PMH0110_ADC5_GEN4_VREF_VADC(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_VREF_VADC)
#define PMH0110_ADC5_GEN4_DIE_TEMP(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_DIE_TEMP)
#define PMH0110_ADC5_GEN4_AMUX1_GPIO5(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX1_GPIO)
#define PMH0110_ADC5_GEN4_AMUX2_GPIO6(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX2_GPIO)
#define PMH0110_ADC5_GEN4_ATEST1(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_ATEST1)
#define PMH0110_ADC5_GEN4_ATEST2(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_ATEST2)
#define PMH0110_ADC5_GEN4_ATEST3(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_ATEST3)
#define PMH0110_ADC5_GEN4_ATEST4(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_ATEST4)

/* 100k pull-up */
#define PMH0110_ADC5_GEN4_AMUX1_GPIO5_100K_PU(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX1_GPIO_100K_PU)
#define PMH0110_ADC5_GEN4_AMUX2_GPIO6_100K_PU(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX2_GPIO_100K_PU)

/* 1/3 Divider */
#define PMH0110_ADC5_GEN4_AMUX1_GPIO5_DIV_3(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX1_GPIO_DIV_3)
#define PMH0110_ADC5_GEN4_AMUX2_GPIO6_DIV_3(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_AMUX2_GPIO_DIV_3)
#define PMH0110_ADC5_GEN4_VPH_PWR(bus_id, sid) \
				ADC5_GEN4_VIRTUAL_CHAN(bus_id, sid, ADC5_GEN4_VPH_PWR)

#endif /* _DT_BINDINGS_QCOM_SPMI_VADC_PMH0110_H */

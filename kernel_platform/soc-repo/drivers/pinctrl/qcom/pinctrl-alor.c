// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-msm.h"

#define REG_BASE 0x100000
#define REG_SIZE 0x1000
#define PINGROUP(id, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, wake_off, bit)	\
	{					        \
		.grp = PINCTRL_PINGROUP("gpio" #id,	\
			gpio##id##_pins,		\
			ARRAY_SIZE(gpio##id##_pins)),	\
		.ctl_reg = REG_BASE + REG_SIZE * id,			\
		.io_reg = REG_BASE + 0x4 + REG_SIZE * id,		\
		.intr_cfg_reg = REG_BASE + 0x8 + REG_SIZE * id,		\
		.intr_status_reg = REG_BASE + 0xc + REG_SIZE * id,	\
		.intr_target_reg = REG_BASE + 0x8 + REG_SIZE * id,	\
		.mux_bit = 2,			\
		.pull_bit = 0,			\
		.drv_bit = 6,			\
		.egpio_enable = 12,		\
		.egpio_present = 11,	\
		.oe_bit = 9,			\
		.in_bit = 0,			\
		.out_bit = 1,			\
		.intr_enable_bit = 0,		\
		.intr_status_bit = 0,		\
		.intr_target_bit = 8,		\
		.intr_wakeup_enable_bit = 7,	\
		.intr_wakeup_present_bit = 6,	\
		.intr_target_kpss_val = 3,	\
		.intr_raw_status_bit = 4,	\
		.intr_polarity_bit = 1,		\
		.intr_detection_bit = 2,	\
		.intr_detection_width = 2,	\
		.wake_reg = REG_BASE + wake_off,	\
		.wake_bit = bit,		\
		.funcs = (int[]){			\
			msm_mux_gpio, /* gpio mode */	\
			msm_mux_##f1,			\
			msm_mux_##f2,			\
			msm_mux_##f3,			\
			msm_mux_##f4,			\
			msm_mux_##f5,			\
			msm_mux_##f6,			\
			msm_mux_##f7,			\
			msm_mux_##f8,			\
			msm_mux_##f9,			\
			msm_mux_##f10,			\
			msm_mux_##f11 /* egpio mode */	\
		},					\
		.nfuncs = 12,				\
	}

#define SDC_QDSD_PINGROUP(pg_name, ctl, pull, drv)	\
	{					        \
		.grp = PINCTRL_PINGROUP(#pg_name,	\
			pg_name##_pins,			\
			ARRAY_SIZE(pg_name##_pins)),	\
		.ctl_reg = ctl,				\
		.io_reg = 0,				\
		.intr_cfg_reg = 0,			\
		.intr_status_reg = 0,			\
		.intr_target_reg = 0,			\
		.mux_bit = -1,				\
		.pull_bit = pull,			\
		.drv_bit = drv,				\
		.oe_bit = -1,				\
		.in_bit = -1,				\
		.out_bit = -1,				\
		.intr_enable_bit = -1,			\
		.intr_status_bit = -1,			\
		.intr_target_bit = -1,			\
		.intr_raw_status_bit = -1,		\
		.intr_polarity_bit = -1,		\
		.intr_detection_bit = -1,		\
		.intr_detection_width = -1,		\
	}

#define UFS_RESET(pg_name, offset)			\
	{					        \
		.grp = PINCTRL_PINGROUP(#pg_name,	\
			pg_name##_pins,			\
			ARRAY_SIZE(pg_name##_pins)),	\
		.ctl_reg = offset,			\
		.io_reg = offset + 0xffc,		\
		.intr_cfg_reg = 0,			\
		.intr_status_reg = 0,			\
		.intr_target_reg = 0,			\
		.mux_bit = -1,				\
		.pull_bit = 3,				\
		.drv_bit = 0,				\
		.oe_bit = -1,				\
		.in_bit = -1,				\
		.out_bit = 0,				\
		.intr_enable_bit = -1,			\
		.intr_status_bit = -1,			\
		.intr_target_bit = -1,			\
		.intr_raw_status_bit = -1,		\
		.intr_polarity_bit = -1,		\
		.intr_detection_bit = -1,		\
		.intr_detection_width = -1,		\
	}

#define QUP_I3C(qup_mode, qup_offset)			\
	{						\
		.mode = qup_mode,			\
		.offset = REG_BASE + qup_offset,	\
	}

#define QUP_1_I3C_4_MODE_OFFSET	0xF7000
#define QUP_1_I3C_0_MODE_OFFSET	0xF8000
#define QUP_2_I3C_0_MODE_OFFSET	0xF9000
#define QUP_2_I3C_1_MODE_OFFSET	0xFA000
#define QUP_2_I3C_2_MODE_OFFSET	0xFB000
#define QUP_2_I3C_3_MODE_OFFSET	0xFC000
#define QUP_2_I3C_6_MODE_OFFSET	0xFD000
#define QUP_2_I3C_7_MODE_OFFSET	0xFE000
#define QUP_3_I3C_1_MODE_OFFSET	0xFF000
#define QUP_3_I3C_4_MODE_OFFSET	0x100000
#define QUP_1_I3C_1_MODE_OFFSET	0x109000
#define QUP_3_I3C_0_MODE_OFFSET	0x10A000

static const struct pinctrl_pin_desc alor_pins[] = {
	PINCTRL_PIN(0, "GPIO_0"),
	PINCTRL_PIN(1, "GPIO_1"),
	PINCTRL_PIN(2, "GPIO_2"),
	PINCTRL_PIN(3, "GPIO_3"),
	PINCTRL_PIN(4, "GPIO_4"),
	PINCTRL_PIN(5, "GPIO_5"),
	PINCTRL_PIN(6, "GPIO_6"),
	PINCTRL_PIN(7, "GPIO_7"),
	PINCTRL_PIN(8, "GPIO_8"),
	PINCTRL_PIN(9, "GPIO_9"),
	PINCTRL_PIN(10, "GPIO_10"),
	PINCTRL_PIN(11, "GPIO_11"),
	PINCTRL_PIN(12, "GPIO_12"),
	PINCTRL_PIN(13, "GPIO_13"),
	PINCTRL_PIN(14, "GPIO_14"),
	PINCTRL_PIN(15, "GPIO_15"),
	PINCTRL_PIN(16, "GPIO_16"),
	PINCTRL_PIN(17, "GPIO_17"),
	PINCTRL_PIN(18, "GPIO_18"),
	PINCTRL_PIN(19, "GPIO_19"),
	PINCTRL_PIN(20, "GPIO_20"),
	PINCTRL_PIN(21, "GPIO_21"),
	PINCTRL_PIN(22, "GPIO_22"),
	PINCTRL_PIN(23, "GPIO_23"),
	PINCTRL_PIN(24, "GPIO_24"),
	PINCTRL_PIN(25, "GPIO_25"),
	PINCTRL_PIN(26, "GPIO_26"),
	PINCTRL_PIN(27, "GPIO_27"),
	PINCTRL_PIN(28, "GPIO_28"),
	PINCTRL_PIN(29, "GPIO_29"),
	PINCTRL_PIN(30, "GPIO_30"),
	PINCTRL_PIN(31, "GPIO_31"),
	PINCTRL_PIN(32, "GPIO_32"),
	PINCTRL_PIN(33, "GPIO_33"),
	PINCTRL_PIN(34, "GPIO_34"),
	PINCTRL_PIN(35, "GPIO_35"),
	PINCTRL_PIN(36, "GPIO_36"),
	PINCTRL_PIN(37, "GPIO_37"),
	PINCTRL_PIN(38, "GPIO_38"),
	PINCTRL_PIN(39, "GPIO_39"),
	PINCTRL_PIN(40, "GPIO_40"),
	PINCTRL_PIN(41, "GPIO_41"),
	PINCTRL_PIN(42, "GPIO_42"),
	PINCTRL_PIN(43, "GPIO_43"),
	PINCTRL_PIN(44, "GPIO_44"),
	PINCTRL_PIN(45, "GPIO_45"),
	PINCTRL_PIN(46, "GPIO_46"),
	PINCTRL_PIN(47, "GPIO_47"),
	PINCTRL_PIN(48, "GPIO_48"),
	PINCTRL_PIN(49, "GPIO_49"),
	PINCTRL_PIN(50, "GPIO_50"),
	PINCTRL_PIN(51, "GPIO_51"),
	PINCTRL_PIN(52, "GPIO_52"),
	PINCTRL_PIN(53, "GPIO_53"),
	PINCTRL_PIN(54, "GPIO_54"),
	PINCTRL_PIN(55, "GPIO_55"),
	PINCTRL_PIN(56, "GPIO_56"),
	PINCTRL_PIN(57, "GPIO_57"),
	PINCTRL_PIN(58, "GPIO_58"),
	PINCTRL_PIN(59, "GPIO_59"),
	PINCTRL_PIN(60, "GPIO_60"),
	PINCTRL_PIN(61, "GPIO_61"),
	PINCTRL_PIN(62, "GPIO_62"),
	PINCTRL_PIN(63, "GPIO_63"),
	PINCTRL_PIN(64, "GPIO_64"),
	PINCTRL_PIN(65, "GPIO_65"),
	PINCTRL_PIN(66, "GPIO_66"),
	PINCTRL_PIN(67, "GPIO_67"),
	PINCTRL_PIN(68, "GPIO_68"),
	PINCTRL_PIN(69, "GPIO_69"),
	PINCTRL_PIN(70, "GPIO_70"),
	PINCTRL_PIN(71, "GPIO_71"),
	PINCTRL_PIN(72, "GPIO_72"),
	PINCTRL_PIN(73, "GPIO_73"),
	PINCTRL_PIN(74, "GPIO_74"),
	PINCTRL_PIN(75, "GPIO_75"),
	PINCTRL_PIN(76, "GPIO_76"),
	PINCTRL_PIN(77, "GPIO_77"),
	PINCTRL_PIN(78, "GPIO_78"),
	PINCTRL_PIN(79, "GPIO_79"),
	PINCTRL_PIN(80, "GPIO_80"),
	PINCTRL_PIN(81, "GPIO_81"),
	PINCTRL_PIN(82, "GPIO_82"),
	PINCTRL_PIN(83, "GPIO_83"),
	PINCTRL_PIN(84, "GPIO_84"),
	PINCTRL_PIN(85, "GPIO_85"),
	PINCTRL_PIN(86, "GPIO_86"),
	PINCTRL_PIN(87, "GPIO_87"),
	PINCTRL_PIN(88, "GPIO_88"),
	PINCTRL_PIN(89, "GPIO_89"),
	PINCTRL_PIN(90, "GPIO_90"),
	PINCTRL_PIN(91, "GPIO_91"),
	PINCTRL_PIN(92, "GPIO_92"),
	PINCTRL_PIN(93, "GPIO_93"),
	PINCTRL_PIN(94, "GPIO_94"),
	PINCTRL_PIN(95, "GPIO_95"),
	PINCTRL_PIN(96, "GPIO_96"),
	PINCTRL_PIN(97, "GPIO_97"),
	PINCTRL_PIN(98, "GPIO_98"),
	PINCTRL_PIN(99, "GPIO_99"),
	PINCTRL_PIN(100, "GPIO_100"),
	PINCTRL_PIN(101, "GPIO_101"),
	PINCTRL_PIN(102, "GPIO_102"),
	PINCTRL_PIN(103, "GPIO_103"),
	PINCTRL_PIN(104, "GPIO_104"),
	PINCTRL_PIN(105, "GPIO_105"),
	PINCTRL_PIN(106, "GPIO_106"),
	PINCTRL_PIN(107, "GPIO_107"),
	PINCTRL_PIN(108, "GPIO_108"),
	PINCTRL_PIN(109, "GPIO_109"),
	PINCTRL_PIN(110, "GPIO_110"),
	PINCTRL_PIN(111, "GPIO_111"),
	PINCTRL_PIN(112, "GPIO_112"),
	PINCTRL_PIN(113, "GPIO_113"),
	PINCTRL_PIN(114, "GPIO_114"),
	PINCTRL_PIN(115, "GPIO_115"),
	PINCTRL_PIN(116, "GPIO_116"),
	PINCTRL_PIN(117, "GPIO_117"),
	PINCTRL_PIN(118, "GPIO_118"),
	PINCTRL_PIN(119, "GPIO_119"),
	PINCTRL_PIN(120, "GPIO_120"),
	PINCTRL_PIN(121, "GPIO_121"),
	PINCTRL_PIN(122, "GPIO_122"),
	PINCTRL_PIN(123, "GPIO_123"),
	PINCTRL_PIN(124, "GPIO_124"),
	PINCTRL_PIN(125, "GPIO_125"),
	PINCTRL_PIN(126, "GPIO_126"),
	PINCTRL_PIN(127, "GPIO_127"),
	PINCTRL_PIN(128, "GPIO_128"),
	PINCTRL_PIN(129, "GPIO_129"),
	PINCTRL_PIN(130, "GPIO_130"),
	PINCTRL_PIN(131, "GPIO_131"),
	PINCTRL_PIN(132, "GPIO_132"),
	PINCTRL_PIN(133, "GPIO_133"),
	PINCTRL_PIN(134, "GPIO_134"),
	PINCTRL_PIN(135, "GPIO_135"),
	PINCTRL_PIN(136, "GPIO_136"),
	PINCTRL_PIN(137, "GPIO_137"),
	PINCTRL_PIN(138, "GPIO_138"),
	PINCTRL_PIN(139, "GPIO_139"),
	PINCTRL_PIN(140, "GPIO_140"),
	PINCTRL_PIN(141, "GPIO_141"),
	PINCTRL_PIN(142, "GPIO_142"),
	PINCTRL_PIN(143, "GPIO_143"),
	PINCTRL_PIN(144, "GPIO_144"),
	PINCTRL_PIN(145, "GPIO_145"),
	PINCTRL_PIN(146, "GPIO_146"),
	PINCTRL_PIN(147, "GPIO_147"),
	PINCTRL_PIN(148, "GPIO_148"),
	PINCTRL_PIN(149, "GPIO_149"),
	PINCTRL_PIN(150, "GPIO_150"),
	PINCTRL_PIN(151, "GPIO_151"),
	PINCTRL_PIN(152, "GPIO_152"),
	PINCTRL_PIN(153, "GPIO_153"),
	PINCTRL_PIN(154, "GPIO_154"),
	PINCTRL_PIN(155, "GPIO_155"),
	PINCTRL_PIN(156, "GPIO_156"),
	PINCTRL_PIN(157, "GPIO_157"),
	PINCTRL_PIN(158, "GPIO_158"),
	PINCTRL_PIN(159, "GPIO_159"),
	PINCTRL_PIN(160, "GPIO_160"),
	PINCTRL_PIN(161, "GPIO_161"),
	PINCTRL_PIN(162, "GPIO_162"),
	PINCTRL_PIN(163, "GPIO_163"),
	PINCTRL_PIN(164, "GPIO_164"),
	PINCTRL_PIN(165, "GPIO_165"),
	PINCTRL_PIN(166, "GPIO_166"),
	PINCTRL_PIN(167, "GPIO_167"),
	PINCTRL_PIN(168, "GPIO_168"),
	PINCTRL_PIN(169, "GPIO_169"),
	PINCTRL_PIN(170, "GPIO_170"),
	PINCTRL_PIN(171, "GPIO_171"),
	PINCTRL_PIN(172, "GPIO_172"),
	PINCTRL_PIN(173, "GPIO_173"),
	PINCTRL_PIN(174, "GPIO_174"),
	PINCTRL_PIN(175, "GPIO_175"),
	PINCTRL_PIN(176, "GPIO_176"),
	PINCTRL_PIN(177, "GPIO_177"),
	PINCTRL_PIN(178, "GPIO_178"),
	PINCTRL_PIN(179, "GPIO_179"),
	PINCTRL_PIN(180, "GPIO_180"),
	PINCTRL_PIN(181, "GPIO_181"),
	PINCTRL_PIN(182, "GPIO_182"),
	PINCTRL_PIN(183, "GPIO_183"),
	PINCTRL_PIN(184, "GPIO_184"),
	PINCTRL_PIN(185, "GPIO_185"),
	PINCTRL_PIN(186, "GPIO_186"),
	PINCTRL_PIN(187, "GPIO_187"),
	PINCTRL_PIN(188, "GPIO_188"),
	PINCTRL_PIN(189, "GPIO_189"),
	PINCTRL_PIN(190, "GPIO_190"),
	PINCTRL_PIN(191, "GPIO_191"),
	PINCTRL_PIN(192, "GPIO_192"),
	PINCTRL_PIN(193, "GPIO_193"),
	PINCTRL_PIN(194, "GPIO_194"),
	PINCTRL_PIN(195, "GPIO_195"),
	PINCTRL_PIN(196, "GPIO_196"),
	PINCTRL_PIN(197, "GPIO_197"),
	PINCTRL_PIN(198, "GPIO_198"),
	PINCTRL_PIN(199, "GPIO_199"),
	PINCTRL_PIN(200, "GPIO_200"),
	PINCTRL_PIN(201, "GPIO_201"),
	PINCTRL_PIN(202, "GPIO_202"),
	PINCTRL_PIN(203, "GPIO_203"),
	PINCTRL_PIN(204, "GPIO_204"),
	PINCTRL_PIN(205, "UFS_RESET"),
};

#define DECLARE_MSM_GPIO_PINS(pin) \
	static const unsigned int gpio##pin##_pins[] = { pin }
DECLARE_MSM_GPIO_PINS(0);
DECLARE_MSM_GPIO_PINS(1);
DECLARE_MSM_GPIO_PINS(2);
DECLARE_MSM_GPIO_PINS(3);
DECLARE_MSM_GPIO_PINS(4);
DECLARE_MSM_GPIO_PINS(5);
DECLARE_MSM_GPIO_PINS(6);
DECLARE_MSM_GPIO_PINS(7);
DECLARE_MSM_GPIO_PINS(8);
DECLARE_MSM_GPIO_PINS(9);
DECLARE_MSM_GPIO_PINS(10);
DECLARE_MSM_GPIO_PINS(11);
DECLARE_MSM_GPIO_PINS(12);
DECLARE_MSM_GPIO_PINS(13);
DECLARE_MSM_GPIO_PINS(14);
DECLARE_MSM_GPIO_PINS(15);
DECLARE_MSM_GPIO_PINS(16);
DECLARE_MSM_GPIO_PINS(17);
DECLARE_MSM_GPIO_PINS(18);
DECLARE_MSM_GPIO_PINS(19);
DECLARE_MSM_GPIO_PINS(20);
DECLARE_MSM_GPIO_PINS(21);
DECLARE_MSM_GPIO_PINS(22);
DECLARE_MSM_GPIO_PINS(23);
DECLARE_MSM_GPIO_PINS(24);
DECLARE_MSM_GPIO_PINS(25);
DECLARE_MSM_GPIO_PINS(26);
DECLARE_MSM_GPIO_PINS(27);
DECLARE_MSM_GPIO_PINS(28);
DECLARE_MSM_GPIO_PINS(29);
DECLARE_MSM_GPIO_PINS(30);
DECLARE_MSM_GPIO_PINS(31);
DECLARE_MSM_GPIO_PINS(32);
DECLARE_MSM_GPIO_PINS(33);
DECLARE_MSM_GPIO_PINS(34);
DECLARE_MSM_GPIO_PINS(35);
DECLARE_MSM_GPIO_PINS(36);
DECLARE_MSM_GPIO_PINS(37);
DECLARE_MSM_GPIO_PINS(38);
DECLARE_MSM_GPIO_PINS(39);
DECLARE_MSM_GPIO_PINS(40);
DECLARE_MSM_GPIO_PINS(41);
DECLARE_MSM_GPIO_PINS(42);
DECLARE_MSM_GPIO_PINS(43);
DECLARE_MSM_GPIO_PINS(44);
DECLARE_MSM_GPIO_PINS(45);
DECLARE_MSM_GPIO_PINS(46);
DECLARE_MSM_GPIO_PINS(47);
DECLARE_MSM_GPIO_PINS(48);
DECLARE_MSM_GPIO_PINS(49);
DECLARE_MSM_GPIO_PINS(50);
DECLARE_MSM_GPIO_PINS(51);
DECLARE_MSM_GPIO_PINS(52);
DECLARE_MSM_GPIO_PINS(53);
DECLARE_MSM_GPIO_PINS(54);
DECLARE_MSM_GPIO_PINS(55);
DECLARE_MSM_GPIO_PINS(56);
DECLARE_MSM_GPIO_PINS(57);
DECLARE_MSM_GPIO_PINS(58);
DECLARE_MSM_GPIO_PINS(59);
DECLARE_MSM_GPIO_PINS(60);
DECLARE_MSM_GPIO_PINS(61);
DECLARE_MSM_GPIO_PINS(62);
DECLARE_MSM_GPIO_PINS(63);
DECLARE_MSM_GPIO_PINS(64);
DECLARE_MSM_GPIO_PINS(65);
DECLARE_MSM_GPIO_PINS(66);
DECLARE_MSM_GPIO_PINS(67);
DECLARE_MSM_GPIO_PINS(68);
DECLARE_MSM_GPIO_PINS(69);
DECLARE_MSM_GPIO_PINS(70);
DECLARE_MSM_GPIO_PINS(71);
DECLARE_MSM_GPIO_PINS(72);
DECLARE_MSM_GPIO_PINS(73);
DECLARE_MSM_GPIO_PINS(74);
DECLARE_MSM_GPIO_PINS(75);
DECLARE_MSM_GPIO_PINS(76);
DECLARE_MSM_GPIO_PINS(77);
DECLARE_MSM_GPIO_PINS(78);
DECLARE_MSM_GPIO_PINS(79);
DECLARE_MSM_GPIO_PINS(80);
DECLARE_MSM_GPIO_PINS(81);
DECLARE_MSM_GPIO_PINS(82);
DECLARE_MSM_GPIO_PINS(83);
DECLARE_MSM_GPIO_PINS(84);
DECLARE_MSM_GPIO_PINS(85);
DECLARE_MSM_GPIO_PINS(86);
DECLARE_MSM_GPIO_PINS(87);
DECLARE_MSM_GPIO_PINS(88);
DECLARE_MSM_GPIO_PINS(89);
DECLARE_MSM_GPIO_PINS(90);
DECLARE_MSM_GPIO_PINS(91);
DECLARE_MSM_GPIO_PINS(92);
DECLARE_MSM_GPIO_PINS(93);
DECLARE_MSM_GPIO_PINS(94);
DECLARE_MSM_GPIO_PINS(95);
DECLARE_MSM_GPIO_PINS(96);
DECLARE_MSM_GPIO_PINS(97);
DECLARE_MSM_GPIO_PINS(98);
DECLARE_MSM_GPIO_PINS(99);
DECLARE_MSM_GPIO_PINS(100);
DECLARE_MSM_GPIO_PINS(101);
DECLARE_MSM_GPIO_PINS(102);
DECLARE_MSM_GPIO_PINS(103);
DECLARE_MSM_GPIO_PINS(104);
DECLARE_MSM_GPIO_PINS(105);
DECLARE_MSM_GPIO_PINS(106);
DECLARE_MSM_GPIO_PINS(107);
DECLARE_MSM_GPIO_PINS(108);
DECLARE_MSM_GPIO_PINS(109);
DECLARE_MSM_GPIO_PINS(110);
DECLARE_MSM_GPIO_PINS(111);
DECLARE_MSM_GPIO_PINS(112);
DECLARE_MSM_GPIO_PINS(113);
DECLARE_MSM_GPIO_PINS(114);
DECLARE_MSM_GPIO_PINS(115);
DECLARE_MSM_GPIO_PINS(116);
DECLARE_MSM_GPIO_PINS(117);
DECLARE_MSM_GPIO_PINS(118);
DECLARE_MSM_GPIO_PINS(119);
DECLARE_MSM_GPIO_PINS(120);
DECLARE_MSM_GPIO_PINS(121);
DECLARE_MSM_GPIO_PINS(122);
DECLARE_MSM_GPIO_PINS(123);
DECLARE_MSM_GPIO_PINS(124);
DECLARE_MSM_GPIO_PINS(125);
DECLARE_MSM_GPIO_PINS(126);
DECLARE_MSM_GPIO_PINS(127);
DECLARE_MSM_GPIO_PINS(128);
DECLARE_MSM_GPIO_PINS(129);
DECLARE_MSM_GPIO_PINS(130);
DECLARE_MSM_GPIO_PINS(131);
DECLARE_MSM_GPIO_PINS(132);
DECLARE_MSM_GPIO_PINS(133);
DECLARE_MSM_GPIO_PINS(134);
DECLARE_MSM_GPIO_PINS(135);
DECLARE_MSM_GPIO_PINS(136);
DECLARE_MSM_GPIO_PINS(137);
DECLARE_MSM_GPIO_PINS(138);
DECLARE_MSM_GPIO_PINS(139);
DECLARE_MSM_GPIO_PINS(140);
DECLARE_MSM_GPIO_PINS(141);
DECLARE_MSM_GPIO_PINS(142);
DECLARE_MSM_GPIO_PINS(143);
DECLARE_MSM_GPIO_PINS(144);
DECLARE_MSM_GPIO_PINS(145);
DECLARE_MSM_GPIO_PINS(146);
DECLARE_MSM_GPIO_PINS(147);
DECLARE_MSM_GPIO_PINS(148);
DECLARE_MSM_GPIO_PINS(149);
DECLARE_MSM_GPIO_PINS(150);
DECLARE_MSM_GPIO_PINS(151);
DECLARE_MSM_GPIO_PINS(152);
DECLARE_MSM_GPIO_PINS(153);
DECLARE_MSM_GPIO_PINS(154);
DECLARE_MSM_GPIO_PINS(155);
DECLARE_MSM_GPIO_PINS(156);
DECLARE_MSM_GPIO_PINS(157);
DECLARE_MSM_GPIO_PINS(158);
DECLARE_MSM_GPIO_PINS(159);
DECLARE_MSM_GPIO_PINS(160);
DECLARE_MSM_GPIO_PINS(161);
DECLARE_MSM_GPIO_PINS(162);
DECLARE_MSM_GPIO_PINS(163);
DECLARE_MSM_GPIO_PINS(164);
DECLARE_MSM_GPIO_PINS(165);
DECLARE_MSM_GPIO_PINS(166);
DECLARE_MSM_GPIO_PINS(167);
DECLARE_MSM_GPIO_PINS(168);
DECLARE_MSM_GPIO_PINS(169);
DECLARE_MSM_GPIO_PINS(170);
DECLARE_MSM_GPIO_PINS(171);
DECLARE_MSM_GPIO_PINS(172);
DECLARE_MSM_GPIO_PINS(173);
DECLARE_MSM_GPIO_PINS(174);
DECLARE_MSM_GPIO_PINS(175);
DECLARE_MSM_GPIO_PINS(176);
DECLARE_MSM_GPIO_PINS(177);
DECLARE_MSM_GPIO_PINS(178);
DECLARE_MSM_GPIO_PINS(179);
DECLARE_MSM_GPIO_PINS(180);
DECLARE_MSM_GPIO_PINS(181);
DECLARE_MSM_GPIO_PINS(182);
DECLARE_MSM_GPIO_PINS(183);
DECLARE_MSM_GPIO_PINS(184);
DECLARE_MSM_GPIO_PINS(185);
DECLARE_MSM_GPIO_PINS(186);
DECLARE_MSM_GPIO_PINS(187);
DECLARE_MSM_GPIO_PINS(188);
DECLARE_MSM_GPIO_PINS(189);
DECLARE_MSM_GPIO_PINS(190);
DECLARE_MSM_GPIO_PINS(191);
DECLARE_MSM_GPIO_PINS(192);
DECLARE_MSM_GPIO_PINS(193);
DECLARE_MSM_GPIO_PINS(194);
DECLARE_MSM_GPIO_PINS(195);
DECLARE_MSM_GPIO_PINS(196);
DECLARE_MSM_GPIO_PINS(197);
DECLARE_MSM_GPIO_PINS(198);
DECLARE_MSM_GPIO_PINS(199);
DECLARE_MSM_GPIO_PINS(200);
DECLARE_MSM_GPIO_PINS(201);
DECLARE_MSM_GPIO_PINS(202);
DECLARE_MSM_GPIO_PINS(203);
DECLARE_MSM_GPIO_PINS(204);

static const unsigned int ufs_reset_pins[] = { 205 };

enum alor_functions {
	msm_mux_gpio,
	msm_mux_HOST2WLAN_SOL,
	msm_mux_RESOUT_GPIO_N,
	msm_mux_SDC2_CLK,
	msm_mux_SDC2_CMD,
	msm_mux_SDC2_DATA,
	msm_mux_SDC2_FB_CLK,
	msm_mux_aoss_cti,
	msm_mux_atest_char0,
	msm_mux_atest_char1,
	msm_mux_atest_char2,
	msm_mux_atest_char3,
	msm_mux_atest_char_start,
	msm_mux_atest_usb0,
	msm_mux_atest_usb00,
	msm_mux_atest_usb01,
	msm_mux_atest_usb02,
	msm_mux_atest_usb03,
	msm_mux_audio_ext_mclk0,
	msm_mux_audio_ext_mclk1,
	msm_mux_audio_ref_clk,
	msm_mux_cam_asc_mclk4,
	msm_mux_cam_mclk,
	msm_mux_cci_async_in0,
	msm_mux_cci_async_in1,
	msm_mux_cci_async_in2,
	msm_mux_cci_i2c_scl0,
	msm_mux_cci_i2c_scl1,
	msm_mux_cci_i2c_scl2,
	msm_mux_cci_i2c_scl3,
	msm_mux_cci_i2c_sda0,
	msm_mux_cci_i2c_sda1,
	msm_mux_cci_i2c_sda2,
	msm_mux_cci_i2c_sda3,
	msm_mux_cci_timer0,
	msm_mux_cci_timer1,
	msm_mux_cci_timer2,
	msm_mux_cci_timer3,
	msm_mux_cci_timer4,
	msm_mux_coex_uart1_rx,
	msm_mux_coex_uart1_tx,
	msm_mux_coex_uart2_rx,
	msm_mux_coex_uart2_tx,
	msm_mux_dbg_out_clk,
	msm_mux_ddr_bist_complete,
	msm_mux_ddr_bist_fail,
	msm_mux_ddr_bist_start,
	msm_mux_ddr_bist_stop,
	msm_mux_ddr_pxi0,
	msm_mux_ddr_pxi1,
	msm_mux_ddr_pxi2,
	msm_mux_ddr_pxi3,
	msm_mux_dp_hot,
	msm_mux_egpio,
	msm_mux_gcc_gp1,
	msm_mux_gcc_gp2,
	msm_mux_gcc_gp3,
	msm_mux_gnss_adc0,
	msm_mux_gnss_adc1,
	msm_mux_i2s0_data0,
	msm_mux_i2s0_data1,
	msm_mux_i2s0_sck,
	msm_mux_i2s0_ws,
	msm_mux_i2s1_data0,
	msm_mux_i2s1_data1,
	msm_mux_i2s1_sck,
	msm_mux_i2s1_ws,
	msm_mux_ibi_i3c,
	msm_mux_jitter_bist,
	msm_mux_mdp_esync0_out,
	msm_mux_mdp_esync1_out,
	msm_mux_mdp_vsync,
	msm_mux_mdp_vsync0_out,
	msm_mux_mdp_vsync1_out,
	msm_mux_mdp_vsync2_out,
	msm_mux_mdp_vsync3_out,
	msm_mux_mdp_vsync5_out,
	msm_mux_mdp_vsync_e,
	msm_mux_nav_gpio0,
	msm_mux_nav_gpio1,
	msm_mux_nav_gpio2,
	msm_mux_nav_gpio3,
	msm_mux_pcie0_clk_req_n,
	msm_mux_phase_flag0,
	msm_mux_phase_flag1,
	msm_mux_phase_flag10,
	msm_mux_phase_flag11,
	msm_mux_phase_flag12,
	msm_mux_phase_flag13,
	msm_mux_phase_flag14,
	msm_mux_phase_flag15,
	msm_mux_phase_flag16,
	msm_mux_phase_flag17,
	msm_mux_phase_flag18,
	msm_mux_phase_flag19,
	msm_mux_phase_flag2,
	msm_mux_phase_flag20,
	msm_mux_phase_flag21,
	msm_mux_phase_flag22,
	msm_mux_phase_flag23,
	msm_mux_phase_flag24,
	msm_mux_phase_flag25,
	msm_mux_phase_flag26,
	msm_mux_phase_flag27,
	msm_mux_phase_flag28,
	msm_mux_phase_flag29,
	msm_mux_phase_flag3,
	msm_mux_phase_flag30,
	msm_mux_phase_flag31,
	msm_mux_phase_flag4,
	msm_mux_phase_flag5,
	msm_mux_phase_flag6,
	msm_mux_phase_flag7,
	msm_mux_phase_flag8,
	msm_mux_phase_flag9,
	msm_mux_pll_bist_sync,
	msm_mux_pll_clk_aux,
	msm_mux_prng_rosc0,
	msm_mux_prng_rosc1,
	msm_mux_prng_rosc2,
	msm_mux_prng_rosc3,
	msm_mux_qdss_cti,
	msm_mux_qdss_gpio_traceclk,
	msm_mux_qdss_gpio_tracectl,
	msm_mux_qdss_gpio_tracedata0,
	msm_mux_qdss_gpio_tracedata1,
	msm_mux_qdss_gpio_tracedata10,
	msm_mux_qdss_gpio_tracedata11,
	msm_mux_qdss_gpio_tracedata12,
	msm_mux_qdss_gpio_tracedata13,
	msm_mux_qdss_gpio_tracedata14,
	msm_mux_qdss_gpio_tracedata15,
	msm_mux_qdss_gpio_tracedata2,
	msm_mux_qdss_gpio_tracedata3,
	msm_mux_qdss_gpio_tracedata4,
	msm_mux_qdss_gpio_tracedata5,
	msm_mux_qdss_gpio_tracedata6,
	msm_mux_qdss_gpio_tracedata7,
	msm_mux_qdss_gpio_tracedata8,
	msm_mux_qdss_gpio_tracedata9,
	msm_mux_qlink_big_enable,
	msm_mux_qlink_big_request,
	msm_mux_qlink_little_enable,
	msm_mux_qlink_little_request,
	msm_mux_qlink_wmss,
	msm_mux_qspi0,
	msm_mux_qspi1,
	msm_mux_qspi2,
	msm_mux_qspi3,
	msm_mux_qspi_clk,
	msm_mux_qspi_cs,
	msm_mux_qup1_se0_l0,
	msm_mux_qup1_se0_l1,
	msm_mux_qup1_se0_l2,
	msm_mux_qup1_se0_l3,
	msm_mux_qup1_se1_l0,
	msm_mux_qup1_se1_l1,
	msm_mux_qup1_se1_l2,
	msm_mux_qup1_se1_l3,
	msm_mux_qup1_se2_l0,
	msm_mux_qup1_se2_l1,
	msm_mux_qup1_se2_l2,
	msm_mux_qup1_se2_l3,
	msm_mux_qup1_se2_l4,
	msm_mux_qup1_se2_l5,
	msm_mux_qup1_se2_l6,
	msm_mux_qup1_se3_l0,
	msm_mux_qup1_se3_l1,
	msm_mux_qup1_se3_l2,
	msm_mux_qup1_se3_l3,
	msm_mux_qup1_se4_l0,
	msm_mux_qup1_se4_l1,
	msm_mux_qup1_se4_l2,
	msm_mux_qup1_se4_l3,
	msm_mux_qup1_se5_l0_mira,
	msm_mux_qup1_se5_l0_mirb,
	msm_mux_qup1_se5_l1_mira,
	msm_mux_qup1_se5_l1_mirb,
	msm_mux_qup1_se5_l2_mira,
	msm_mux_qup1_se5_l2_mirb,
	msm_mux_qup1_se5_l3_mira,
	msm_mux_qup1_se5_l3_mirb,
	msm_mux_qup1_se6_l0,
	msm_mux_qup1_se6_l1,
	msm_mux_qup1_se6_l2,
	msm_mux_qup1_se6_l3,
	msm_mux_qup1_se7_l0,
	msm_mux_qup1_se7_l1,
	msm_mux_qup1_se7_l2,
	msm_mux_qup1_se7_l3,
	msm_mux_qup2_se0_l0,
	msm_mux_qup2_se0_l1,
	msm_mux_qup2_se0_l2,
	msm_mux_qup2_se0_l3,
	msm_mux_qup2_se1_l0,
	msm_mux_qup2_se1_l1,
	msm_mux_qup2_se1_l2,
	msm_mux_qup2_se1_l3,
	msm_mux_qup2_se2_l0,
	msm_mux_qup2_se2_l1,
	msm_mux_qup2_se2_l2,
	msm_mux_qup2_se2_l3,
	msm_mux_qup2_se2_l4,
	msm_mux_qup2_se2_l5,
	msm_mux_qup2_se2_l6,
	msm_mux_qup2_se3_l0,
	msm_mux_qup2_se3_l1,
	msm_mux_qup2_se3_l2,
	msm_mux_qup2_se3_l3,
	msm_mux_qup2_se4_l0_mira,
	msm_mux_qup2_se4_l0_mirb,
	msm_mux_qup2_se4_l1_mira,
	msm_mux_qup2_se4_l1_mirb,
	msm_mux_qup2_se4_l2_mira,
	msm_mux_qup2_se4_l2_mirb,
	msm_mux_qup2_se4_l3_mira,
	msm_mux_qup2_se4_l3_mirb,
	msm_mux_qup2_se5_l0,
	msm_mux_qup2_se5_l1,
	msm_mux_qup2_se5_l2,
	msm_mux_qup2_se5_l3,
	msm_mux_qup2_se5_l6,
	msm_mux_qup2_se6_l0,
	msm_mux_qup2_se6_l1,
	msm_mux_qup2_se6_l2,
	msm_mux_qup2_se6_l3,
	msm_mux_qup2_se7_l0,
	msm_mux_qup2_se7_l1,
	msm_mux_qup2_se7_l2,
	msm_mux_qup2_se7_l3,
	msm_mux_qup3_se0_l0,
	msm_mux_qup3_se0_l1,
	msm_mux_qup3_se0_l2,
	msm_mux_qup3_se0_l3,
	msm_mux_qup3_se1_l0,
	msm_mux_qup3_se1_l1,
	msm_mux_qup3_se1_l2,
	msm_mux_qup3_se1_l3,
	msm_mux_qup3_se2_l0,
	msm_mux_qup3_se2_l1,
	msm_mux_qup3_se2_l2,
	msm_mux_qup3_se2_l3,
	msm_mux_qup3_se3_l0,
	msm_mux_qup3_se3_l1,
	msm_mux_qup3_se3_l2,
	msm_mux_qup3_se3_l3,
	msm_mux_qup3_se4_l0,
	msm_mux_qup3_se4_l1,
	msm_mux_qup3_se4_l2,
	msm_mux_qup3_se4_l3,
	msm_mux_sd_write_protect,
	msm_mux_sys_throttle,
	msm_mux_tb_trig_sdc2,
	msm_mux_tmess_prng0,
	msm_mux_tmess_prng1,
	msm_mux_tmess_prng2,
	msm_mux_tmess_prng3,
	msm_mux_tsense_pwm1,
	msm_mux_tsense_pwm2,
	msm_mux_tsense_pwm3,
	msm_mux_tsense_pwm4,
	msm_mux_tsense_pwm5,
	msm_mux_tsense_pwm6,
	msm_mux_tsense_pwm7,
	msm_mux_uim0_clk,
	msm_mux_uim0_data,
	msm_mux_uim0_present,
	msm_mux_uim0_reset,
	msm_mux_uim1_clk_mira,
	msm_mux_uim1_clk_mirb,
	msm_mux_uim1_clk_mirc,
	msm_mux_uim1_clk_mird,
	msm_mux_uim1_data_mira,
	msm_mux_uim1_data_mirb,
	msm_mux_uim1_data_mirc,
	msm_mux_uim1_data_mird,
	msm_mux_uim1_present,
	msm_mux_uim1_reset_mira,
	msm_mux_uim1_reset_mirb,
	msm_mux_uim1_reset_mirc,
	msm_mux_uim1_reset_mird,
	msm_mux_usb1_hs,
	msm_mux_usb_phy,
	msm_mux_vfr_0,
	msm_mux_vfr_1,
	msm_mux_vsense_trigger_mirnat,
	msm_mux_wcn_sw,
	msm_mux_wcn_sw_ctrl,
	msm_mux_NA,
};

static const char *const gpio_groups[] = {
	"gpio0", "gpio1", "gpio2", "gpio3", "gpio4", "gpio5",
	"gpio6", "gpio7", "gpio8", "gpio9", "gpio10", "gpio11",
	"gpio12", "gpio13", "gpio14", "gpio15", "gpio16", "gpio17",
	"gpio18", "gpio19", "gpio20", "gpio21", "gpio22", "gpio23",
	"gpio24", "gpio25", "gpio26", "gpio27", "gpio28", "gpio29",
	"gpio30", "gpio31", "gpio32", "gpio33", "gpio34", "gpio35",
	"gpio36", "gpio37", "gpio38", "gpio39", "gpio40", "gpio41",
	"gpio42", "gpio43", "gpio44", "gpio45", "gpio46", "gpio47",
	"gpio48", "gpio49", "gpio50", "gpio51", "gpio52", "gpio53",
	"gpio54", "gpio55", "gpio56", "gpio57", "gpio58", "gpio59",
	"gpio60", "gpio61", "gpio62", "gpio63", "gpio64", "gpio65",
	"gpio66", "gpio67", "gpio68", "gpio69", "gpio70", "gpio71",
	"gpio72", "gpio73", "gpio74", "gpio75", "gpio76", "gpio77",
	"gpio78", "gpio79", "gpio80", "gpio81", "gpio82", "gpio83",
	"gpio84", "gpio85", "gpio86", "gpio87", "gpio88", "gpio89",
	"gpio90", "gpio91", "gpio92", "gpio93", "gpio94", "gpio95",
	"gpio96", "gpio97", "gpio98", "gpio99", "gpio100", "gpio101",
	"gpio102", "gpio103", "gpio104", "gpio105", "gpio106", "gpio107",
	"gpio108", "gpio109", "gpio110", "gpio111", "gpio112", "gpio113",
	"gpio114", "gpio115", "gpio116", "gpio117", "gpio118", "gpio119",
	"gpio120", "gpio121", "gpio122", "gpio123", "gpio124", "gpio125",
	"gpio126", "gpio127", "gpio128", "gpio129", "gpio130", "gpio131",
	"gpio132", "gpio133", "gpio134", "gpio135", "gpio136", "gpio137",
	"gpio138", "gpio139", "gpio140", "gpio141", "gpio142", "gpio143",
	"gpio144", "gpio145", "gpio146", "gpio147", "gpio148", "gpio149",
	"gpio150", "gpio151", "gpio152", "gpio153", "gpio154", "gpio155",
	"gpio156", "gpio157", "gpio158", "gpio159", "gpio160", "gpio161",
	"gpio162", "gpio163", "gpio164", "gpio165", "gpio166", "gpio167",
	"gpio168", "gpio169", "gpio170", "gpio171", "gpio172", "gpio173",
	"gpio174", "gpio175", "gpio176", "gpio177", "gpio178", "gpio179",
	"gpio180", "gpio181", "gpio182", "gpio183", "gpio184", "gpio185",
	"gpio186", "gpio187", "gpio188", "gpio189", "gpio190", "gpio191",
	"gpio192", "gpio193", "gpio194", "gpio195", "gpio196", "gpio197",
	"gpio198", "gpio199", "gpio200", "gpio201", "gpio202", "gpio203",
	"gpio204",
};
static const char *const HOST2WLAN_SOL_groups[] = {
	"gpio95",
};
static const char *const RESOUT_GPIO_N_groups[] = {
	"gpio97",
};
static const char *const SDC2_CLK_groups[] = {
	"gpio98",
};
static const char *const SDC2_CMD_groups[] = {
	"gpio162",
};
static const char *const SDC2_DATA_groups[] = {
	"gpio121",
	"gpio122",
	"gpio123",
	"gpio124",
};
static const char *const SDC2_FB_CLK_groups[] = {
	"gpio96",
};
static const char *const aoss_cti_groups[] = {
	"gpio50", "gpio51", "gpio60", "gpio61",
};
static const char *const atest_char0_groups[] = {
	"gpio114",
};
static const char *const atest_char1_groups[] = {
	"gpio113",
};
static const char *const atest_char2_groups[] = {
	"gpio118",
};
static const char *const atest_char3_groups[] = {
	"gpio117",
};
static const char *const atest_char_start_groups[] = {
	"gpio115",
};
static const char *const atest_usb0_groups[] = {
	"gpio86",
};
static const char *const atest_usb00_groups[] = {
	"gpio125",
};
static const char *const atest_usb01_groups[] = {
	"gpio63",
};
static const char *const atest_usb02_groups[] = {
	"gpio72",
};
static const char *const atest_usb03_groups[] = {
	"gpio73",
};
static const char *const audio_ext_mclk0_groups[] = {
	"gpio125",
};
static const char *const audio_ext_mclk1_groups[] = {
	"gpio124",
};
static const char *const audio_ref_clk_groups[] = {
	"gpio125",
};
static const char *const cam_asc_mclk4_groups[] = {
	"gpio93",
};
static const char *const cam_mclk_groups[] = {
	"gpio89", "gpio90", "gpio91", "gpio92", "gpio94",
};
static const char *const cci_async_in0_groups[] = {
	"gpio15",
};
static const char *const cci_async_in1_groups[] = {
	"gpio10",
};
static const char *const cci_async_in2_groups[] = {
	"gpio11",
};
static const char *const cci_i2c_scl0_groups[] = {
	"gpio114",
};
static const char *const cci_i2c_scl1_groups[] = {
	"gpio116",
};
static const char *const cci_i2c_scl2_groups[] = {
	"gpio118",
};
static const char *const cci_i2c_scl3_groups[] = {
	"gpio7",
};
static const char *const cci_i2c_sda0_groups[] = {
	"gpio113",
};
static const char *const cci_i2c_sda1_groups[] = {
	"gpio115",
};
static const char *const cci_i2c_sda2_groups[] = {
	"gpio117",
};
static const char *const cci_i2c_sda3_groups[] = {
	"gpio111",
};
static const char *const cci_timer0_groups[] = {
	"gpio109",
};
static const char *const cci_timer1_groups[] = {
	"gpio110",
};
static const char *const cci_timer2_groups[] = {
	"gpio163",
};
static const char *const cci_timer3_groups[] = {
	"gpio164",
};
static const char *const cci_timer4_groups[] = {
	"gpio111",
};
static const char *const coex_uart1_rx_groups[] = {
	"gpio148",
};
static const char *const coex_uart1_tx_groups[] = {
	"gpio149",
};
static const char *const coex_uart2_rx_groups[] = {
	"gpio150",
};
static const char *const coex_uart2_tx_groups[] = {
	"gpio151",
};
static const char *const dbg_out_clk_groups[] = {
	"gpio90",
};
static const char *const ddr_bist_complete_groups[] = {
	"gpio44",
};
static const char *const ddr_bist_fail_groups[] = {
	"gpio40",
};
static const char *const ddr_bist_start_groups[] = {
	"gpio41",
};
static const char *const ddr_bist_stop_groups[] = {
	"gpio45",
};
static const char *const ddr_pxi0_groups[] = {
	"gpio10", "gpio51",
};
static const char *const ddr_pxi1_groups[] = {
	"gpio92", "gpio93",
};
static const char *const ddr_pxi2_groups[] = {
	"gpio43", "gpio127",
};
static const char *const ddr_pxi3_groups[] = {
	"gpio94", "gpio128",
};
static const char *const dp_hot_groups[] = {
	"gpio22",
};
static const char *const egpio_groups[] = {
	"gpio0", "gpio1", "gpio2", "gpio3", "gpio4", "gpio5",
	"gpio8", "gpio9", "gpio20", "gpio21", "gpio22", "gpio23",
	"gpio48", "gpio49", "gpio50", "gpio51", "gpio76", "gpio77",
	"gpio105", "gpio106", "gpio107", "gpio108", "gpio112", "gpio119",
	"gpio120", "gpio153", "gpio165", "gpio166", "gpio167", "gpio168",
	"gpio169", "gpio170", "gpio171", "gpio172", "gpio173", "gpio174",
	"gpio175", "gpio176", "gpio177", "gpio178", "gpio179", "gpio180",
	"gpio181", "gpio182", "gpio183", "gpio184", "gpio185", "gpio186",
	"gpio187", "gpio188", "gpio189", "gpio190", "gpio191", "gpio192",
	"gpio193", "gpio194", "gpio195", "gpio196", "gpio197", "gpio198",
	"gpio199", "gpio200", "gpio201", "gpio202", "gpio203", "gpio204",
};
static const char *const gcc_gp1_groups[] = {
	"gpio48", "gpio76",
};
static const char *const gcc_gp2_groups[] = {
	"gpio49", "gpio77",
};
static const char *const gcc_gp3_groups[] = {
	"gpio125", "gpio136",
};
static const char *const gnss_adc0_groups[] = {
	"gpio90", "gpio91",
};
static const char *const gnss_adc1_groups[] = {
	"gpio60", "gpio87",
};
static const char *const i2s0_data0_groups[] = {
	"gpio127",
};
static const char *const i2s0_data1_groups[] = {
	"gpio128",
};
static const char *const i2s0_sck_groups[] = {
	"gpio126",
};
static const char *const i2s0_ws_groups[] = {
	"gpio129",
};
static const char *const i2s1_data0_groups[] = {
	"gpio122",
};
static const char *const i2s1_data1_groups[] = {
	"gpio124",
};
static const char *const i2s1_sck_groups[] = {
	"gpio121",
};
static const char *const i2s1_ws_groups[] = {
	"gpio123",
};
static const char *const ibi_i3c_groups[] = {
	"gpio0", "gpio1", "gpio4", "gpio5", "gpio8", "gpio9",
	"gpio12", "gpio13", "gpio24", "gpio25", "gpio28", "gpio29",
	"gpio32", "gpio33", "gpio36", "gpio37", "gpio48", "gpio49",
	"gpio52", "gpio53", "gpio64", "gpio65", "gpio68", "gpio69",
};
static const char *const jitter_bist_groups[] = {
	"gpio73",
};
static const char *const mdp_esync0_out_groups[] = {
	"gpio88",
};
static const char *const mdp_esync1_out_groups[] = {
	"gpio100",
};
static const char *const mdp_vsync_groups[] = {
	"gpio55", "gpio86", "gpio87", "gpio97",
};
static const char *const mdp_vsync0_out_groups[] = {
	"gpio86",
};
static const char *const mdp_vsync1_out_groups[] = {
	"gpio86",
};
static const char *const mdp_vsync2_out_groups[] = {
	"gpio87",
};
static const char *const mdp_vsync3_out_groups[] = {
	"gpio87",
};
static const char *const mdp_vsync5_out_groups[] = {
	"gpio87",
};
static const char *const mdp_vsync_e_groups[] = {
	"gpio88",
};
static const char *const nav_gpio0_groups[] = {
	"gpio154",
};
static const char *const nav_gpio1_groups[] = {
	"gpio155",
};
static const char *const nav_gpio2_groups[] = {
	"gpio152",
};
static const char *const nav_gpio3_groups[] = {
	"gpio154",
};
static const char *const pcie0_clk_req_n_groups[] = {
	"gpio103",
};
static const char *const phase_flag0_groups[] = {
	"gpio163",
};
static const char *const phase_flag1_groups[] = {
	"gpio6",
};
static const char *const phase_flag10_groups[] = {
	"gpio34",
};
static const char *const phase_flag11_groups[] = {
	"gpio47",
};
static const char *const phase_flag12_groups[] = {
	"gpio26",
};
static const char *const phase_flag13_groups[] = {
	"gpio27",
};
static const char *const phase_flag14_groups[] = {
	"gpio16",
};
static const char *const phase_flag15_groups[] = {
	"gpio17",
};
static const char *const phase_flag16_groups[] = {
	"gpio18",
};
static const char *const phase_flag17_groups[] = {
	"gpio19",
};
static const char *const phase_flag18_groups[] = {
	"gpio102",
};
static const char *const phase_flag19_groups[] = {
	"gpio103",
};
static const char *const phase_flag2_groups[] = {
	"gpio7",
};
static const char *const phase_flag20_groups[] = {
	"gpio104",
};
static const char *const phase_flag21_groups[] = {
	"gpio117",
};
static const char *const phase_flag22_groups[] = {
	"gpio100",
};
static const char *const phase_flag23_groups[] = {
	"gpio88",
};
static const char *const phase_flag24_groups[] = {
	"gpio109",
};
static const char *const phase_flag25_groups[] = {
	"gpio42",
};
static const char *const phase_flag26_groups[] = {
	"gpio62",
};
static const char *const phase_flag27_groups[] = {
	"gpio31",
};
static const char *const phase_flag28_groups[] = {
	"gpio110",
};
static const char *const phase_flag29_groups[] = {
	"gpio11",
};
static const char *const phase_flag3_groups[] = {
	"gpio113",
};
static const char *const phase_flag30_groups[] = {
	"gpio14",
};
static const char *const phase_flag31_groups[] = {
	"gpio15",
};
static const char *const phase_flag4_groups[] = {
	"gpio114",
};
static const char *const phase_flag5_groups[] = {
	"gpio118",
};
static const char *const phase_flag6_groups[] = {
	"gpio129",
};
static const char *const phase_flag7_groups[] = {
	"gpio44",
};
static const char *const phase_flag8_groups[] = {
	"gpio45",
};
static const char *const phase_flag9_groups[] = {
	"gpio46",
};
static const char *const pll_bist_sync_groups[] = {
	"gpio104",
};
static const char *const pll_clk_aux_groups[] = {
	"gpio111",
};
static const char *const prng_rosc0_groups[] = {
	"gpio163",
};
static const char *const prng_rosc1_groups[] = {
	"gpio6",
};
static const char *const prng_rosc2_groups[] = {
	"gpio7",
};
static const char *const prng_rosc3_groups[] = {
	"gpio113",
};
static const char *const qdss_cti_groups[] = {
	"gpio2", "gpio20", "gpio21", "gpio31", "gpio105", "gpio108",
	"gpio112", "gpio119",
};
static const char *const qdss_gpio_traceclk_groups[] = {
	"gpio92",
};
static const char *const qdss_gpio_tracectl_groups[] = {
	"gpio93",
};
static const char *const qdss_gpio_tracedata0_groups[] = {
	"gpio94",
};
static const char *const qdss_gpio_tracedata1_groups[] = {
	"gpio164",
};
static const char *const qdss_gpio_tracedata10_groups[] = {
	"gpio118",
};
static const char *const qdss_gpio_tracedata11_groups[] = {
	"gpio113",
};
static const char *const qdss_gpio_tracedata12_groups[] = {
	"gpio114",
};
static const char *const qdss_gpio_tracedata13_groups[] = {
	"gpio91",
};
static const char *const qdss_gpio_tracedata14_groups[] = {
	"gpio89",
};
static const char *const qdss_gpio_tracedata15_groups[] = {
	"gpio26",
};
static const char *const qdss_gpio_tracedata2_groups[] = {
	"gpio116",
};
static const char *const qdss_gpio_tracedata3_groups[] = {
	"gpio109",
};
static const char *const qdss_gpio_tracedata4_groups[] = {
	"gpio110",
};
static const char *const qdss_gpio_tracedata5_groups[] = {
	"gpio111",
};
static const char *const qdss_gpio_tracedata6_groups[] = {
	"gpio3",
};
static const char *const qdss_gpio_tracedata7_groups[] = {
	"gpio27",
};
static const char *const qdss_gpio_tracedata8_groups[] = {
	"gpio90",
};
static const char *const qdss_gpio_tracedata9_groups[] = {
	"gpio117",
};
static const char *const qlink_big_enable_groups[] = {
	"gpio160",
};
static const char *const qlink_big_request_groups[] = {
	"gpio159",
};
static const char *const qlink_little_enable_groups[] = {
	"gpio78",
};
static const char *const qlink_little_request_groups[] = {
	"gpio156",
};
static const char *const qlink_wmss_groups[] = {
	"gpio158",
};
static const char *const qspi0_groups[] = {
	"gpio80",
};
static const char *const qspi1_groups[] = {
	"gpio147",
};
static const char *const qspi2_groups[] = {
	"gpio81",
};
static const char *const qspi3_groups[] = {
	"gpio82",
};
static const char *const qspi_clk_groups[] = {
	"gpio83",
};
static const char *const qspi_cs_groups[] = {
	"gpio55", "gpio146",
};
static const char *const qup1_se0_l0_groups[] = {
	"gpio32",
};
static const char *const qup1_se0_l1_groups[] = {
	"gpio33",
};
static const char *const qup1_se0_l2_groups[] = {
	"gpio34",
};
static const char *const qup1_se0_l3_groups[] = {
	"gpio35",
};
static const char *const qup1_se1_l0_groups[] = {
	"gpio36",
};
static const char *const qup1_se1_l1_groups[] = {
	"gpio37",
};
static const char *const qup1_se1_l2_groups[] = {
	"gpio38",
};
static const char *const qup1_se1_l3_groups[] = {
	"gpio39",
};
static const char *const qup1_se2_l0_groups[] = {
	"gpio40",
};
static const char *const qup1_se2_l1_groups[] = {
	"gpio41",
};
static const char *const qup1_se2_l2_groups[] = {
	"gpio42",
};
static const char *const qup1_se2_l3_groups[] = {
	"gpio43",
};
static const char *const qup1_se2_l4_groups[] = {
	"gpio60",
};
static const char *const qup1_se2_l5_groups[] = {
	"gpio61",
};
static const char *const qup1_se2_l6_groups[] = {
	"gpio62",
};
static const char *const qup1_se3_l0_groups[] = {
	"gpio44",
};
static const char *const qup1_se3_l1_groups[] = {
	"gpio45",
};
static const char *const qup1_se3_l2_groups[] = {
	"gpio46",
};
static const char *const qup1_se3_l3_groups[] = {
	"gpio47",
};
static const char *const qup1_se4_l0_groups[] = {
	"gpio48",
};
static const char *const qup1_se4_l1_groups[] = {
	"gpio49",
};
static const char *const qup1_se4_l2_groups[] = {
	"gpio50",
};
static const char *const qup1_se4_l3_groups[] = {
	"gpio51",
};
static const char *const qup1_se5_l0_mira_groups[] = {
	"gpio72",
};
static const char *const qup1_se5_l0_mirb_groups[] = {
	"gpio202",
};
static const char *const qup1_se5_l1_mira_groups[] = {
	"gpio73",
};
static const char *const qup1_se5_l1_mirb_groups[] = {
	"gpio203",
};
static const char *const qup1_se5_l2_mira_groups[] = {
	"gpio74",
};
static const char *const qup1_se5_l2_mirb_groups[] = {
	"gpio202",
};
static const char *const qup1_se5_l3_mira_groups[] = {
	"gpio75",
};
static const char *const qup1_se5_l3_mirb_groups[] = {
	"gpio203",
};
static const char *const qup1_se6_l0_groups[] = {
	"gpio56",
};
static const char *const qup1_se6_l1_groups[] = {
	"gpio57",
};
static const char *const qup1_se6_l2_groups[] = {
	"gpio58",
};
static const char *const qup1_se6_l3_groups[] = {
	"gpio59",
};
static const char *const qup1_se7_l0_groups[] = {
	"gpio60",
};
static const char *const qup1_se7_l1_groups[] = {
	"gpio61",
};
static const char *const qup1_se7_l2_groups[] = {
	"gpio62",
};
static const char *const qup1_se7_l3_groups[] = {
	"gpio63",
};
static const char *const qup2_se0_l0_groups[] = {
	"gpio0",
};
static const char *const qup2_se0_l1_groups[] = {
	"gpio1",
};
static const char *const qup2_se0_l2_groups[] = {
	"gpio2",
};
static const char *const qup2_se0_l3_groups[] = {
	"gpio3",
};
static const char *const qup2_se1_l0_groups[] = {
	"gpio4",
};
static const char *const qup2_se1_l1_groups[] = {
	"gpio5",
};
static const char *const qup2_se1_l2_groups[] = {
	"gpio6",
};
static const char *const qup2_se1_l3_groups[] = {
	"gpio7",
};
static const char *const qup2_se2_l0_groups[] = {
	"gpio8",
};
static const char *const qup2_se2_l1_groups[] = {
	"gpio9",
};
static const char *const qup2_se2_l2_groups[] = {
	"gpio10",
};
static const char *const qup2_se2_l3_groups[] = {
	"gpio11",
};
static const char *const qup2_se2_l4_groups[] = {
	"gpio13",
};
static const char *const qup2_se2_l5_groups[] = {
	"gpio15",
};
static const char *const qup2_se2_l6_groups[] = {
	"gpio12",
};
static const char *const qup2_se3_l0_groups[] = {
	"gpio12",
};
static const char *const qup2_se3_l1_groups[] = {
	"gpio13",
};
static const char *const qup2_se3_l2_groups[] = {
	"gpio14",
};
static const char *const qup2_se3_l3_groups[] = {
	"gpio15",
};
static const char *const qup2_se4_l0_mira_groups[] = {
	"gpio16",
};
static const char *const qup2_se4_l0_mirb_groups[] = {
	"gpio117",
};
static const char *const qup2_se4_l1_mira_groups[] = {
	"gpio17",
};
static const char *const qup2_se4_l1_mirb_groups[] = {
	"gpio118",
};
static const char *const qup2_se4_l2_mira_groups[] = {
	"gpio18",
};
static const char *const qup2_se4_l2_mirb_groups[] = {
	"gpio109",
};
static const char *const qup2_se4_l3_mira_groups[] = {
	"gpio19",
};
static const char *const qup2_se4_l3_mirb_groups[] = {
	"gpio164",
};
static const char *const qup2_se5_l0_groups[] = {
	"gpio102",
};
static const char *const qup2_se5_l1_groups[] = {
	"gpio104",
};
static const char *const qup2_se5_l2_groups[] = {
	"gpio16",
};
static const char *const qup2_se5_l3_groups[] = {
	"gpio17",
};
static const char *const qup2_se5_l6_groups[] = {
	"gpio17",
};
static const char *const qup2_se6_l0_groups[] = {
	"gpio24",
};
static const char *const qup2_se6_l1_groups[] = {
	"gpio25",
};
static const char *const qup2_se6_l2_groups[] = {
	"gpio26",
};
static const char *const qup2_se6_l3_groups[] = {
	"gpio27",
};
static const char *const qup2_se7_l0_groups[] = {
	"gpio28",
};
static const char *const qup2_se7_l1_groups[] = {
	"gpio29",
};
static const char *const qup2_se7_l2_groups[] = {
	"gpio30",
};
static const char *const qup2_se7_l3_groups[] = {
	"gpio31",
};
static const char *const qup3_se0_l0_groups[] = {
	"gpio64",
};
static const char *const qup3_se0_l1_groups[] = {
	"gpio65",
};
static const char *const qup3_se0_l2_groups[] = {
	"gpio66",
};
static const char *const qup3_se0_l3_groups[] = {
	"gpio67",
};
static const char *const qup3_se1_l0_groups[] = {
	"gpio68",
};
static const char *const qup3_se1_l1_groups[] = {
	"gpio69",
};
static const char *const qup3_se1_l2_groups[] = {
	"gpio70",
};
static const char *const qup3_se1_l3_groups[] = {
	"gpio71",
};
static const char *const qup3_se2_l0_groups[] = {
	"gpio80",
};
static const char *const qup3_se2_l1_groups[] = {
	"gpio81",
};
static const char *const qup3_se2_l2_groups[] = {
	"gpio82",
};
static const char *const qup3_se2_l3_groups[] = {
	"gpio83",
};
static const char *const qup3_se3_l0_groups[] = {
	"gpio76",
};
static const char *const qup3_se3_l1_groups[] = {
	"gpio77",
};
static const char *const qup3_se3_l2_groups[] = {
	"gpio146",
};
static const char *const qup3_se3_l3_groups[] = {
	"gpio147",
};
static const char *const qup3_se4_l0_groups[] = {
	"gpio52",
};
static const char *const qup3_se4_l1_groups[] = {
	"gpio53",
};
static const char *const qup3_se4_l2_groups[] = {
	"gpio54",
};
static const char *const qup3_se4_l3_groups[] = {
	"gpio55",
};
static const char *const sd_write_protect_groups[] = {
	"gpio125",
};
static const char *const sys_throttle_groups[] = {
	"gpio99",
};
static const char *const tb_trig_sdc2_groups[] = {
	"gpio87",
};
static const char *const tmess_prng0_groups[] = {
	"gpio163",
};
static const char *const tmess_prng1_groups[] = {
	"gpio6",
};
static const char *const tmess_prng2_groups[] = {
	"gpio7",
};
static const char *const tmess_prng3_groups[] = {
	"gpio113",
};
static const char *const tsense_pwm1_groups[] = {
	"gpio95",
};
static const char *const tsense_pwm2_groups[] = {
	"gpio96",
};
static const char *const tsense_pwm3_groups[] = {
	"gpio98",
};
static const char *const tsense_pwm4_groups[] = {
	"gpio121",
};
static const char *const tsense_pwm5_groups[] = {
	"gpio122",
};
static const char *const tsense_pwm6_groups[] = {
	"gpio123",
};
static const char *const tsense_pwm7_groups[] = {
	"gpio162",
};
static const char *const uim0_clk_groups[] = {
	"gpio131",
};
static const char *const uim0_data_groups[] = {
	"gpio130",
};
static const char *const uim0_present_groups[] = {
	"gpio133",
};
static const char *const uim0_reset_groups[] = {
	"gpio132",
};
static const char *const uim1_clk_mira_groups[] = {
	"gpio135",
};
static const char *const uim1_clk_mirb_groups[] = {
	"gpio31",
};
static const char *const uim1_clk_mirc_groups[] = {
	"gpio55",
};
static const char *const uim1_clk_mird_groups[] = {
	"gpio71",
};
static const char *const uim1_data_mira_groups[] = {
	"gpio134",
};
static const char *const uim1_data_mirb_groups[] = {
	"gpio30",
};
static const char *const uim1_data_mirc_groups[] = {
	"gpio54",
};
static const char *const uim1_data_mird_groups[] = {
	"gpio70",
};
static const char *const uim1_present_groups[] = {
	"gpio137",
};
static const char *const uim1_reset_mira_groups[] = {
	"gpio136",
};
static const char *const uim1_reset_mirb_groups[] = {
	"gpio29",
};
static const char *const uim1_reset_mirc_groups[] = {
	"gpio53",
};
static const char *const uim1_reset_mird_groups[] = {
	"gpio67",
};
static const char *const usb1_hs_groups[] = {
	"gpio56",
};
static const char *const usb_phy_groups[] = {
	"gpio59", "gpio60",
};
static const char *const vfr_0_groups[] = {
	"gpio150",
};
static const char *const vfr_1_groups[] = {
	"gpio155",
};
static const char *const vsense_trigger_mirnat_groups[] = {
	"gpio59",
};
static const char *const wcn_sw_groups[] = {
	"gpio19",
};
static const char *const wcn_sw_ctrl_groups[] = {
	"gpio18",
};

static const struct pinfunction alor_functions[] = {
	MSM_PIN_FUNCTION(gpio),
	MSM_PIN_FUNCTION(HOST2WLAN_SOL),
	MSM_PIN_FUNCTION(RESOUT_GPIO_N),
	MSM_PIN_FUNCTION(SDC2_CLK),
	MSM_PIN_FUNCTION(SDC2_CMD),
	MSM_PIN_FUNCTION(SDC2_DATA),
	MSM_PIN_FUNCTION(SDC2_FB_CLK),
	MSM_PIN_FUNCTION(aoss_cti),
	MSM_PIN_FUNCTION(atest_char0),
	MSM_PIN_FUNCTION(atest_char1),
	MSM_PIN_FUNCTION(atest_char2),
	MSM_PIN_FUNCTION(atest_char3),
	MSM_PIN_FUNCTION(atest_char_start),
	MSM_PIN_FUNCTION(atest_usb0),
	MSM_PIN_FUNCTION(atest_usb00),
	MSM_PIN_FUNCTION(atest_usb01),
	MSM_PIN_FUNCTION(atest_usb02),
	MSM_PIN_FUNCTION(atest_usb03),
	MSM_PIN_FUNCTION(audio_ext_mclk0),
	MSM_PIN_FUNCTION(audio_ext_mclk1),
	MSM_PIN_FUNCTION(audio_ref_clk),
	MSM_PIN_FUNCTION(cam_asc_mclk4),
	MSM_PIN_FUNCTION(cam_mclk),
	MSM_PIN_FUNCTION(cci_async_in0),
	MSM_PIN_FUNCTION(cci_async_in1),
	MSM_PIN_FUNCTION(cci_async_in2),
	MSM_PIN_FUNCTION(cci_i2c_scl0),
	MSM_PIN_FUNCTION(cci_i2c_scl1),
	MSM_PIN_FUNCTION(cci_i2c_scl2),
	MSM_PIN_FUNCTION(cci_i2c_scl3),
	MSM_PIN_FUNCTION(cci_i2c_sda0),
	MSM_PIN_FUNCTION(cci_i2c_sda1),
	MSM_PIN_FUNCTION(cci_i2c_sda2),
	MSM_PIN_FUNCTION(cci_i2c_sda3),
	MSM_PIN_FUNCTION(cci_timer0),
	MSM_PIN_FUNCTION(cci_timer1),
	MSM_PIN_FUNCTION(cci_timer2),
	MSM_PIN_FUNCTION(cci_timer3),
	MSM_PIN_FUNCTION(cci_timer4),
	MSM_PIN_FUNCTION(coex_uart1_rx),
	MSM_PIN_FUNCTION(coex_uart1_tx),
	MSM_PIN_FUNCTION(coex_uart2_rx),
	MSM_PIN_FUNCTION(coex_uart2_tx),
	MSM_PIN_FUNCTION(dbg_out_clk),
	MSM_PIN_FUNCTION(ddr_bist_complete),
	MSM_PIN_FUNCTION(ddr_bist_fail),
	MSM_PIN_FUNCTION(ddr_bist_start),
	MSM_PIN_FUNCTION(ddr_bist_stop),
	MSM_PIN_FUNCTION(ddr_pxi0),
	MSM_PIN_FUNCTION(ddr_pxi1),
	MSM_PIN_FUNCTION(ddr_pxi2),
	MSM_PIN_FUNCTION(ddr_pxi3),
	MSM_PIN_FUNCTION(dp_hot),
	MSM_PIN_FUNCTION(egpio),
	MSM_PIN_FUNCTION(gcc_gp1),
	MSM_PIN_FUNCTION(gcc_gp2),
	MSM_PIN_FUNCTION(gcc_gp3),
	MSM_PIN_FUNCTION(gnss_adc0),
	MSM_PIN_FUNCTION(gnss_adc1),
	MSM_PIN_FUNCTION(i2s0_data0),
	MSM_PIN_FUNCTION(i2s0_data1),
	MSM_PIN_FUNCTION(i2s0_sck),
	MSM_PIN_FUNCTION(i2s0_ws),
	MSM_PIN_FUNCTION(i2s1_data0),
	MSM_PIN_FUNCTION(i2s1_data1),
	MSM_PIN_FUNCTION(i2s1_sck),
	MSM_PIN_FUNCTION(i2s1_ws),
	MSM_PIN_FUNCTION(ibi_i3c),
	MSM_PIN_FUNCTION(jitter_bist),
	MSM_PIN_FUNCTION(mdp_esync0_out),
	MSM_PIN_FUNCTION(mdp_esync1_out),
	MSM_PIN_FUNCTION(mdp_vsync),
	MSM_PIN_FUNCTION(mdp_vsync0_out),
	MSM_PIN_FUNCTION(mdp_vsync1_out),
	MSM_PIN_FUNCTION(mdp_vsync2_out),
	MSM_PIN_FUNCTION(mdp_vsync3_out),
	MSM_PIN_FUNCTION(mdp_vsync5_out),
	MSM_PIN_FUNCTION(mdp_vsync_e),
	MSM_PIN_FUNCTION(nav_gpio0),
	MSM_PIN_FUNCTION(nav_gpio1),
	MSM_PIN_FUNCTION(nav_gpio2),
	MSM_PIN_FUNCTION(nav_gpio3),
	MSM_PIN_FUNCTION(pcie0_clk_req_n),
	MSM_PIN_FUNCTION(phase_flag0),
	MSM_PIN_FUNCTION(phase_flag1),
	MSM_PIN_FUNCTION(phase_flag10),
	MSM_PIN_FUNCTION(phase_flag11),
	MSM_PIN_FUNCTION(phase_flag12),
	MSM_PIN_FUNCTION(phase_flag13),
	MSM_PIN_FUNCTION(phase_flag14),
	MSM_PIN_FUNCTION(phase_flag15),
	MSM_PIN_FUNCTION(phase_flag16),
	MSM_PIN_FUNCTION(phase_flag17),
	MSM_PIN_FUNCTION(phase_flag18),
	MSM_PIN_FUNCTION(phase_flag19),
	MSM_PIN_FUNCTION(phase_flag2),
	MSM_PIN_FUNCTION(phase_flag20),
	MSM_PIN_FUNCTION(phase_flag21),
	MSM_PIN_FUNCTION(phase_flag22),
	MSM_PIN_FUNCTION(phase_flag23),
	MSM_PIN_FUNCTION(phase_flag24),
	MSM_PIN_FUNCTION(phase_flag25),
	MSM_PIN_FUNCTION(phase_flag26),
	MSM_PIN_FUNCTION(phase_flag27),
	MSM_PIN_FUNCTION(phase_flag28),
	MSM_PIN_FUNCTION(phase_flag29),
	MSM_PIN_FUNCTION(phase_flag3),
	MSM_PIN_FUNCTION(phase_flag30),
	MSM_PIN_FUNCTION(phase_flag31),
	MSM_PIN_FUNCTION(phase_flag4),
	MSM_PIN_FUNCTION(phase_flag5),
	MSM_PIN_FUNCTION(phase_flag6),
	MSM_PIN_FUNCTION(phase_flag7),
	MSM_PIN_FUNCTION(phase_flag8),
	MSM_PIN_FUNCTION(phase_flag9),
	MSM_PIN_FUNCTION(pll_bist_sync),
	MSM_PIN_FUNCTION(pll_clk_aux),
	MSM_PIN_FUNCTION(prng_rosc0),
	MSM_PIN_FUNCTION(prng_rosc1),
	MSM_PIN_FUNCTION(prng_rosc2),
	MSM_PIN_FUNCTION(prng_rosc3),
	MSM_PIN_FUNCTION(qdss_cti),
	MSM_PIN_FUNCTION(qdss_gpio_traceclk),
	MSM_PIN_FUNCTION(qdss_gpio_tracectl),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata0),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata1),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata10),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata11),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata12),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata13),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata14),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata15),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata2),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata3),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata4),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata5),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata6),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata7),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata8),
	MSM_PIN_FUNCTION(qdss_gpio_tracedata9),
	MSM_PIN_FUNCTION(qlink_big_enable),
	MSM_PIN_FUNCTION(qlink_big_request),
	MSM_PIN_FUNCTION(qlink_little_enable),
	MSM_PIN_FUNCTION(qlink_little_request),
	MSM_PIN_FUNCTION(qlink_wmss),
	MSM_PIN_FUNCTION(qspi0),
	MSM_PIN_FUNCTION(qspi1),
	MSM_PIN_FUNCTION(qspi2),
	MSM_PIN_FUNCTION(qspi3),
	MSM_PIN_FUNCTION(qspi_clk),
	MSM_PIN_FUNCTION(qspi_cs),
	MSM_PIN_FUNCTION(qup1_se0_l0),
	MSM_PIN_FUNCTION(qup1_se0_l1),
	MSM_PIN_FUNCTION(qup1_se0_l2),
	MSM_PIN_FUNCTION(qup1_se0_l3),
	MSM_PIN_FUNCTION(qup1_se1_l0),
	MSM_PIN_FUNCTION(qup1_se1_l1),
	MSM_PIN_FUNCTION(qup1_se1_l2),
	MSM_PIN_FUNCTION(qup1_se1_l3),
	MSM_PIN_FUNCTION(qup1_se2_l0),
	MSM_PIN_FUNCTION(qup1_se2_l1),
	MSM_PIN_FUNCTION(qup1_se2_l2),
	MSM_PIN_FUNCTION(qup1_se2_l3),
	MSM_PIN_FUNCTION(qup1_se2_l4),
	MSM_PIN_FUNCTION(qup1_se2_l5),
	MSM_PIN_FUNCTION(qup1_se2_l6),
	MSM_PIN_FUNCTION(qup1_se3_l0),
	MSM_PIN_FUNCTION(qup1_se3_l1),
	MSM_PIN_FUNCTION(qup1_se3_l2),
	MSM_PIN_FUNCTION(qup1_se3_l3),
	MSM_PIN_FUNCTION(qup1_se4_l0),
	MSM_PIN_FUNCTION(qup1_se4_l1),
	MSM_PIN_FUNCTION(qup1_se4_l2),
	MSM_PIN_FUNCTION(qup1_se4_l3),
	MSM_PIN_FUNCTION(qup1_se5_l0_mira),
	MSM_PIN_FUNCTION(qup1_se5_l0_mirb),
	MSM_PIN_FUNCTION(qup1_se5_l1_mira),
	MSM_PIN_FUNCTION(qup1_se5_l1_mirb),
	MSM_PIN_FUNCTION(qup1_se5_l2_mira),
	MSM_PIN_FUNCTION(qup1_se5_l2_mirb),
	MSM_PIN_FUNCTION(qup1_se5_l3_mira),
	MSM_PIN_FUNCTION(qup1_se5_l3_mirb),
	MSM_PIN_FUNCTION(qup1_se6_l0),
	MSM_PIN_FUNCTION(qup1_se6_l1),
	MSM_PIN_FUNCTION(qup1_se6_l2),
	MSM_PIN_FUNCTION(qup1_se6_l3),
	MSM_PIN_FUNCTION(qup1_se7_l0),
	MSM_PIN_FUNCTION(qup1_se7_l1),
	MSM_PIN_FUNCTION(qup1_se7_l2),
	MSM_PIN_FUNCTION(qup1_se7_l3),
	MSM_PIN_FUNCTION(qup2_se0_l0),
	MSM_PIN_FUNCTION(qup2_se0_l1),
	MSM_PIN_FUNCTION(qup2_se0_l2),
	MSM_PIN_FUNCTION(qup2_se0_l3),
	MSM_PIN_FUNCTION(qup2_se1_l0),
	MSM_PIN_FUNCTION(qup2_se1_l1),
	MSM_PIN_FUNCTION(qup2_se1_l2),
	MSM_PIN_FUNCTION(qup2_se1_l3),
	MSM_PIN_FUNCTION(qup2_se2_l0),
	MSM_PIN_FUNCTION(qup2_se2_l1),
	MSM_PIN_FUNCTION(qup2_se2_l2),
	MSM_PIN_FUNCTION(qup2_se2_l3),
	MSM_PIN_FUNCTION(qup2_se2_l4),
	MSM_PIN_FUNCTION(qup2_se2_l5),
	MSM_PIN_FUNCTION(qup2_se2_l6),
	MSM_PIN_FUNCTION(qup2_se3_l0),
	MSM_PIN_FUNCTION(qup2_se3_l1),
	MSM_PIN_FUNCTION(qup2_se3_l2),
	MSM_PIN_FUNCTION(qup2_se3_l3),
	MSM_PIN_FUNCTION(qup2_se4_l0_mira),
	MSM_PIN_FUNCTION(qup2_se4_l0_mirb),
	MSM_PIN_FUNCTION(qup2_se4_l1_mira),
	MSM_PIN_FUNCTION(qup2_se4_l1_mirb),
	MSM_PIN_FUNCTION(qup2_se4_l2_mira),
	MSM_PIN_FUNCTION(qup2_se4_l2_mirb),
	MSM_PIN_FUNCTION(qup2_se4_l3_mira),
	MSM_PIN_FUNCTION(qup2_se4_l3_mirb),
	MSM_PIN_FUNCTION(qup2_se5_l0),
	MSM_PIN_FUNCTION(qup2_se5_l1),
	MSM_PIN_FUNCTION(qup2_se5_l2),
	MSM_PIN_FUNCTION(qup2_se5_l3),
	MSM_PIN_FUNCTION(qup2_se5_l6),
	MSM_PIN_FUNCTION(qup2_se6_l0),
	MSM_PIN_FUNCTION(qup2_se6_l1),
	MSM_PIN_FUNCTION(qup2_se6_l2),
	MSM_PIN_FUNCTION(qup2_se6_l3),
	MSM_PIN_FUNCTION(qup2_se7_l0),
	MSM_PIN_FUNCTION(qup2_se7_l1),
	MSM_PIN_FUNCTION(qup2_se7_l2),
	MSM_PIN_FUNCTION(qup2_se7_l3),
	MSM_PIN_FUNCTION(qup3_se0_l0),
	MSM_PIN_FUNCTION(qup3_se0_l1),
	MSM_PIN_FUNCTION(qup3_se0_l2),
	MSM_PIN_FUNCTION(qup3_se0_l3),
	MSM_PIN_FUNCTION(qup3_se1_l0),
	MSM_PIN_FUNCTION(qup3_se1_l1),
	MSM_PIN_FUNCTION(qup3_se1_l2),
	MSM_PIN_FUNCTION(qup3_se1_l3),
	MSM_PIN_FUNCTION(qup3_se2_l0),
	MSM_PIN_FUNCTION(qup3_se2_l1),
	MSM_PIN_FUNCTION(qup3_se2_l2),
	MSM_PIN_FUNCTION(qup3_se2_l3),
	MSM_PIN_FUNCTION(qup3_se3_l0),
	MSM_PIN_FUNCTION(qup3_se3_l1),
	MSM_PIN_FUNCTION(qup3_se3_l2),
	MSM_PIN_FUNCTION(qup3_se3_l3),
	MSM_PIN_FUNCTION(qup3_se4_l0),
	MSM_PIN_FUNCTION(qup3_se4_l1),
	MSM_PIN_FUNCTION(qup3_se4_l2),
	MSM_PIN_FUNCTION(qup3_se4_l3),
	MSM_PIN_FUNCTION(sd_write_protect),
	MSM_PIN_FUNCTION(sys_throttle),
	MSM_PIN_FUNCTION(tb_trig_sdc2),
	MSM_PIN_FUNCTION(tmess_prng0),
	MSM_PIN_FUNCTION(tmess_prng1),
	MSM_PIN_FUNCTION(tmess_prng2),
	MSM_PIN_FUNCTION(tmess_prng3),
	MSM_PIN_FUNCTION(tsense_pwm1),
	MSM_PIN_FUNCTION(tsense_pwm2),
	MSM_PIN_FUNCTION(tsense_pwm3),
	MSM_PIN_FUNCTION(tsense_pwm4),
	MSM_PIN_FUNCTION(tsense_pwm5),
	MSM_PIN_FUNCTION(tsense_pwm6),
	MSM_PIN_FUNCTION(tsense_pwm7),
	MSM_PIN_FUNCTION(uim0_clk),
	MSM_PIN_FUNCTION(uim0_data),
	MSM_PIN_FUNCTION(uim0_present),
	MSM_PIN_FUNCTION(uim0_reset),
	MSM_PIN_FUNCTION(uim1_clk_mira),
	MSM_PIN_FUNCTION(uim1_clk_mirb),
	MSM_PIN_FUNCTION(uim1_clk_mirc),
	MSM_PIN_FUNCTION(uim1_clk_mird),
	MSM_PIN_FUNCTION(uim1_data_mira),
	MSM_PIN_FUNCTION(uim1_data_mirb),
	MSM_PIN_FUNCTION(uim1_data_mirc),
	MSM_PIN_FUNCTION(uim1_data_mird),
	MSM_PIN_FUNCTION(uim1_present),
	MSM_PIN_FUNCTION(uim1_reset_mira),
	MSM_PIN_FUNCTION(uim1_reset_mirb),
	MSM_PIN_FUNCTION(uim1_reset_mirc),
	MSM_PIN_FUNCTION(uim1_reset_mird),
	MSM_PIN_FUNCTION(usb1_hs),
	MSM_PIN_FUNCTION(usb_phy),
	MSM_PIN_FUNCTION(vfr_0),
	MSM_PIN_FUNCTION(vfr_1),
	MSM_PIN_FUNCTION(vsense_trigger_mirnat),
	MSM_PIN_FUNCTION(wcn_sw),
	MSM_PIN_FUNCTION(wcn_sw_ctrl),
};

/* Every pin is maintained as a single group, and missing or non-existing pin
 * would be maintained as dummy group to synchronize pin group index with
 * pin descriptor registered with pinctrl core.
 * Clients would not be able to request these dummy pin groups.
 */
static const struct msm_pingroup alor_groups[] = {
	[0] = PINGROUP(0, qup2_se0_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[1] = PINGROUP(1, qup2_se0_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[2] = PINGROUP(2, qup2_se0_l2, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[3] = PINGROUP(3, qup2_se0_l3, qdss_gpio_tracedata6, NA, NA, NA, NA, NA,
		       NA, NA, NA, egpio, 0, -1),
	[4] = PINGROUP(4, qup2_se1_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[5] = PINGROUP(5, qup2_se1_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[6] = PINGROUP(6, qup2_se1_l2, tmess_prng1, phase_flag1, prng_rosc1, NA,
		       NA, NA, NA, NA, NA, NA, 0, -1),
	[7] = PINGROUP(7, qup2_se1_l3, cci_i2c_scl3, tmess_prng2, phase_flag2,
		       prng_rosc2, NA, NA, NA, NA, NA, NA, 0, -1),
	[8] = PINGROUP(8, qup2_se2_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[9] = PINGROUP(9, qup2_se2_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA, NA,
		       egpio, 0, -1),
	[10] = PINGROUP(10, qup2_se2_l2, cci_async_in1, ddr_pxi0, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[11] = PINGROUP(11, qup2_se2_l3, cci_async_in2, phase_flag29, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[12] = PINGROUP(12, qup2_se3_l0, ibi_i3c, qup2_se2_l6, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[13] = PINGROUP(13, qup2_se3_l1, ibi_i3c, qup2_se2_l4, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[14] = PINGROUP(14, qup2_se3_l2, phase_flag30, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[15] = PINGROUP(15, qup2_se3_l3, cci_async_in0, qup2_se2_l5,
			phase_flag31, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[16] = PINGROUP(16, qup2_se4_l0_mira, qup2_se5_l2, phase_flag14, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[17] = PINGROUP(17, qup2_se4_l1_mira, qup2_se5_l3, qup2_se5_l6,
			phase_flag15, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[18] = PINGROUP(18, wcn_sw_ctrl, qup2_se4_l2_mira, phase_flag16, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[19] = PINGROUP(19, wcn_sw, qup2_se4_l3_mira, phase_flag17, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[20] = PINGROUP(20, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio,
			0, -1),
	[21] = PINGROUP(21, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio,
			0, -1),
	[22] = PINGROUP(22, dp_hot, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio,
			0, -1),
	[23] = PINGROUP(23, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			-1),
	[24] = PINGROUP(24, qup2_se6_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[25] = PINGROUP(25, qup2_se6_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[26] = PINGROUP(26, qup2_se6_l2, phase_flag12, NA,
			qdss_gpio_tracedata15, NA, NA, NA, NA, NA, NA, NA, 0,
			-1),
	[27] = PINGROUP(27, qup2_se6_l3, phase_flag13, NA, qdss_gpio_tracedata7,
			NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[28] = PINGROUP(28, qup2_se7_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[29] = PINGROUP(29, qup2_se7_l1, uim1_reset_mirb, ibi_i3c, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[30] = PINGROUP(30, qup2_se7_l2, uim1_data_mirb, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[31] = PINGROUP(31, qup2_se7_l3, uim1_clk_mirb, phase_flag27, NA,
			qdss_cti, NA, NA, NA, NA, NA, NA, 0, -1),
	[32] = PINGROUP(32, qup1_se0_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[33] = PINGROUP(33, qup1_se0_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[34] = PINGROUP(34, qup1_se0_l2, phase_flag10, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[35] = PINGROUP(35, qup1_se0_l3, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[36] = PINGROUP(36, qup1_se1_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[37] = PINGROUP(37, qup1_se1_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[38] = PINGROUP(38, qup1_se1_l2, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[39] = PINGROUP(39, qup1_se1_l3, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[40] = PINGROUP(40, qup1_se2_l0, ddr_bist_fail, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[41] = PINGROUP(41, qup1_se2_l1, ddr_bist_start, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[42] = PINGROUP(42, qup1_se2_l2, phase_flag25, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[43] = PINGROUP(43, qup1_se2_l3, NA, ddr_pxi2, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[44] = PINGROUP(44, qup1_se3_l0, phase_flag7, ddr_bist_complete, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[45] = PINGROUP(45, qup1_se3_l1, phase_flag8, ddr_bist_stop, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[46] = PINGROUP(46, qup1_se3_l2, phase_flag9, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[47] = PINGROUP(47, qup1_se3_l3, phase_flag11, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[48] = PINGROUP(48, qup1_se4_l0, ibi_i3c, gcc_gp1, NA, NA, NA, NA, NA,
			NA, NA, egpio, 0, -1),
	[49] = PINGROUP(49, qup1_se4_l1, ibi_i3c, gcc_gp2, NA, NA, NA, NA, NA,
			NA, NA, egpio, 0, -1),
	[50] = PINGROUP(50, qup1_se4_l2, aoss_cti, NA, NA, NA, NA, NA, NA, NA,
			NA, egpio, 0, -1),
	[51] = PINGROUP(51, qup1_se4_l3, aoss_cti, ddr_pxi0, NA, NA, NA, NA, NA,
			NA, NA, egpio, 0, -1),
	[52] = PINGROUP(52, qup3_se4_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[53] = PINGROUP(53, qup3_se4_l1, ibi_i3c, uim1_reset_mirc, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[54] = PINGROUP(54, qup3_se4_l2, uim1_data_mirc, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[55] = PINGROUP(55, qup3_se4_l3, qspi_cs, uim1_clk_mirc, mdp_vsync, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[56] = PINGROUP(56, qup1_se6_l0, usb1_hs, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[57] = PINGROUP(57, qup1_se6_l1, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[58] = PINGROUP(58, qup1_se6_l2, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[59] = PINGROUP(59, qup1_se6_l3, usb_phy, vsense_trigger_mirnat, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[60] = PINGROUP(60, qup1_se7_l0, usb_phy, aoss_cti, qup1_se2_l4,
			gnss_adc1, NA, NA, NA, NA, NA, NA, 0, -1),
	[61] = PINGROUP(61, qup1_se7_l1, qup1_se2_l5, aoss_cti, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[62] = PINGROUP(62, qup1_se7_l2, qup1_se2_l6, phase_flag26, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[63] = PINGROUP(63, qup1_se7_l3, NA, atest_usb01, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[64] = PINGROUP(64, qup3_se0_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[65] = PINGROUP(65, qup3_se0_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[66] = PINGROUP(66, qup3_se0_l2, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			0, -1),
	[67] = PINGROUP(67, qup3_se0_l3, uim1_reset_mird, NA, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[68] = PINGROUP(68, qup3_se1_l0, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[69] = PINGROUP(69, qup3_se1_l1, ibi_i3c, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[70] = PINGROUP(70, qup3_se1_l2, uim1_data_mird, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[71] = PINGROUP(71, qup3_se1_l3, uim1_clk_mird, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[72] = PINGROUP(72, qup1_se5_l0_mira, NA, atest_usb02, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[73] = PINGROUP(73, qup1_se5_l1_mira, jitter_bist, atest_usb03, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[74] = PINGROUP(74, qup1_se5_l2_mira, NA, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[75] = PINGROUP(75, qup1_se5_l3_mira, NA, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[76] = PINGROUP(76, qup3_se3_l0, gcc_gp1, NA, NA, NA, NA, NA, NA, NA,
			NA, egpio, 0, -1),
	[77] = PINGROUP(77, qup3_se3_l1, gcc_gp2, NA, NA, NA, NA, NA, NA, NA,
			NA, egpio, 0, -1),
	[78] = PINGROUP(78, qlink_little_enable, NA, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[79] = PINGROUP(79, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[80] = PINGROUP(80, qup3_se2_l0, NA, qspi0, NA, NA, NA, NA, NA, NA, NA,
			NA, 0, -1),
	[81] = PINGROUP(81, qup3_se2_l1, NA, qspi2, NA, NA, NA, NA, NA, NA, NA,
			NA, 0, -1),
	[82] = PINGROUP(82, qup3_se2_l2, NA, NA, qspi3, NA, NA, NA, NA, NA, NA,
			NA, 0, -1),
	[83] = PINGROUP(83, qup3_se2_l3, NA, NA, qspi_clk, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[84] = PINGROUP(84, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[85] = PINGROUP(85, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[86] = PINGROUP(86, mdp_vsync, mdp_vsync0_out, mdp_vsync1_out,
			atest_usb0, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[87] = PINGROUP(87, mdp_vsync, mdp_vsync2_out, mdp_vsync3_out,
			mdp_vsync5_out, tb_trig_sdc2, gnss_adc1, NA, NA, NA, NA,
			NA, 0, -1),
	[88] = PINGROUP(88, mdp_vsync_e, mdp_esync0_out, phase_flag23, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[89] = PINGROUP(89, cam_mclk, qdss_gpio_tracedata14, NA, NA, NA, NA, NA,
			NA, NA, NA, NA, 0, -1),
	[90] = PINGROUP(90, cam_mclk, dbg_out_clk, qdss_gpio_tracedata8,
			gnss_adc0, NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[91] = PINGROUP(91, cam_mclk, qdss_gpio_tracedata13, gnss_adc0, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[92] = PINGROUP(92, cam_mclk, qdss_gpio_traceclk, ddr_pxi1, NA, NA, NA,
			NA, NA, NA, NA, NA, 0, -1),
	[93] = PINGROUP(93, cam_asc_mclk4, qdss_gpio_tracectl, ddr_pxi1, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[94] = PINGROUP(94, cam_mclk, qdss_gpio_tracedata0, ddr_pxi3, NA, NA,
			NA, NA, NA, NA, NA, NA, 0, -1),
	[95] = PINGROUP(95, HOST2WLAN_SOL, tsense_pwm1, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[96] = PINGROUP(96, SDC2_FB_CLK, tsense_pwm2, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[97] = PINGROUP(97, RESOUT_GPIO_N, mdp_vsync, NA, NA, NA, NA, NA, NA,
			NA, NA, NA, 0, -1),
	[98] = PINGROUP(98, SDC2_CLK, tsense_pwm3, NA, NA, NA, NA, NA, NA, NA,
			NA, NA, 0, -1),
	[99] = PINGROUP(99, sys_throttle, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			NA, 0, -1),
	[100] = PINGROUP(100, mdp_esync1_out, phase_flag22, NA, NA, NA, NA, NA,
			 NA, NA, NA, NA, 0, -1),
	[101] = PINGROUP(101, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[102] = PINGROUP(102, qup2_se5_l0, phase_flag18, NA, NA, NA, NA, NA, NA,
			 NA, NA, NA, 0, -1),
	[103] = PINGROUP(103, pcie0_clk_req_n, phase_flag19, NA, NA, NA, NA, NA,
			 NA, NA, NA, NA, 0, -1),
	[104] = PINGROUP(104, qup2_se5_l1, pll_bist_sync, phase_flag20, NA, NA,
			 NA, NA, NA, NA, NA, NA, 0, -1),
	[105] = PINGROUP(105, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 egpio, 0, -1),
	[106] = PINGROUP(106, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[107] = PINGROUP(107, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[108] = PINGROUP(108, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 egpio, 0, -1),
	[109] = PINGROUP(109, qup2_se4_l2_mirb, cci_timer0, phase_flag24, NA,
			 qdss_gpio_tracedata3, NA, NA, NA, NA, NA, NA, 0, -1),
	[110] = PINGROUP(110, cci_timer1, phase_flag28, NA,
			 qdss_gpio_tracedata4, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[111] = PINGROUP(111, cci_timer4, cci_i2c_sda3, pll_clk_aux,
			 qdss_gpio_tracedata5, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[112] = PINGROUP(112, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 egpio, 0, -1),
	[113] = PINGROUP(113, cci_i2c_sda0, tmess_prng3, phase_flag3,
			 prng_rosc3, NA, qdss_gpio_tracedata11, atest_char1, NA,
			 NA, NA, NA, 0, -1),
	[114] = PINGROUP(114, cci_i2c_scl0, phase_flag4, NA,
			 qdss_gpio_tracedata12, atest_char0, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[115] = PINGROUP(115, cci_i2c_sda1, atest_char_start, NA, NA, NA, NA,
			 NA, NA, NA, NA, NA, 0, -1),
	[116] = PINGROUP(116, cci_i2c_scl1, qdss_gpio_tracedata2, NA, NA, NA,
			 NA, NA, NA, NA, NA, NA, 0, -1),
	[117] = PINGROUP(117, cci_i2c_sda2, qup2_se4_l0_mirb, phase_flag21, NA,
			 qdss_gpio_tracedata9, atest_char3, NA, NA, NA, NA, NA,
			 0, -1),
	[118] = PINGROUP(118, cci_i2c_scl2, qup2_se4_l1_mirb, phase_flag5, NA,
			 qdss_gpio_tracedata10, atest_char2, NA, NA, NA, NA, NA,
			 0, -1),
	[119] = PINGROUP(119, qdss_cti, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 egpio, 0, -1),
	[120] = PINGROUP(120, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[121] = PINGROUP(121, i2s1_sck, SDC2_DATA, tsense_pwm4, NA, NA, NA, NA,
			 NA, NA, NA, NA, 0, -1),
	[122] = PINGROUP(122, i2s1_data0, SDC2_DATA, tsense_pwm5, NA, NA, NA,
			 NA, NA, NA, NA, NA, 0, -1),
	[123] = PINGROUP(123, i2s1_ws, SDC2_DATA, tsense_pwm6, NA, NA, NA, NA,
			 NA, NA, NA, NA, 0, -1),
	[124] = PINGROUP(124, i2s1_data1, SDC2_DATA, audio_ext_mclk1, NA, NA,
			 NA, NA, NA, NA, NA, NA, 0, -1),
	[125] = PINGROUP(125, audio_ext_mclk0, sd_write_protect, audio_ref_clk,
			 gcc_gp3, NA, atest_usb00, NA, NA, NA, NA, NA, 0, -1),
	[126] = PINGROUP(126, i2s0_sck, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 0, -1),
	[127] = PINGROUP(127, i2s0_data0, NA, ddr_pxi2, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[128] = PINGROUP(128, i2s0_data1, NA, ddr_pxi3, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[129] = PINGROUP(129, i2s0_ws, phase_flag6, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[130] = PINGROUP(130, uim0_data, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 0, -1),
	[131] = PINGROUP(131, uim0_clk, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 0, -1),
	[132] = PINGROUP(132, uim0_reset, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[133] = PINGROUP(133, uim0_present, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[134] = PINGROUP(134, uim1_data_mira, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[135] = PINGROUP(135, uim1_clk_mira, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[136] = PINGROUP(136, uim1_reset_mira, gcc_gp3, NA, NA, NA, NA, NA, NA,
			 NA, NA, NA, 0, -1),
	[137] = PINGROUP(137, uim1_present, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[138] = PINGROUP(138, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[139] = PINGROUP(139, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[140] = PINGROUP(140, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[141] = PINGROUP(141, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[142] = PINGROUP(142, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[143] = PINGROUP(143, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[144] = PINGROUP(144, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[145] = PINGROUP(145, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[146] = PINGROUP(146, qup3_se3_l2, NA, NA, NA, NA, qspi_cs, NA, NA, NA,
			 NA, NA, 0, -1),
	[147] = PINGROUP(147, qup3_se3_l3, NA, NA, NA, qspi1, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[148] = PINGROUP(148, coex_uart1_rx, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[149] = PINGROUP(149, coex_uart1_tx, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[150] = PINGROUP(150, NA, vfr_0, coex_uart2_rx, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[151] = PINGROUP(151, NA, coex_uart2_tx, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[152] = PINGROUP(152, nav_gpio2, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 0, -1),
	[153] = PINGROUP(153, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[154] = PINGROUP(154, nav_gpio0, nav_gpio3, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[155] = PINGROUP(155, nav_gpio1, vfr_1, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[156] = PINGROUP(156, qlink_little_request, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, NA, 0, -1),
	[157] = PINGROUP(157, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[158] = PINGROUP(158, qlink_wmss, NA, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, 0, -1),
	[159] = PINGROUP(159, qlink_big_request, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[160] = PINGROUP(160, qlink_big_enable, NA, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[161] = PINGROUP(161, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 0,
			 -1),
	[162] = PINGROUP(162, SDC2_CMD, tsense_pwm7, NA, NA, NA, NA, NA, NA, NA,
			 NA, NA, 0, -1),
	[163] = PINGROUP(163, cci_timer2, prng_rosc0, tmess_prng0, phase_flag0,
			 NA, NA, NA, NA, NA, NA, NA, 0, -1),
	[164] = PINGROUP(164, qup2_se4_l3_mirb, cci_timer3,
			 qdss_gpio_tracedata1, NA, NA, NA, NA, NA, NA, NA, NA,
			 0, -1),
	[165] = PINGROUP(165, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[166] = PINGROUP(166, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[167] = PINGROUP(167, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[168] = PINGROUP(168, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[169] = PINGROUP(169, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[170] = PINGROUP(170, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[171] = PINGROUP(171, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[172] = PINGROUP(172, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[173] = PINGROUP(173, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[174] = PINGROUP(174, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[175] = PINGROUP(175, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[176] = PINGROUP(176, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[177] = PINGROUP(177, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[178] = PINGROUP(178, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[179] = PINGROUP(179, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[180] = PINGROUP(180, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[181] = PINGROUP(181, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[182] = PINGROUP(182, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[183] = PINGROUP(183, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[184] = PINGROUP(184, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[185] = PINGROUP(185, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[186] = PINGROUP(186, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[187] = PINGROUP(187, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[188] = PINGROUP(188, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[189] = PINGROUP(189, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[190] = PINGROUP(190, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[191] = PINGROUP(191, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[192] = PINGROUP(192, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[193] = PINGROUP(193, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[194] = PINGROUP(194, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[195] = PINGROUP(195, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[196] = PINGROUP(196, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[197] = PINGROUP(197, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[198] = PINGROUP(198, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[199] = PINGROUP(199, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[200] = PINGROUP(200, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[201] = PINGROUP(201, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[202] = PINGROUP(202, qup1_se5_l0_mirb, qup1_se5_l2_mirb, NA, NA, NA,
			 NA, NA, NA, NA, NA, egpio, 0, -1),
	[203] = PINGROUP(203, qup1_se5_l1_mirb, qup1_se5_l3_mirb, NA, NA, NA,
			 NA, NA, NA, NA, NA, egpio, 0, -1),
	[204] = PINGROUP(204, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, egpio, 0,
			 -1),
	[205] = UFS_RESET(ufs_reset, 0x1E8004),
};

static struct pinctrl_qup alor_qup_regs[] = {
	QUP_I3C(1, QUP_1_I3C_4_MODE_OFFSET),
	QUP_I3C(2, QUP_1_I3C_0_MODE_OFFSET),
	QUP_I3C(3, QUP_2_I3C_0_MODE_OFFSET),
	QUP_I3C(4, QUP_2_I3C_1_MODE_OFFSET),
	QUP_I3C(5, QUP_2_I3C_2_MODE_OFFSET),
	QUP_I3C(6, QUP_2_I3C_3_MODE_OFFSET),
	QUP_I3C(7, QUP_2_I3C_6_MODE_OFFSET),
	QUP_I3C(8, QUP_2_I3C_7_MODE_OFFSET),
	QUP_I3C(9, QUP_3_I3C_1_MODE_OFFSET),
	QUP_I3C(10, QUP_3_I3C_4_MODE_OFFSET),
	QUP_I3C(11, QUP_1_I3C_1_MODE_OFFSET),
	QUP_I3C(12, QUP_3_I3C_0_MODE_OFFSET),
};

static const struct msm_gpio_wakeirq_map alor_pdc_map[] = {
	{ 0, 98 }, { 3, 120 }, { 4, 95 }, { 7, 105 }, { 8, 101 }, { 11, 102 },
	{ 12, 94 }, { 15, 104 }, { 17, 171 }, { 18, 96 }, { 19, 115 }, { 20, 132 },
	{ 21, 108 }, { 22, 110 }, { 24, 109 }, { 27, 111 }, { 28, 112 }, { 31, 125 },
	{ 32, 118 }, { 35, 114 }, { 36, 119 }, { 39, 121 }, { 43, 106 }, { 47, 116 },
	{ 48, 123 }, { 51, 122 }, { 52, 152 }, { 55, 182 }, { 56, 153 }, { 57, 129 },
	{ 59, 130 }, { 60, 131 }, { 61, 139 }, { 62, 141 }, { 63, 99 }, { 64, 97 },
	{ 65, 126 }, { 66, 142 }, { 67, 100 }, { 68, 107 }, { 71, 128 }, { 74, 140 },
	{ 75, 185 }, { 76, 93 }, { 77, 163 }, { 80, 148 }, { 83, 124 }, { 84, 117 },
	{ 86, 149 }, { 87, 150 }, { 88, 136 }, { 97, 169 }, { 103, 156 }, { 104, 103 },
	{ 108, 127 }, { 110, 155 }, { 112, 158 }, { 122, 187 }, { 124, 188 }, { 133, 113 },
	{ 137, 135 }, { 147, 183 }, { 148, 159 }, { 150, 160 }, { 152, 134 }, { 153, 151 },
	{ 154, 161 }, { 155, 133 }, { 156, 162 }, { 159, 164 }, { 162, 186 }, { 164, 157 },
	{ 166, 167 }, { 169, 166 }, { 171, 172 }, { 172, 168 }, { 174, 144 }, { 176, 173 },
	{ 177, 174 }, { 181, 175 }, { 182, 177 }, { 185, 145 }, { 188, 184 }, { 190, 138 },
	{ 191, 176 }, { 192, 178 }, { 193, 165 }, { 195, 154 }, { 196, 179 }, { 197, 180 },
	{ 198, 143 }, { 199, 137 }, { 200, 146 }, { 201, 181 }, { 203, 147 }, { 204, 170 },
};

static const struct msm_pinctrl_soc_data alor_tlmm = {
	.pins = alor_pins,
	.npins = ARRAY_SIZE(alor_pins),
	.functions = alor_functions,
	.nfunctions = ARRAY_SIZE(alor_functions),
	.groups = alor_groups,
	.ngroups = ARRAY_SIZE(alor_groups),
	.ngpios = 206,
	.qup_regs = alor_qup_regs,
	.nqup_regs = ARRAY_SIZE(alor_qup_regs),
	.wakeirq_map = alor_pdc_map,
	.nwakeirq_map = ARRAY_SIZE(alor_pdc_map),
	.egpio_func = 11,
};

static const struct msm_pinctrl_soc_data alor_vm_tlmm = {
	.pins = alor_pins,
	.npins = ARRAY_SIZE(alor_pins),
	.functions = alor_functions,
	.nfunctions = ARRAY_SIZE(alor_functions),
	.groups = alor_groups,
	.ngroups = ARRAY_SIZE(alor_groups),
	.ngpios = 206,
	.egpio_func = 11,
};

static const struct of_device_id alor_tlmm_of_match[] = {
	{ .compatible = "qcom,alor-tlmm", .data = &alor_tlmm},
	{ .compatible = "qcom,alor-vm-tlmm", .data = &alor_vm_tlmm},
	{},
};

static int alor_tlmm_probe(struct platform_device *pdev)
{
	const struct msm_pinctrl_soc_data *pinctrl_data;
	struct device *dev = &pdev->dev;

	pinctrl_data = of_device_get_match_data(dev);
	if (!pinctrl_data)
		return -EINVAL;

	return msm_pinctrl_probe(pdev, pinctrl_data);
}

static struct platform_driver alor_tlmm_driver = {
	.driver = {
		.name = "alor-pinctrl",
		.of_match_table = alor_tlmm_of_match,
	},
	.probe = alor_tlmm_probe,
	.remove = msm_pinctrl_remove,
};

module_platform_driver(alor_tlmm_driver);

MODULE_DESCRIPTION("QTI alor TLMM driver");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(of, alor_tlmm_of_match);

#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-$(CONFIG_$(SPL_TPL_)MISC) += misc-uclass.o misc_otp.o
obj-$(CONFIG_$(SPL_TPL_)MISC_DECOMPRESS) += misc_decompress.o
obj-$(CONFIG_ALI152X) += ali512x.o
obj-$(CONFIG_ALTERA_SYSID) += altera_sysid.o
obj-$(CONFIG_ATSHA204A) += atsha204a-i2c.o
obj-$(CONFIG_DS4510)  += ds4510.o
obj-$(CONFIG_CBMEM_CONSOLE) += cbmem_console.o
ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_CROS_EC) += cros_ec.o
obj-$(CONFIG_CROS_EC_LPC) += cros_ec_lpc.o
obj-$(CONFIG_CROS_EC_I2C) += cros_ec_i2c.o
obj-$(CONFIG_CROS_EC_SANDBOX) += cros_ec_sandbox.o
obj-$(CONFIG_CROS_EC_SPI) += cros_ec_spi.o
obj-$(CONFIG_ROCKCHIP_PM_CONFIG) += rockchip_pm_config.o
endif
obj-$(CONFIG_FSL_IIM) += fsl_iim.o
obj-$(CONFIG_LED_STATUS_GPIO) += gpio_led.o
obj-$(CONFIG_$(SPL_TPL_)I2C_EEPROM) += i2c_eeprom.o
obj-$(CONFIG_FSL_MC9SDZ60) += mc9sdz60.o
obj-$(CONFIG_MXC_OCOTP) += mxc_ocotp.o
obj-$(CONFIG_MXS_OCOTP) += mxs_ocotp.o
obj-$(CONFIG_NUVOTON_NCT6102D) += nuvoton_nct6102d.o
obj-$(CONFIG_NS87308) += ns87308.o
obj-$(CONFIG_$(SPL_)PWRSEQ) += pwrseq-uclass.o
ifdef CONFIG_DM_I2C
ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_SANDBOX) += i2c_eeprom_emul.o
endif
endif
obj-$(CONFIG_SMSC_LPC47M) += smsc_lpc47m.o
obj-$(CONFIG_SMSC_SIO1007) += smsc_sio1007.o
obj-$(CONFIG_LED_STATUS) += status_led.o
obj-$(CONFIG_SANDBOX) += swap_case.o
ifdef CONFIG_SPL_OF_PLATDATA
ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SANDBOX) += spltest_sandbox.o
endif
endif
obj-$(CONFIG_SANDBOX) += syscon_sandbox.o
obj-$(CONFIG_TEGRA_CAR) += tegra_car.o
obj-$(CONFIG_TEGRA186_BPMP) += tegra186_bpmp.o
obj-$(CONFIG_TWL4030_LED) += twl4030_led.o
obj-$(CONFIG_FSL_IFC) += fsl_ifc.o
obj-$(CONFIG_FSL_SEC_MON) += fsl_sec_mon.o
obj-$(CONFIG_PCA9551_LED) += pca9551_led.o
obj-$(CONFIG_FSL_DEVICE_DISABLE) += fsl_devdis.o
obj-$(CONFIG_WINBOND_W83627) += winbond_w83627.o
obj-$(CONFIG_QFW) += qfw.o
obj-$(CONFIG_ROCKCHIP_EFUSE) += rockchip-efuse.o
obj-$(CONFIG_ROCKCHIP_OTP) += rockchip-otp.o

ifeq ($(CONFIG_$(SPL_TPL_)ROCKCHIP_SECURE_OTP),y)
obj-$(CONFIG_ROCKCHIP_RK3308) += rk3308-secure-otp.o
obj-$(CONFIG_ROCKCHIP_PX30)   += px30-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3328) += rk3328-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3568) += rk3568-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RV1103B) += rv1103b-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RV1106) += rv1106-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RV1126) += rv1126-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3506) += rk3506-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3588) += rk3588-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3528) += rk3528-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3562) += rk3562-secure-otp.o
obj-$(CONFIG_ROCKCHIP_RK3576) += rk3576-secure-otp.o
endif

obj-$(CONFIG_$(SPL_TPL_)ROCKCHIP_HW_DECOMPRESS) += rockchip_decompress.o

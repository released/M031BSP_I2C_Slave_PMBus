#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "pmbus/pmbus_cfg_common.h"

/*
    Default M031 board mapping for the PMBus framework.

    I2C0 pin assignment follows the M031 I2C slave sample:
    - PB.5 : I2C0_SCL
    - PB.4 : I2C0_SDA

    SMBALERT# and A0/A1 are placeholder defaults for this project.
    Update them here when moving to the real target board.
*/

#define PMBUS_DEFAULT_ADDRESS_A0_LEVEL        0U
#define PMBUS_DEFAULT_ADDRESS_A1_LEVEL        1U
#define PMBUS_PORT_I2C_INSTANCE                     I2C0
#define PMBUS_PORT_I2C_MODULE                       I2C0_MODULE
#define PMBUS_PORT_I2C_IRQn                         I2C0_IRQn
#define PMBUS_PORT_I2C_IRQHandler                   I2C0_IRQHandler
#define PMBUS_PORT_I2C_ISR_PROTOTYPE                void PMBUS_PORT_I2C_IRQHandler(void)
#define PMBUS_PORT_I2C_BUS_CLOCK                    400000UL

#define PMBUS_ADDRESS_STRAP_USE_GPIO                0U

#define PMBUS_DEBUG_PRINT                           printf

#define PMBUS_PORT_SET_I2C_PINS_MFP()              do { \
                                                        SYS_UnlockReg(); \
                                                        CLK_EnableModuleClock(PMBUS_PORT_I2C_MODULE); \
                                                        SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk)) | \
                                                                        (SYS_GPB_MFPL_PB5MFP_I2C0_SCL | SYS_GPB_MFPL_PB4MFP_I2C0_SDA); \
                                                        SYS_LockReg(); \
                                                    } while (0)

#define PMBUS_PORT_SET_I2C_PINS_GPIO()             do { \
                                                        SYS_UnlockReg(); \
                                                        SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk)) | \
                                                                        (SYS_GPB_MFPL_PB5MFP_GPIO | SYS_GPB_MFPL_PB4MFP_GPIO); \
                                                        SYS_LockReg(); \
                                                    } while (0)

#define PMBUS_PORT_INIT_I2C_PINS()                 do { \
                                                        PMBUS_PORT_SET_I2C_PINS_MFP(); \
                                                        GPIO_SetMode(PB, BIT5 | BIT4, GPIO_MODE_OPEN_DRAIN); \
                                                        PB5 = 1; \
                                                        PB4 = 1; \
                                                    } while (0)

#define PMBUS_PORT_READ_SCL()                      (PB5)
#define PMBUS_PORT_READ_SDA()                      (PB4)
#define PMBUS_PORT_PREPARE_BUS_GPIO()              do { \
                                                        PMBUS_PORT_SET_I2C_PINS_GPIO(); \
                                                        GPIO_SetMode(PB, BIT5 | BIT4, GPIO_MODE_OPEN_DRAIN); \
                                                        PB5 = 1; \
                                                        PB4 = 1; \
                                                    } while (0)
#define PMBUS_PORT_DRIVE_SCL_LOW()                 do { PB5 = 0; } while (0)
#define PMBUS_PORT_DRIVE_SCL_HIGH()                do { PB5 = 1; } while (0)
#define PMBUS_PORT_DRIVE_SDA_HIGH()                do { PB4 = 1; } while (0)

#define PMBUS_PORT_INIT_ALERT_PIN()                do { \
                                                        GPIO_SetMode(PB, BIT6, GPIO_MODE_OPEN_DRAIN); \
                                                        PB6 = 1; \
                                                    } while (0)
#define PMBUS_PORT_ALERT_ASSERT()                  do { PB6 = 0; } while (0)
#define PMBUS_PORT_ALERT_RELEASE()                 do { PB6 = 1; } while (0)

#if PMBUS_ADDRESS_STRAP_USE_GPIO
#define PMBUS_PORT_INIT_ADDRESS_PINS()             do { \
                                                        GPIO_SetMode(PC, BIT0 | BIT1, GPIO_MODE_INPUT); \
                                                    } while (0)
#define PMBUS_PORT_READ_ADDRESS_A0()               (PC0)
#define PMBUS_PORT_READ_ADDRESS_A1()               (PC1)
#else
#define PMBUS_PORT_INIT_ADDRESS_PINS()             do { } while (0)
#define PMBUS_PORT_READ_ADDRESS_A0()               (PMBUS_DEFAULT_ADDRESS_A0_LEVEL)
#define PMBUS_PORT_READ_ADDRESS_A1()               (PMBUS_DEFAULT_ADDRESS_A1_LEVEL)
#endif

#endif

/**
 ******************************************************************************
 * @file    Bl_DriverAdapter.c
 * @author  <author_name>
 * @version V1.0.0
 * @date    2026-08-09
 * @brief   Bl_DriverAdapter module source file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-09  [New] Module created
 *                       [New] s_Bl_DriverAdapter_CddInit added for CDD init
 *                       [New] s_Bl_DriverAdapter_TimerInit added for Timer init
 *                       [New] TIM1 update interrupt init
 *                       [New] Deinit added for module de-initialization
 *                       [Modify] TIM1 callback migrated to Bl_Isr
 *                       [New] Can init/deinit added for Bl_Can module
 *                       [New] Bl_DriverAdapter_SystemReset added: adapter-layer
 *                       system reset for UDS 0x11 (maps reset types to
 *                       NVIC_SystemReset on STM32F1)
 *                       [New] Fls init added for the AUTOSAR Bl_Fls driver
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_DriverAdapter.h"
#include "Bl_Can.h"
#include "Bl_Fls.h"
#include "main.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  init all CDD modules
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CddInit(void);

/**
 * @brief  init timer peripherals
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_TimerInit(void);

/**
 * @brief  deinit CDD modules
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CddDeinit(void);

/**
 * @brief  deinit timer peripherals
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_TimerDeinit(void);

/**
 * @brief  init CAN driver
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CanInit(void);

/**
 * @brief  deinit CAN driver
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CanDeinit(void);

/**
 * @brief  init Fls driver (AUTOSAR internal flash)
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_FlsInit(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  module init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_DriverAdapter_Init(void)
{
    bl_ret_t e_Ret;

    e_Ret  = s_Bl_DriverAdapter_CddInit();
    e_Ret |= s_Bl_DriverAdapter_TimerInit();
    e_Ret |= s_Bl_DriverAdapter_CanInit();
    e_Ret |= s_Bl_DriverAdapter_FlsInit();

    return e_Ret;
}

/**
 * @brief  module deinit
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_DriverAdapter_Deinit(void)
{
    bl_ret_t e_Ret;

    e_Ret  = s_Bl_DriverAdapter_CddDeinit();
    e_Ret |= s_Bl_DriverAdapter_TimerDeinit();
    e_Ret |= s_Bl_DriverAdapter_CanDeinit();

    return e_Ret;
}

/**
 * @brief  system reset through the adapter layer (chip-specific)
 * @param  u8_ResetType : reset type (0x01 hard / 0x02 key-off-on /
 *                        0x03 soft / 0x04 fast-soft)
 * @retval BL_E_OK     : reset issued (never returns on success)
 * @retval BL_E_NOT_OK : reset type not supported
 */
bl_ret_t Bl_DriverAdapter_SystemReset(bl_uint8_t u8_ResetType)
{
    switch (u8_ResetType)
    {
    case 0x01U:     /* hard reset */
    case 0x02U:     /* key off/on reset (simulated: no power control on this HW) */
    case 0x03U:     /* soft reset */
    case 0x04U:     /* fast soft reset */
        /* User Add Begin */
        NVIC_SystemReset();
        /* User Add End */
        break;

    default:
        return BL_E_NOT_OK;
    }

    /* NVIC_SystemReset() never returns on success; trap here to keep the
       compiler honest and catch a disabled/waiting reset */
    while (1)
    {
    }
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  init all CDD modules
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CddInit(void)
{
    /* User Add Begin */
    OLED_Init();
    /* User Add End */

    return BL_E_OK;
}

/**
 * @brief  init timer peripherals
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_TimerInit(void)
{
    /* User Add Begin */
    HAL_TIM_Base_Start_IT(&htim1);
    /* User Add End */

    return BL_E_OK;
}

/**
 * @brief  deinit CDD modules
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CddDeinit(void)
{
    /* User Add Begin */
    /* TODO: add CDD deinit code here */
    /* User Add End */

    return BL_E_OK;
}

/**
 * @brief  deinit timer peripherals
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_TimerDeinit(void)
{
    /* User Add Begin */
    HAL_TIM_Base_Stop_IT(&htim1);
    /* User Add End */

    return BL_E_OK;
}

/**
 * @brief  init CAN driver
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CanInit(void)
{
    /* User Add Begin */
    return Bl_Can_Init();
    /* User Add End */
}

/**
 * @brief  deinit CAN driver
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_CanDeinit(void)
{
    /* User Add Begin */
    return Bl_Can_DeInit();
    /* User Add End */
}

/**
 * @brief  init Fls driver (AUTOSAR internal flash)
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_DriverAdapter_FlsInit(void)
{
    /* User Add Begin */
    return Bl_Fls_Init();
    /* User Add End */
}

/******************************* EOF (End of File) ***************************/

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
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_DriverAdapter.h"
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

    return e_Ret;
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

/******************************* EOF (End of File) ***************************/

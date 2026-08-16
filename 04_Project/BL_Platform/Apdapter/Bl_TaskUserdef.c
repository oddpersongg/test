/**
 ******************************************************************************
 * @file    Bl_TaskUserdef.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_TaskUserdef module source file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, user application tasks: 1s uptime
 *                       counter only. CAN send/statistics test tasks (KEY0/KEY1)
 *                       and the Bl_CanIf_RxIndicationApp / _TxConfirmationApp
 *                       overrides are removed — the CanIf hooks are back to
 *                       pure weak no-ops in Core, ready for the CanTp layer to
 *                       take over the RX/TX path
 *                       [Remove] one-shot flash test task (erased the app-area
 *                       first sector on every boot) removed after the
 *                       Bl_Fls / Bl_FlashIf bring-up test passed on hardware
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_TaskUserdef.h"
#include "Bl_TaskSchedule.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                   Static Variables
 ***************************************************************/

static bl_uint32_t s_Bl_TaskUserdef_Counter = 0U;  /**< 1s uptime counter */
static bl_uint8_t  s_Bl_TaskUserdef_CounterTaskId = BL_TASKSCHEDULE_INVALID_ID;  /**< uptime counter task ID */

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

static void s_Bl_TaskUserdef_CounterTask(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  user-defined task init, register application tasks
 * @param  None
 * @retval None
 */
void Bl_TaskUserdef_Init(void)
{
    /* User Add Begin */
    s_Bl_TaskUserdef_CounterTaskId = Bl_TaskSchedule_RegisterTaskInfinite(s_Bl_TaskUserdef_CounterTask, 1000U);
    /* User Add End */
}

/**
 * @brief  user-defined task deinit, unregister all application tasks
 * @param  None
 * @retval None
 */
void Bl_TaskUserdef_Deinit(void)
{
    /* User Add Begin */
    if (s_Bl_TaskUserdef_CounterTaskId != BL_TASKSCHEDULE_INVALID_ID)
    {
        Bl_TaskSchedule_UnregisterTask(s_Bl_TaskUserdef_CounterTaskId);
        s_Bl_TaskUserdef_CounterTaskId = BL_TASKSCHEDULE_INVALID_ID;
    }
    /* User Add End */
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  1s uptime counter task (increments only, no I/O)
 * @param  None
 * @retval None
 */
static void s_Bl_TaskUserdef_CounterTask(void)
{
    s_Bl_TaskUserdef_Counter++;
}

/******************************* EOF (End of File) ***************************/

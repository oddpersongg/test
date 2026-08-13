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
 * V1.0.0   2026-08-12   [New] module created, basic functionality implemented
 *                       [New] Bl_TaskUserdef_Deinit added to clear all tasks
 *                       [Modify] KEY0 removes OLED task, KEY1 restores OLED task
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_TaskUserdef.h"
#include "Bl_TaskSchedule.h"
#include "OLED.h"
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

static bl_uint32_t s_Bl_TaskUserdef_Counter = 0U;  /**< 1s counter for OLED display */
static bl_uint8_t  s_Bl_TaskUserdef_CounterTaskId = BL_TASKSCHEDULE_INVALID_ID;  /**< OLED counter task ID */
static bl_uint8_t  s_Bl_TaskUserdef_Key0TaskId = BL_TASKSCHEDULE_INVALID_ID;     /**< KEY0 monitor task ID */
static bl_uint8_t  s_Bl_TaskUserdef_Key1TaskId = BL_TASKSCHEDULE_INVALID_ID;     /**< KEY1 monitor task ID */

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

static void s_Bl_TaskUserdef_CounterTask(void);
static void s_Bl_TaskUserdef_Key0MonitorTask(void);
static void s_Bl_TaskUserdef_Key1MonitorTask(void);

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
    s_Bl_TaskUserdef_Key0TaskId    = Bl_TaskSchedule_RegisterTaskInfinite(s_Bl_TaskUserdef_Key0MonitorTask, 10U);
    s_Bl_TaskUserdef_Key1TaskId    = Bl_TaskSchedule_RegisterTaskInfinite(s_Bl_TaskUserdef_Key1MonitorTask, 10U);
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

    if (s_Bl_TaskUserdef_Key0TaskId != BL_TASKSCHEDULE_INVALID_ID)
    {
        Bl_TaskSchedule_UnregisterTask(s_Bl_TaskUserdef_Key0TaskId);
        s_Bl_TaskUserdef_Key0TaskId = BL_TASKSCHEDULE_INVALID_ID;
    }

    if (s_Bl_TaskUserdef_Key1TaskId != BL_TASKSCHEDULE_INVALID_ID)
    {
        Bl_TaskSchedule_UnregisterTask(s_Bl_TaskUserdef_Key1TaskId);
        s_Bl_TaskUserdef_Key1TaskId = BL_TASKSCHEDULE_INVALID_ID;
    }
    /* User Add End */
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  1s counter task, increment and display on OLED line 2
 * @param  None
 * @retval None
 */
static void s_Bl_TaskUserdef_CounterTask(void)
{
    s_Bl_TaskUserdef_Counter++;
    OLED_ShowNum(2, 1, s_Bl_TaskUserdef_Counter, 8);
}

/**
 * @brief  KEY0 (PD10) monitor, remove OLED task when pressed
 * @param  None
 * @retval None
 */
static void s_Bl_TaskUserdef_Key0MonitorTask(void)
{
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10) == GPIO_PIN_RESET)
    {
        if (s_Bl_TaskUserdef_CounterTaskId != BL_TASKSCHEDULE_INVALID_ID)
        {
            Bl_TaskSchedule_UnregisterTask(s_Bl_TaskUserdef_CounterTaskId);
            s_Bl_TaskUserdef_CounterTaskId = BL_TASKSCHEDULE_INVALID_ID;
        }
    }
}

/**
 * @brief  KEY1 (PD9) monitor, restore OLED task when pressed
 * @param  None
 * @retval None
 */
static void s_Bl_TaskUserdef_Key1MonitorTask(void)
{
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_9) == GPIO_PIN_RESET)
    {
        if (s_Bl_TaskUserdef_CounterTaskId == BL_TASKSCHEDULE_INVALID_ID)
        {
            s_Bl_TaskUserdef_CounterTaskId = Bl_TaskSchedule_RegisterTaskInfinite(s_Bl_TaskUserdef_CounterTask, 1000U);
        }
    }
}

/******************************* EOF (End of File) ***************************/

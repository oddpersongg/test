/**
 ******************************************************************************
 * @file    Bl_TaskSchedule.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_TaskSchedule module source file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-12   [New] module created, basic functionality implemented
 *                       [New] RegisterTaskInfinite added for infinite-period tasks
 *                       [Modify] Init reads config from Lcfg const struct
 *                       [Modify] MainFunction delegate to s_Bl_TaskSchedule_Process
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_TaskSchedule.h"
#include "Bl_TaskSchedule_Lcfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief private task runtime data
 */
typedef struct {
    Bl_TaskSchedule_TaskFunc_t p_Func;       /**< task function pointer        */
    bl_uint16_t                u16_PeriodMs; /**< task period in ms            */
    bl_uint16_t                u16_RepeatCnt;/**< remaining execution count    */
    bl_uint32_t                u32_LastTickMs;/**< last execution tick stamp   */
    bl_uint8_t                 u8_IsUsed;    /**< 1=slot occupied, 0=free     */
} Bl_TaskSchedule_PrivTask_t;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

static Bl_TaskSchedule_PrivTask_t s_Bl_TaskSchedule_TaskTable[BL_TASKSCHEDULE_MAX_TASKS]; /**< task table              */
static bl_uint8_t                 s_Bl_TaskSchedule_MaxTasks = 0U;                         /**< active task count      */
static bl_uint16_t                s_Bl_TaskSchedule_TickPeriodMs = 1U;                     /**< tick period in ms      */
static volatile bl_uint32_t       s_Bl_TaskSchedule_TickMs = 0U;                           /**< system tick counter    */
static bl_uint8_t                 s_Bl_TaskSchedule_Ready = 0U;                            /**< init complete flag     */

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  process all tasks in one scheduler tick
 * @param  None
 * @retval None
 */
static void s_Bl_TaskSchedule_Process(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize task scheduler
 * @param  p_Config : scheduler config pointer (NULL = ignored)
 * @retval None
 */
void Bl_TaskSchedule_Init(void)
{
    bl_uint8_t i;

    /* clamp max tasks to supported limit */
    s_Bl_TaskSchedule_MaxTasks = g_Bl_TaskSchedule_Config.u8_MaxTasks;
    if (s_Bl_TaskSchedule_MaxTasks > BL_TASKSCHEDULE_MAX_TASKS)
    {
        s_Bl_TaskSchedule_MaxTasks = BL_TASKSCHEDULE_MAX_TASKS;
    }

    s_Bl_TaskSchedule_TickPeriodMs = g_Bl_TaskSchedule_Config.u16_TickPeriodMs;
    s_Bl_TaskSchedule_TickMs       = 0U;

    /* clear task table */
    for (i = 0U; i < s_Bl_TaskSchedule_MaxTasks; i++)
    {
        s_Bl_TaskSchedule_TaskTable[i].p_Func        = BL_NULL_PTR;
        s_Bl_TaskSchedule_TaskTable[i].u16_PeriodMs   = 0U;
        s_Bl_TaskSchedule_TaskTable[i].u16_RepeatCnt  = 0U;
        s_Bl_TaskSchedule_TaskTable[i].u32_LastTickMs = 0U;
        s_Bl_TaskSchedule_TaskTable[i].u8_IsUsed      = 0U;
    }

    s_Bl_TaskSchedule_Ready = 1U;
}

/**
 * @brief  register a task into scheduler
 * @param  p_TaskAttr : task attribute pointer (func, period, repeat count)
 * @retval task ID (0 .. maxTasks-1) on success
 * @retval BL_TASKSCHEDULE_INVALID_ID on failure
 */
bl_uint8_t Bl_TaskSchedule_RegisterTask(const Bl_TaskSchedule_TaskAttr_t *p_TaskAttr)
{
    bl_uint8_t i;

    if ((s_Bl_TaskSchedule_Ready == 0U) ||
        (p_TaskAttr == BL_NULL_PTR) ||
        (p_TaskAttr->p_Func == BL_NULL_PTR))
    {
        return BL_TASKSCHEDULE_INVALID_ID;
    }

    /* find a free slot */
    for (i = 0U; i < s_Bl_TaskSchedule_MaxTasks; i++)
    {
        if (s_Bl_TaskSchedule_TaskTable[i].u8_IsUsed == 0U)
        {
            s_Bl_TaskSchedule_TaskTable[i].p_Func        = p_TaskAttr->p_Func;
            s_Bl_TaskSchedule_TaskTable[i].u16_PeriodMs   = p_TaskAttr->u16_PeriodMs;
            s_Bl_TaskSchedule_TaskTable[i].u16_RepeatCnt  = p_TaskAttr->u16_RepeatCnt;
            s_Bl_TaskSchedule_TaskTable[i].u32_LastTickMs = s_Bl_TaskSchedule_TickMs;
            s_Bl_TaskSchedule_TaskTable[i].u8_IsUsed      = 1U;
            return i;
        }
    }

    return BL_TASKSCHEDULE_INVALID_ID;
}

/**
 * @brief  register a task that runs infinitely until manually cancelled
 * @param  p_Func       : task function pointer
 * @param  u16_PeriodMs : task period in ms
 * @retval task ID (0 .. maxTasks-1) on success
 * @retval BL_TASKSCHEDULE_INVALID_ID on failure
 */
bl_uint8_t Bl_TaskSchedule_RegisterTaskInfinite(Bl_TaskSchedule_TaskFunc_t p_Func,
                                                 bl_uint16_t                u16_PeriodMs)
{
    Bl_TaskSchedule_TaskAttr_t s_TaskAttr;

    if (p_Func == BL_NULL_PTR)
    {
        return BL_TASKSCHEDULE_INVALID_ID;
    }

    s_TaskAttr.p_Func        = p_Func;
    s_TaskAttr.u16_PeriodMs  = u16_PeriodMs;
    s_TaskAttr.u16_RepeatCnt = BL_TASKSCHEDULE_REPEAT_INFINITE;

    return Bl_TaskSchedule_RegisterTask(&s_TaskAttr);
}

/**
 * @brief  unregister a task from scheduler
 * @param  u8_TaskId : task ID returned by RegisterTask
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure (invalid ID or not in use)
 */
bl_ret_t Bl_TaskSchedule_UnregisterTask(bl_uint8_t u8_TaskId)
{
    if ((s_Bl_TaskSchedule_Ready == 0U) ||
        (u8_TaskId >= s_Bl_TaskSchedule_MaxTasks))
    {
        return BL_E_NOT_OK;
    }

    if (s_Bl_TaskSchedule_TaskTable[u8_TaskId].u8_IsUsed == 0U)
    {
        return BL_E_NOT_OK;
    }

    s_Bl_TaskSchedule_TaskTable[u8_TaskId].u8_IsUsed = 0U;
    s_Bl_TaskSchedule_TaskTable[u8_TaskId].p_Func    = BL_NULL_PTR;

    return BL_E_OK;
}

/**
 * @brief  tick increment, call from SysTick or timer ISR
 * @param  None
 * @retval None
 */
void Bl_TaskSchedule_TickInc(void)
{
    if (s_Bl_TaskSchedule_Ready != 0U)
    {
        s_Bl_TaskSchedule_TickMs += s_Bl_TaskSchedule_TickPeriodMs;
    }
}

/**
 * @brief  scheduler main loop, never returns
 * @param  None
 * @retval None
 */
void Bl_TaskSchedule_MainFunction(void)
{
    while (1)
    {
        s_Bl_TaskSchedule_Process();
    }
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  process all tasks in one scheduler tick
 * @param  None
 * @retval None
 */
static void s_Bl_TaskSchedule_Process(void)
{
    bl_uint8_t  i;
    bl_uint32_t u32_CurrentTickMs;
    bl_uint32_t u32_ElapsedMs;

    if (s_Bl_TaskSchedule_Ready == 0U)
    {
        return;
    }

    u32_CurrentTickMs = s_Bl_TaskSchedule_TickMs;

    for (i = 0U; i < s_Bl_TaskSchedule_MaxTasks; i++)
    {
        if (s_Bl_TaskSchedule_TaskTable[i].u8_IsUsed == 0U)
        {
            continue;
        }

        u32_ElapsedMs = u32_CurrentTickMs - s_Bl_TaskSchedule_TaskTable[i].u32_LastTickMs;

        if (u32_ElapsedMs >= s_Bl_TaskSchedule_TaskTable[i].u16_PeriodMs)
        {
            s_Bl_TaskSchedule_TaskTable[i].u32_LastTickMs = u32_CurrentTickMs;

            /* execute task function */
            if (s_Bl_TaskSchedule_TaskTable[i].p_Func != BL_NULL_PTR)
            {
                s_Bl_TaskSchedule_TaskTable[i].p_Func();
            }

            /* decrement repeat count, auto-remove when exhausted */
            if (s_Bl_TaskSchedule_TaskTable[i].u16_RepeatCnt > 0U)
            {
                s_Bl_TaskSchedule_TaskTable[i].u16_RepeatCnt--;
                if (s_Bl_TaskSchedule_TaskTable[i].u16_RepeatCnt == 0U)
                {
                    s_Bl_TaskSchedule_TaskTable[i].u8_IsUsed = 0U;
                    s_Bl_TaskSchedule_TaskTable[i].p_Func    = BL_NULL_PTR;
                }
            }
        }
    }
}

/******************************* EOF (End of File) ***************************/

/**
 ******************************************************************************
 * @file    Bl_TaskSchedule.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_TaskSchedule module header file
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
 *                       [Modify] Init takes no param, reads from Lcfg const struct
 *                       [Modify] BL_TASKSCHEDULE_MAX_TASKS moved to Cfg.h
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_TASKSCHEDULE_H__
#define __BL_TASKSCHEDULE_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_TaskSchedule_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief module version */
#define BL_TASKSCHEDULE_VERSION_MAJOR    1
#define BL_TASKSCHEDULE_VERSION_MINOR    0
#define BL_TASKSCHEDULE_VERSION_PATCH    0

/** @brief invalid task ID returned on registration failure */
#define BL_TASKSCHEDULE_INVALID_ID       0xFFU

/** @brief infinite repeat count */
#define BL_TASKSCHEDULE_REPEAT_INFINITE  0U

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief task function pointer type
 */
typedef void (*Bl_TaskSchedule_TaskFunc_t)(void);

/**
 * @brief task attribute type, user fills this to register a task
 */
typedef struct {
    Bl_TaskSchedule_TaskFunc_t p_Func;       /**< task function pointer        */
    bl_uint16_t                u16_PeriodMs; /**< task period in ms            */
    bl_uint16_t                u16_RepeatCnt;/**< 0=infinite, N=run N times   */
} Bl_TaskSchedule_TaskAttr_t;

/**
 * @brief scheduler config type
 */
typedef struct {
    bl_uint8_t  u8_MaxTasks;      /**< max task count, <= BL_TASKSCHEDULE_MAX_TASKS */
    bl_uint16_t u16_TickPeriodMs; /**< tick period in ms, typically 1ms             */
} Bl_TaskSchedule_Config_t;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize task scheduler, reads config from Lcfg const struct
 * @param  None
 * @retval None
 */
void Bl_TaskSchedule_Init(void);

/**
 * @brief  register a task into scheduler
 * @param  p_TaskAttr : task attribute pointer (func, period, repeat count)
 * @retval task ID (0 .. maxTasks-1) on success
 * @retval BL_TASKSCHEDULE_INVALID_ID on failure (table full or invalid param)
 */
bl_uint8_t Bl_TaskSchedule_RegisterTask(const Bl_TaskSchedule_TaskAttr_t *p_TaskAttr);

/**
 * @brief  register a task that runs infinitely until manually cancelled
 * @param  p_Func       : task function pointer
 * @param  u16_PeriodMs : task period in ms
 * @retval task ID (0 .. maxTasks-1) on success
 * @retval BL_TASKSCHEDULE_INVALID_ID on failure (table full or invalid param)
 */
bl_uint8_t Bl_TaskSchedule_RegisterTaskInfinite(Bl_TaskSchedule_TaskFunc_t p_Func,
                                                 bl_uint16_t                u16_PeriodMs);

/**
 * @brief  unregister a task from scheduler
 * @param  u8_TaskId : task ID returned by RegisterTask
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure (invalid ID or not in use)
 */
bl_ret_t Bl_TaskSchedule_UnregisterTask(bl_uint8_t u8_TaskId);

/**
 * @brief  tick increment, call from SysTick ISR or timer ISR
 * @param  None
 * @retval None
 */
void Bl_TaskSchedule_TickInc(void);

/**
 * @brief  scheduler main loop, never returns
 * @param  None
 * @retval None
 */
void Bl_TaskSchedule_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_TASKSCHEDULE_H__ */

/******************************* EOF (End of File) ***************************/

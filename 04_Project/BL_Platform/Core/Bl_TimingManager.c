/**
 ******************************************************************************
 * @file    Bl_TimingManager.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_TimingManager module source file
 *          (centralized protocol timing service)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, generic software timer service:
 *                       per-protocol timeout values from Bl_TimingManager_Cfg.h,
 *                       tick source Bl_TaskSchedule_GetTickMs, 32-bit wrap-safe
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_TimingManager.h"
#include "Bl_TaskSchedule.h"    /* system tick (Bl_TaskSchedule_GetTickMs) */

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/** @brief per-timer runtime state */
typedef struct
{
    bl_uint32_t u32_StartTick;      /**< tick when the timer was (re)started    */
    bl_uint32_t u32_TimeoutMs;      /**< configured timeout from the Cfg table  */
    bl_uint8_t  u8_Running;         /**< 1 = running, 0 = stopped              */
} Bl_TimingManager_TimerState_t;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief configured timeout per timer id (order must match the enum) */
static const bl_uint32_t s_Bl_TimingManager_TimeoutMs[BL_TIMINGMANAGER_TIMER_CNT] =
{
    BL_TIMINGMANAGER_S3_TIMEOUT_MS,     /* BL_TIMINGMANAGER_TIMER_S3     */
    BL_TIMINGMANAGER_P2_TIMEOUT_MS,     /* BL_TIMINGMANAGER_TIMER_P2     */
    BL_TIMINGMANAGER_P2STAR_TIMEOUT_MS, /* BL_TIMINGMANAGER_TIMER_P2STAR */
    BL_TIMINGMANAGER_N_AS_TIMEOUT_MS,   /* BL_TIMINGMANAGER_TIMER_N_AS   */
    BL_TIMINGMANAGER_N_BS_TIMEOUT_MS,   /* BL_TIMINGMANAGER_TIMER_N_BS   */
    BL_TIMINGMANAGER_N_CR_TIMEOUT_MS    /* BL_TIMINGMANAGER_TIMER_N_CR   */
};

/** @brief runtime state of all timers */
static Bl_TimingManager_TimerState_t s_Bl_TimingManager_Timers[BL_TIMINGMANAGER_TIMER_CNT];

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  check if a timer id is valid
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : valid
 * @retval BL_E_NOT_OK : out of range
 */
static bl_ret_t s_Bl_TimingManager_CheckId(Bl_TimingManager_TimerId_t e_TimerId);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  init all timers to stopped state
 * @param  None
 * @retval BL_E_OK     : success
 */
bl_ret_t Bl_TimingManager_Init(void)
{
    bl_uint32_t u32_Idx;

    for (u32_Idx = 0U; u32_Idx < BL_TIMINGMANAGER_TIMER_CNT; u32_Idx++)
    {
        s_Bl_TimingManager_Timers[u32_Idx].u32_StartTick = 0U;
        s_Bl_TimingManager_Timers[u32_Idx].u32_TimeoutMs =
            s_Bl_TimingManager_TimeoutMs[u32_Idx];
        s_Bl_TimingManager_Timers[u32_Idx].u8_Running = 0U;
    }

    return BL_E_OK;
}

/**
 * @brief  start (or restart) a protocol timer
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : invalid timer id
 */
bl_ret_t Bl_TimingManager_Start(Bl_TimingManager_TimerId_t e_TimerId)
{
    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return BL_E_NOT_OK;
    }

    s_Bl_TimingManager_Timers[e_TimerId].u32_StartTick = Bl_TaskSchedule_GetTickMs();
    s_Bl_TimingManager_Timers[e_TimerId].u8_Running    = 1U;

    return BL_E_OK;
}

/**
 * @brief  stop a protocol timer
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : invalid timer id
 */
bl_ret_t Bl_TimingManager_Stop(Bl_TimingManager_TimerId_t e_TimerId)
{
    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return BL_E_NOT_OK;
    }

    s_Bl_TimingManager_Timers[e_TimerId].u8_Running = 0U;

    return BL_E_OK;
}

/**
 * @brief  check if a timer is running
 * @param  e_TimerId: timer identifier
 * @retval 1 : running
 * @retval 0 : stopped or invalid id
 */
bl_uint8_t Bl_TimingManager_IsRunning(Bl_TimingManager_TimerId_t e_TimerId)
{
    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return 0U;
    }

    return s_Bl_TimingManager_Timers[e_TimerId].u8_Running;
}

/**
 * @brief  check if a running timer has expired (wrap-safe subtraction)
 * @param  e_TimerId: timer identifier
 * @retval 1 : expired
 * @retval 0 : running, stopped, or invalid id
 */
bl_uint8_t Bl_TimingManager_IsExpired(Bl_TimingManager_TimerId_t e_TimerId)
{
    bl_uint32_t u32_Elapsed;

    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return 0U;
    }

    if (s_Bl_TimingManager_Timers[e_TimerId].u8_Running == 0U)
    {
        return 0U;
    }

    u32_Elapsed = Bl_TaskSchedule_GetTickMs() - s_Bl_TimingManager_Timers[e_TimerId].u32_StartTick;
    if (u32_Elapsed >= s_Bl_TimingManager_Timers[e_TimerId].u32_TimeoutMs)
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief  get remaining time of a running timer
 * @param  e_TimerId: timer identifier
 * @retval remaining ms (0 if stopped/expired/invalid)
 */
bl_uint32_t Bl_TimingManager_GetRemainingMs(Bl_TimingManager_TimerId_t e_TimerId)
{
    bl_uint32_t u32_Elapsed;

    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return 0U;
    }

    if (s_Bl_TimingManager_Timers[e_TimerId].u8_Running == 0U)
    {
        return 0U;
    }

    u32_Elapsed = Bl_TaskSchedule_GetTickMs() - s_Bl_TimingManager_Timers[e_TimerId].u32_StartTick;
    if (u32_Elapsed >= s_Bl_TimingManager_Timers[e_TimerId].u32_TimeoutMs)
    {
        return 0U;
    }

    return (s_Bl_TimingManager_Timers[e_TimerId].u32_TimeoutMs - u32_Elapsed);
}

/**
 * @brief  get configured timeout of a timer
 * @param  e_TimerId: timer identifier
 * @retval timeout ms (0 if invalid)
 */
bl_uint32_t Bl_TimingManager_GetTimeoutMs(Bl_TimingManager_TimerId_t e_TimerId)
{
    if (s_Bl_TimingManager_CheckId(e_TimerId) != BL_E_OK)
    {
        return 0U;
    }

    return s_Bl_TimingManager_Timers[e_TimerId].u32_TimeoutMs;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  check if a timer id is valid
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : valid
 * @retval BL_E_NOT_OK : out of range
 */
static bl_ret_t s_Bl_TimingManager_CheckId(Bl_TimingManager_TimerId_t e_TimerId)
{
    if (e_TimerId >= BL_TIMINGMANAGER_TIMER_CNT)
    {
        return BL_E_NOT_OK;
    }

    return BL_E_OK;
}

/******************************* EOF (End of File) ***************************/

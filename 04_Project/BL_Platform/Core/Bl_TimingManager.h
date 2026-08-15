/**
 ******************************************************************************
 * @file    Bl_TimingManager.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_TimingManager module header file
 *          (centralized protocol timing: UDS S3/P2/P2*, CanTp N_As/N_Bs/N_Cr)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, generic software timer service
 *                       over Bl_TaskSchedule_GetTickMs with per-protocol
 *                       timeout values from Bl_TimingManager_Cfg.h
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_TIMINGMANAGER_H__
#define __BL_TIMINGMANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_TimingManager_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/** @brief protocol timer identifiers (index into the config table) */
typedef enum
{
    BL_TIMINGMANAGER_TIMER_S3 = 0,       /**< UDS session timeout                  */
    BL_TIMINGMANAGER_TIMER_P2,           /**< UDS server response time             */
    BL_TIMINGMANAGER_TIMER_P2STAR,       /**< UDS slow-service response time       */
    BL_TIMINGMANAGER_TIMER_N_AS,         /**< CanTp TX frame confirm               */
    BL_TIMINGMANAGER_TIMER_N_BS,         /**< CanTp wait FC                        */
    BL_TIMINGMANAGER_TIMER_N_CR,         /**< CanTp wait next CF                   */
    BL_TIMINGMANAGER_TIMER_CNT           /**< must equal BL_TIMINGMANAGER_TIMER_CNT */
} Bl_TimingManager_TimerId_t;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  init all timers to stopped state
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_TimingManager_Init(void);

/**
 * @brief  start (or restart) a protocol timer with its configured timeout
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : invalid timer id
 */
bl_ret_t Bl_TimingManager_Start(Bl_TimingManager_TimerId_t e_TimerId);

/**
 * @brief  stop a protocol timer
 * @param  e_TimerId: timer identifier
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : invalid timer id
 */
bl_ret_t Bl_TimingManager_Stop(Bl_TimingManager_TimerId_t e_TimerId);

/**
 * @brief  check if a protocol timer is currently running
 * @param  e_TimerId: timer identifier
 * @retval 1 : running
 * @retval 0 : stopped or invalid id
 */
bl_uint8_t Bl_TimingManager_IsRunning(Bl_TimingManager_TimerId_t e_TimerId);

/**
 * @brief  check if a running protocol timer has expired
 *         (never expires while stopped; safe across tick wrap-around)
 * @param  e_TimerId: timer identifier
 * @retval 1 : expired
 * @retval 0 : running, stopped, or invalid id
 */
bl_uint8_t Bl_TimingManager_IsExpired(Bl_TimingManager_TimerId_t e_TimerId);

/**
 * @brief  get remaining time of a running timer (ms); 0 if stopped/expired
 * @param  e_TimerId: timer identifier
 * @retval remaining milliseconds
 */
bl_uint32_t Bl_TimingManager_GetRemainingMs(Bl_TimingManager_TimerId_t e_TimerId);

/**
 * @brief  get configured timeout of a timer (ms); 0 if invalid id
 * @param  e_TimerId: timer identifier
 * @retval timeout milliseconds
 */
bl_uint32_t Bl_TimingManager_GetTimeoutMs(Bl_TimingManager_TimerId_t e_TimerId);

#ifdef __cplusplus
}
#endif

#endif /* __BL_TIMINGMANAGER_H__ */

/******************************* EOF (End of File) ***************************/

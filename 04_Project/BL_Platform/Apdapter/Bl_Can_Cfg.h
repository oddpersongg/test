/**
 ******************************************************************************
 * @file    Bl_Can_Cfg.h
 * @author  LENOVO
 * @version V1.0.0
 * @date    2026-08-13
 * @brief   Bl_Can pre-compile configuration header file (adapter-specific)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, pre-compile config (chip-specific):
 *                       TX queue 8, RX ring 128, filter banks 14, bus-off
 *                       software recovery (enable + recovery time)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CAN_CFG_H__
#define __BL_CAN_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief max HOH count supported by the driver */
#define BL_CAN_MAX_HOH                  32U

/** @brief software TX queue depth (pending + queued TX requests) */
#define BL_CAN_TX_QUEUE_DEPTH           8U

/** @brief max CAN filter banks (STM32F1 single CAN: 14 banks, 0..13) */
#define BL_CAN_MAX_FILTER_BANK          14U

/** @brief software RX queue depth (frames buffered between ISR and MainFunction) */
#define BL_CAN_RX_QUEUE_DEPTH           32U

/** @brief enable software bus-off recovery in Bl_Can_MainFunctionBusOff (1=on, 0=off) */
#define BL_CAN_BUSOFF_RECOVERY_ENABLE   1U

/** @brief bus-off recovery wait in ms: CAN is stopped, waits, then restarted */
#define BL_CAN_BUSOFF_RECOVERY_TIME_MS  10U

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_CAN_CFG_H__ */

/******************************* EOF (End of File) ***************************/

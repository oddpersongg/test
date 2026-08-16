/**
 ******************************************************************************
 * @file    Bl_Uds_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Uds pre-compile configuration header file (service implementations)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, service config: SIDs, NRCs,
 *                       sessions, P2/P2*, security seed/key (Phase 1 fixed),
 *                       response buffer size
 *                       [Modify] P2/P2* values now alias the centralized
 *                       Bl_TimingManager_Cfg.h (single source of truth)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_UDS_CFG_H__
#define __BL_UDS_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_TimingManager_Cfg.h"   /* single source for P2/P2* timeout values */

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief supported service IDs (SID) */
#define BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL   0x10U
#define BL_UDS_SID_ECU_RESET                    0x11U
#define BL_UDS_SID_SECURITY_ACCESS              0x27U
#define BL_UDS_SID_REQUEST_DOWNLOAD             0x34U
#define BL_UDS_SID_TRANSFER_DATA                0x36U
#define BL_UDS_SID_REQUEST_TRANSFER_EXIT        0x37U
#define BL_UDS_SID_TESTER_PRESENT               0x3EU

/** @brief TransferData (0x36) block data size (advertised in the 0x74 response) */
#define BL_UDS_TRANSFER_BLOCK_SIZE              2048U

/** @brief diagnostic sessions */
#define BL_UDS_SESSION_DEFAULT                  0x01U
#define BL_UDS_SESSION_PROGRAMMING              0x02U
#define BL_UDS_SESSION_EXTENDED                 0x03U

/** @brief negative response codes (NRC) */
#define BL_UDS_NRC_GENERAL_REJECT               0x10U
#define BL_UDS_NRC_SERVICE_NOT_SUPPORTED        0x11U
#define BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED    0x12U
#define BL_UDS_NRC_INCORRECT_MSG_LENGTH         0x13U
#define BL_UDS_NRC_CONDITIONS_NOT_CORRECT       0x22U
#define BL_UDS_NRC_REQUEST_SEQUENCE_ERROR       0x24U
#define BL_UDS_NRC_REQUEST_OUT_OF_RANGE         0x31U
#define BL_UDS_NRC_SECURITY_ACCESS_DENIED       0x33U
#define BL_UDS_NRC_INVALID_KEY                  0x35U
#define BL_UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION 0x7FU

/** @brief response timing (ms) advertised in the 0x10 response — aliases of
 *         the centralized Bl_TimingManager values. Kept for compatibility;
 *         the 0x10 handler reads the values at runtime via
 *         Bl_TimingManager_GetTimeoutMs() instead of these macros. */
#define BL_UDS_P2_DEFAULT_MS             BL_TIMINGMANAGER_P2_TIMEOUT_MS
#define BL_UDS_P2STAR_DEFAULT_MS         BL_TIMINGMANAGER_P2STAR_TIMEOUT_MS

/** @brief security access seed/key (Phase 1 fixed values; real algorithm later) */
#define BL_UDS_SECURITY_SEED                    0x5AU
#define BL_UDS_SECURITY_KEY                     0xA5U

/** @brief max response SDU length (built in Bl_Uds, sent via CanTp) */
#define BL_UDS_RESPONSE_BUFFER_LEN              64U

/** @brief response TX queue depth (CanTp TX is single-session; back-to-back
 *         requests queue their responses here and flush on TxConfirmation) */
#define BL_UDS_RESPONSE_QUEUE_DEPTH             4U

/** @brief application flash area (0x34 address validation; adjust per layout) */
#define BL_UDS_APP_FLASH_BASE_ADDR              0x08008000UL
#define BL_UDS_APP_FLASH_MAX_SIZE               0x000F8000UL   /* 512KB - bootloader */

/** @brief download session states (Bl_Uds internal) */
#define BL_UDS_DL_STATE_IDLE                    0U
#define BL_UDS_DL_STATE_READY                   1U
#define BL_UDS_DL_STATE_TRANSFERRING            2U

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

#endif /* __BL_UDS_CFG_H__ */

/******************************* EOF (End of File) ***************************/

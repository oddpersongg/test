/**
 ******************************************************************************
 * @file    Bl_CanTp_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_CanTp pre-compile configuration header file (ISO 15765-2, Phase 1)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, simplified Phase 1 config:
 *                       11-bit physical addressing, SF + FF/CF/FC, fixed
 *                       BS/STmin, N_As/N_Bs/N_Cr timeouts
 *                       [Modify] N_* timeout values now alias the centralized
 *                       Bl_TimingManager_Cfg.h (single source of truth)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CANTP_CFG_H__
#define __BL_CANTP_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_CanIf_Cfg.h"
#include "Bl_TimingManager_Cfg.h"   /* single source for N_* timeout values */

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief max data bytes per CAN frame by type (PCI excluded) */
#define BL_CANTP_SF_MAX_DATA_LEN        7U
#define BL_CANTP_FF_DATA_LEN            6U
#define BL_CANTP_CF_DATA_LEN            7U

/** @brief flow control frame DLC (PCI + BS + STmin) */
#define BL_CANTP_FC_DLC                 3U

/** @brief FC parameters we send to the tester (receiver role) */
#define BL_CANTP_DEFAULT_BS             0U      /**< 0 = no block size limit    */
#define BL_CANTP_DEFAULT_STMIN          0x00U   /**< 0 = no inter-frame delay   */

/** @brief protocol timeouts (ms) — aliases of the centralized Bl_TimingManager values */
#define BL_CANTP_N_AS_TIMEOUT_MS        BL_TIMINGMANAGER_N_AS_TIMEOUT_MS   /**< TX: request -> frame confirm     */
#define BL_CANTP_N_BS_TIMEOUT_MS        BL_TIMINGMANAGER_N_BS_TIMEOUT_MS   /**< TX: wait for FC (after FF/block) */
#define BL_CANTP_N_CR_TIMEOUT_MS        BL_TIMINGMANAGER_N_CR_TIMEOUT_MS   /**< RX: wait for next CF             */

/** @brief CanIf PDU ids used by CanTp (diagnostic channel, see Bl_CanIf_Lcfg.c) */
#define BL_CANTP_CANIF_RX_PDU_ID        BL_CANIF_PDU_ID_DIAG_RX         /**< 0x7E0 physical requests */
#define BL_CANTP_CANIF_FUNC_RX_PDU_ID   BL_CANIF_PDU_ID_DIAG_FUNC_RX    /**< 0x7DF functional requests */
#define BL_CANTP_CANIF_TX_PDU_ID        BL_CANIF_PDU_ID_DIAG_TX         /**< 0x7E8 responses */

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

#endif /* __BL_CANTP_CFG_H__ */

/******************************* EOF (End of File) ***************************/

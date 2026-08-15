/**
 ******************************************************************************
 * @file    Bl_CanIf_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-15
 * @brief   Bl_CanIf pre-compile configuration header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, PDU id / direction / size macros;
 *                       V1.0.0 (2026-08-16): diagnostic PDUs 0x7E0 (RX) /
 *                       0x7E8 (TX) for the CanTp layer
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CANIF_CFG_H__
#define __BL_CANIF_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief PDU direction: TX = Bl_CanIf_Transmit target, RX = CanIf_RxIndication source */
#define BL_CANIF_DIR_TX                 0U
#define BL_CANIF_DIR_RX                 1U

/** @brief max PDU entries supported (config sanity bound, >= BL_CANIF_PDU_CNT) */
#define BL_CANIF_MAX_PDUS               8U

/** @brief PDU config entry count (must match g_Bl_CanIf_PduConfig in Bl_CanIf_Lcfg.c) */
#define BL_CANIF_PDU_CNT                2U

/** @brief PDU ids (routing keys used by upper layers) */
#define BL_CANIF_PDU_ID_DIAG_RX         0U
#define BL_CANIF_PDU_ID_DIAG_TX         1U

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

#endif /* __BL_CANIF_CFG_H__ */

/******************************* EOF (End of File) ***************************/

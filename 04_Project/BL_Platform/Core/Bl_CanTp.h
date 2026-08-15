/**
 ******************************************************************************
 * @file    Bl_CanTp.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_CanTp module header file (ISO 15765-2 transport layer, Phase 1)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, simplified Phase 1 transport
 *                       layer: SF + FF/CF/FC, single RX + single TX session,
 *                       N_As/N_Bs/N_Cr timeouts, reassembly into the DCM
 *                       download buffer (BL_DCM_BUFFER_SIZE)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CANTP_H__
#define __BL_CANTP_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Can.h"          /* Bl_Can_PduType (lower-facing RX frame) */
#include "Bl_CanIf.h"        /* Bl_CanIf_PduIdType / Bl_CanIf_Transmit  */
#include "Bl_CanTp_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize the CanTp layer (resets RX/TX sessions)
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_CanTp_Init(void);

/**
 * @brief  de-initialize the CanTp layer
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_CanTp_Deinit(void);

/**
 * @brief  transmit one SDU through the transport layer (upper-layer API)
 * @note   SDUs <= 7 bytes are sent as a single frame; longer SDUs are sent as
 *         FF + CF(s) with flow control from the peer. The SDU buffer must stay
 *         valid until Bl_CanTp_UpperTxConfirmation is called.
 * @param  u16_PduId  : CanIf PDU id (must be BL_CANTP_CANIF_TX_PDU_ID)
 * @param  p_Sdu      : pointer to the SDU data
 * @param  u32_SduLen : SDU length in bytes (1 .. sizeof(BL_DCM_BUFFER_SIZE))
 * @retval BL_E_OK     : request accepted
 * @retval BL_E_NOT_OK : invalid params / busy / PDU id mismatch
 */
bl_ret_t Bl_CanTp_Transmit(Bl_CanIf_PduIdType u16_PduId,
                           const bl_uint8_t *p_Sdu,
                           bl_uint32_t u32_SduLen);

/**
 * @brief  CanTp cyclic function (call from the task scheduler main loop)
 * @note   Handles N_As / N_Bs / N_Cr timeouts and STmin pacing.
 * @param  None
 * @retval None
 */
void Bl_CanTp_MainFunction(void);

/**
 * @brief  lower-facing RX indication (called by CanIf_RxIndication after
 *         PDU routing). Feeds SF/FF/CF frames into the RX session and FC
 *         frames into the TX session.
 * @param  u16_PduId : CanIf PDU id the frame was routed to
 * @param  p_Pdu     : received CAN L-PDU (PCI + payload)
 * @retval None
 */
void Bl_CanTp_RxIndication(Bl_CanIf_PduIdType u16_PduId,
                           const Bl_Can_PduType *p_Pdu);

/**
 * @brief  lower-facing TX confirmation (called by CanIf_TxConfirmation when
 *         a frame sent on the diagnostic TX channel completed)
 * @param  u16_PduId : CanIf PDU id that completed transmission
 * @retval None
 */
void Bl_CanTp_TxConfirmation(Bl_CanIf_PduIdType u16_PduId);

/**
 * @brief  upper-layer RX indication (weak default no-op; UDS will override)
 * @note   Delivers a complete received SDU. For single frames p_Sdu points to
 *         a transient buffer; for multi-frame SDUs it points into the UDS
 *         download buffer. Consume/copy before returning.
 * @param  u16_PduId  : CanIf PDU id the SDU belongs to
 * @param  p_Sdu      : pointer to the SDU data
 * @param  u32_SduLen : SDU length in bytes
 * @retval None
 */
void Bl_CanTp_UpperRxIndication(Bl_CanIf_PduIdType u16_PduId,
                                const bl_uint8_t *p_Sdu,
                                bl_uint32_t u32_SduLen);

/**
 * @brief  upper-layer TX confirmation (weak default no-op; UDS will override)
 * @note   Reports completion of a whole SDU transmission.
 * @param  u16_PduId : CanIf PDU id the SDU belongs to
 * @param  u8_Result : BL_E_OK = sent, BL_E_NOT_OK = aborted (timeout/OVF)
 * @retval None
 */
void Bl_CanTp_UpperTxConfirmation(Bl_CanIf_PduIdType u16_PduId,
                                  bl_uint8_t u8_Result);

/**
 * @brief  upper-layer RX error indication (weak default no-op; UDS may override)
 * @note   Reports an aborted RX session (sequence error / N_Cr timeout).
 * @param  u16_PduId : CanIf PDU id of the aborted session
 * @retval None
 */
void Bl_CanTp_UpperRxErrorIndication(Bl_CanIf_PduIdType u16_PduId);

#ifdef __cplusplus
}
#endif

#endif /* __BL_CANTP_H__ */

/******************************* EOF (End of File) ***************************/

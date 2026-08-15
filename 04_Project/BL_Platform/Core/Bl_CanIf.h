/**
 ******************************************************************************
 * @file    Bl_CanIf.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-15
 * @brief   Bl_CanIf module header file (CAN Interface layer, hardware-independent)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, CAN interface layer interface:
 *                       Bl_CanIf_Transmit; driver-facing CanIf_RxIndication /
 *                       CanIf_TxConfirmation (AUTOSAR MCAL symbols) as the seam
 *                       to the upper layer — CanTp entry points called directly
 *                       here once integrated
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CANIF_H__
#define __BL_CANIF_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Can.h"          /* stable Apdapter interface: PduType / HOH / length */
#include "Bl_CanIf_Cfg.h"
#include "Bl_CanIf_Lcfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/** @brief CanIf PDU id (routing key for upper layers) */
typedef bl_uint16_t Bl_CanIf_PduIdType;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize the CanIf layer (validates the PDU config table)
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed (config table invalid)
 */
bl_ret_t Bl_CanIf_Init(void);

/**
 * @brief  de-initialize the CanIf layer
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_CanIf_Deinit(void);

/**
 * @brief  transmit one L-PDU through the configured PDU (upper-layer API)
 * @note   Looks up the TX PDU config by u16_PduId, builds a Bl_Can_PduType
 *         (swPduHandle = u16_PduId, data beyond dlc zero-filled) and hands it
 *         to the driver via Bl_Can_Write (async, enqueue only). Result is
 *         reported later through Bl_CanIf_TxConfirmation.
 * @param  u16_PduId : CanIf PDU id (see Bl_CanIf_Cfg.h)
 * @param  p_Data    : pointer to payload data (dlc bytes read)
 * @param  u8_Dlc    : payload length [0..8]
 * @retval BL_E_OK     : request accepted (enqueued)
 * @retval BL_E_NOT_OK : PDU id unknown / param invalid / driver rejected
 */
bl_ret_t Bl_CanIf_Transmit(Bl_CanIf_PduIdType u16_PduId,
                           const bl_uint8_t *p_Data,
                           bl_uint8_t u8_Dlc);

/**
 * @brief  CAN driver RX indication (AUTOSAR MCAL symbol: CanIf_RxIndication)
 * @note   Called by the Can driver — Bl_Can.c here, or a real AUTOSAR MCAL Can
 *         driver when porting — when a frame is received. The upper transport
 *         layer (Bl_CanTp) entry point is called from here when CanTp is
 *         integrated; may run in ISR context.
 * @param  hoh   : driver HOH id the frame arrived on
 * @param  p_Pdu : pointer to received L-PDU
 * @retval None
 */
void CanIf_RxIndication(Bl_Can_HohType hoh, const Bl_Can_PduType *p_Pdu);

/**
 * @brief  CAN driver TX confirmation (AUTOSAR MCAL symbol: CanIf_TxConfirmation)
 * @note   Called by the Can driver when a transmission completes. The upper
 *         transport layer (Bl_CanTp) confirmation entry is called from here
 *         when CanTp is integrated.
 * @param  hoh : driver HOH id that completed transmission
 * @retval None
 */
void CanIf_TxConfirmation(Bl_Can_HohType hoh);

#ifdef __cplusplus
}
#endif

#endif /* __BL_CANIF_H__ */

/******************************* EOF (End of File) ***************************/

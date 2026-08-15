/**
 ******************************************************************************
 * @file    Bl_CanIf.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-15
 * @brief   Bl_CanIf module source file (CAN Interface layer, hardware-independent)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, CAN interface layer: TX by PDU id
 *                       (Bl_CanIf_Transmit, PDU config from Bl_CanIf_Lcfg.c),
 *                       driver-facing CanIf_RxIndication / CanIf_TxConfirmation
 *                       (AUTOSAR MCAL symbols) with PDU dispatch (HOH + CAN id)
 *                       to the upper transport layer; V1.0.0 (2026-08-16):
 *                       wired to Bl_CanTp (diagnostic PDUs 0x7E0/0x7E8)
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_CanIf.h"
#include "Bl_CanTp.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief init complete flag */
static bl_uint8_t s_Bl_CanIf_Ready = BL_E_NOT_OK;

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  find a TX PDU config entry by PDU id
 * @param  u16_PduId : CanIf PDU id
 * @retval pointer to config, or BL_NULL_PTR if not found
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindTxPdu(Bl_CanIf_PduIdType u16_PduId);

/**
 * @brief  find a TX PDU config entry by driver HOH (TX confirmation routing)
 * @param  hoh : driver HOH id
 * @retval pointer to config, or BL_NULL_PTR if not found
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindTxPduByHoh(Bl_Can_HohType hoh);

/**
 * @brief  find the RX PDU config entry matching (hoh, can id) — table order matters
 * @param  hoh   : driver HOH id the frame arrived on
 * @param  p_Pdu : received L-PDU
 * @retval pointer to config, or BL_NULL_PTR if no entry matches
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindRxPdu(Bl_Can_HohType hoh,
                                                          const Bl_Can_PduType *p_Pdu);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize the CanIf layer
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed (config table invalid)
 */
bl_ret_t Bl_CanIf_Init(void)
{
    if ((g_Bl_CanIf_PduConfig == BL_NULL_PTR) ||
        (BL_CANIF_PDU_CNT == 0U) ||
        (BL_CANIF_PDU_CNT > BL_CANIF_MAX_PDUS))
    {
        return BL_E_NOT_OK;
    }

    s_Bl_CanIf_Ready = BL_E_OK;

    return BL_E_OK;
}

/**
 * @brief  de-initialize the CanIf layer
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_CanIf_Deinit(void)
{
    s_Bl_CanIf_Ready = BL_E_NOT_OK;

    return BL_E_OK;
}

/**
 * @brief  transmit one L-PDU through the configured PDU (upper-layer API)
 * @param  u16_PduId : CanIf PDU id (see Bl_CanIf_Cfg.h)
 * @param  p_Data    : pointer to payload data (dlc bytes read)
 * @param  u8_Dlc    : payload length [0..8]
 * @retval BL_E_OK     : request accepted (enqueued)
 * @retval BL_E_NOT_OK : PDU id unknown / param invalid / driver rejected
 */
bl_ret_t Bl_CanIf_Transmit(Bl_CanIf_PduIdType u16_PduId,
                           const bl_uint8_t *p_Data,
                           bl_uint8_t u8_Dlc)
{
    const Bl_CanIf_PduConfigType *p_PduInfo;
    Bl_Can_PduType s_Pdu;
    bl_uint8_t i;

    if ((s_Bl_CanIf_Ready != BL_E_OK) ||
        (p_Data == BL_NULL_PTR) ||
        (u8_Dlc > BL_CAN_PDU_DATA_LENGTH))
    {
        return BL_E_NOT_OK;
    }

    p_PduInfo = s_Bl_CanIf_FindTxPdu(u16_PduId);
    if (p_PduInfo == BL_NULL_PTR)
    {
        return BL_E_NOT_OK;
    }

    /* build L-PDU: id from config, swPduHandle = PDU id, data zero-filled */
    s_Pdu.id          = (Bl_Can_CanIdType)p_PduInfo->u32_CanId;
    s_Pdu.swPduHandle = u16_PduId;
    s_Pdu.dlc         = u8_Dlc;
    for (i = 0U; i < BL_CAN_PDU_DATA_LENGTH; i++)
    {
        s_Pdu.data[i] = (i < u8_Dlc) ? p_Data[i] : 0U;
    }

    return Bl_Can_Write((Bl_Can_HohType)p_PduInfo->u16_Hoh, &s_Pdu);
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  find a TX PDU config entry by PDU id
 * @param  u16_PduId : CanIf PDU id
 * @retval pointer to config, or BL_NULL_PTR if not found
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindTxPdu(Bl_CanIf_PduIdType u16_PduId)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_CANIF_PDU_CNT; i++)
    {
        const Bl_CanIf_PduConfigType *p_PduInfo = &g_Bl_CanIf_PduConfig[i];

        if ((p_PduInfo->u8_Dir == BL_CANIF_DIR_TX) && (p_PduInfo->u16_PduId == u16_PduId))
        {
            return p_PduInfo;
        }
    }
    return BL_NULL_PTR;
}

/* ----------------------------------------------------------------------------
 * Driver-facing callbacks. Symbol names match the AUTOSAR MCAL CanIf interface
 * (CanIf_RxIndication / CanIf_TxConfirmation): the Can driver — Bl_Can.c here,
 * or a real AUTOSAR MCAL Can driver when porting — calls them directly, so no
 * adapter is needed. CanIf dispatches by PDU (HOH + CAN id) and hands frames /
 * confirmations to the upper transport layer (Bl_CanTp) below.
 * -------------------------------------------------------------------------- */

/**
 * @brief  find a TX PDU config entry by driver HOH (TX confirmation routing)
 * @param  hoh : driver HOH id
 * @retval pointer to config, or BL_NULL_PTR if not found
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindTxPduByHoh(Bl_Can_HohType hoh)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_CANIF_PDU_CNT; i++)
    {
        const Bl_CanIf_PduConfigType *p_PduInfo = &g_Bl_CanIf_PduConfig[i];

        if ((p_PduInfo->u8_Dir == BL_CANIF_DIR_TX) &&
            ((Bl_Can_HohType)p_PduInfo->u16_Hoh == hoh))
        {
            return p_PduInfo;
        }
    }
    return BL_NULL_PTR;
}

/**
 * @brief  find the RX PDU config entry matching (hoh, can id)
 * @param  hoh   : driver HOH id the frame arrived on
 * @param  p_Pdu : received L-PDU
 * @retval pointer to config, or BL_NULL_PTR if no entry matches
 */
static const Bl_CanIf_PduConfigType *s_Bl_CanIf_FindRxPdu(Bl_Can_HohType hoh,
                                                          const Bl_Can_PduType *p_Pdu)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_CANIF_PDU_CNT; i++)
    {
        const Bl_CanIf_PduConfigType *p_PduInfo = &g_Bl_CanIf_PduConfig[i];

        if ((p_PduInfo->u8_Dir == BL_CANIF_DIR_RX) &&
            ((Bl_Can_HohType)p_PduInfo->u16_Hoh == hoh) &&
            ((p_Pdu->id & p_PduInfo->u32_IdMask) ==
             (p_PduInfo->u32_CanId & p_PduInfo->u32_IdMask)))
        {
            return p_PduInfo;
        }
    }
    return BL_NULL_PTR;
}

/**
 * @brief  CAN driver RX indication (AUTOSAR MCAL: CanIf_RxIndication)
 * @note   Routes the frame to its PDU (HOH + CAN id match) and hands it to the
 *         upper transport layer (Bl_CanTp). May run in ISR context.
 * @param  hoh   : driver HOH id the frame arrived on
 * @param  p_Pdu : pointer to received L-PDU
 * @retval None
 */
void CanIf_RxIndication(Bl_Can_HohType hoh, const Bl_Can_PduType *p_Pdu)
{
    const Bl_CanIf_PduConfigType *p_PduInfo;

    if ((s_Bl_CanIf_Ready != BL_E_OK) || (p_Pdu == BL_NULL_PTR))
    {
        return;
    }

    p_PduInfo = s_Bl_CanIf_FindRxPdu(hoh, p_Pdu);
    if (p_PduInfo != BL_NULL_PTR)
    {
        Bl_CanTp_RxIndication(p_PduInfo->u16_PduId, p_Pdu);
    }
}

/**
 * @brief  CAN driver TX confirmation (AUTOSAR MCAL: CanIf_TxConfirmation)
 * @note   Maps the HOH back to its PDU id and hands the confirmation to the
 *         upper transport layer (Bl_CanTp).
 * @param  hoh : driver HOH id that completed transmission
 * @retval None
 */
void CanIf_TxConfirmation(Bl_Can_HohType hoh)
{
    const Bl_CanIf_PduConfigType *p_PduInfo;

    if (s_Bl_CanIf_Ready != BL_E_OK)
    {
        return;
    }

    p_PduInfo = s_Bl_CanIf_FindTxPduByHoh(hoh);
    if (p_PduInfo != BL_NULL_PTR)
    {
        Bl_CanTp_TxConfirmation(p_PduInfo->u16_PduId);
    }
}

/******************************* EOF (End of File) ***************************/

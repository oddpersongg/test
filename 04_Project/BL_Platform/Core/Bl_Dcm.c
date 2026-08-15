/**
 ******************************************************************************
 * @file    Bl_Dcm.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-13
 * @brief   Bl_Dcm module source file (Dcm dispatcher, ISO 14229)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-13   [New] module created, download buffer reserved;
 *                       V1.0.0 (2026-08-16) Dcm dispatcher: service table
 *                       (SID -> handler implemented in Bl_Uds), NRC gating
 *                       (length / session / security), overrides
 *                       Bl_CanTp_UpperRxIndication;
 *                       [Modify] service table struct extended: sub-function
 *                       length + supported bitmap + suppress-bit flag +
 *                       P2/P2* overrides + response data length; sub-function
 *                       gating (NRC 0x12) added to the dispatch checks;
 *                       [Modify] service table (type + data) moved to the
 *                       Config layer (Bl_Dcm_Lcfg.h/.c), dispatcher now reads
 *                       g_Bl_Dcm_ServiceConfig
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Dcm.h"
#include "Bl_Dcm_Lcfg.h"
#include "Bl_CanTp.h"
#include "Bl_Uds.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  find a service table entry by SID
 * @param  u8_Sid : service ID
 * @retval pointer to entry, or BL_NULL_PTR if not found
 */
static const Bl_Dcm_Service_t *s_Bl_Dcm_FindService(bl_uint8_t u8_Sid);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/**
 * @brief download receive buffer (one TransferData block + protocol overhead)
 * @note  Also the CanTp reassembly buffer (zero-copy). Requests delivered by
 *        CanTp point into this buffer.
 */
bl_uint8_t BL_DCM_BUFFER_SIZE[BL_DCM_BUFFER_LEN];

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  CanTp upper-layer RX indication (overrides the weak default)
 * @note   Dispatch: parse SID, gating checks (length / session / security),
 *         then call the service handler implemented in Bl_Uds.
 * @param  u16_PduId : CanIf PDU id the SDU belongs to (diagnostic RX)
 * @param  p_Sdu     : complete request SDU
 * @param  u32_SduLen: SDU length
 * @retval None
 */
void Bl_CanTp_UpperRxIndication(Bl_CanIf_PduIdType u16_PduId,
                                const bl_uint8_t *p_Sdu,
                                bl_uint32_t u32_SduLen)
{
    const Bl_Dcm_Service_t *p_Info;
    bl_uint8_t u8_Sid;
    bl_uint8_t u8_Sub;

    (void)u16_PduId;

    if ((p_Sdu == BL_NULL_PTR) || (u32_SduLen == 0U))
    {
        return;
    }

    u8_Sid = p_Sdu[0];

    p_Info = s_Bl_Dcm_FindService(u8_Sid);
    if (p_Info == BL_NULL_PTR)
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SERVICE_NOT_SUPPORTED);
        return;
    }

    if (u32_SduLen < p_Info->u8_MinLen)
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    /* sub-function gating (only for services that carry a sub-function byte) */
    if (p_Info->u8_SubFuncLen > 0U)
    {
        if (u32_SduLen < (1U + p_Info->u8_SubFuncLen))
        {
            Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
            return;
        }

        u8_Sub = p_Sdu[1] & 0x7FU;      /* strip the suppress-positive-response bit */

        /* sub-function not in the supported bitmap -> 0x12
           (bitmap covers sub 0x00..0x07; anything above is not supported) */
        if ((p_Info->u8_SubFuncSupported != 0xFFU) &&
            ((u8_Sub > 7U) ||
             ((p_Info->u8_SubFuncSupported & (bl_uint8_t)(1U << u8_Sub)) == 0U)))
        {
            Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
            return;
        }

        /* suppress-positive-response bit used but not allowed for this service -> 0x12 */
        if ((p_Info->u8_SuppressBit == 0U) && ((p_Sdu[1] & 0x80U) != 0U))
        {
            Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
            return;
        }
    }

    if ((p_Info->u8_SessionMask & (1U << (Bl_Uds_GetSession() - 1U))) == 0U)
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
        return;
    }

    if ((p_Info->u8_SecurityNeeded != 0U) && (Bl_Uds_GetSecurityLevel() == 0U))
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SECURITY_ACCESS_DENIED);
        return;
    }

    p_Info->p_Func(p_Sdu, u32_SduLen);
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  find a service table entry by SID
 * @param  u8_Sid : service ID
 * @retval pointer to entry, or BL_NULL_PTR if not found
 */
static const Bl_Dcm_Service_t *s_Bl_Dcm_FindService(bl_uint8_t u8_Sid)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_DCM_SERVICE_CNT; i++)
    {
        if (g_Bl_Dcm_ServiceConfig[i].u8_Sid == u8_Sid)
        {
            return &g_Bl_Dcm_ServiceConfig[i];
        }
    }
    return BL_NULL_PTR;
}

/******************************* EOF (End of File) ***************************/

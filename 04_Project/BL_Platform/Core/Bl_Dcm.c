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
 *                       g_Bl_Dcm_ServiceConfig;
 *                       [Modify] Bl_Dcm_MainFunction added: periodic driver
 *                       of the UDS response TX queue (AUTOSAR
 *                       Dcm_MainFunction style);
 *                       [Modify] S3 session timeout: every accepted request
 *                       restarts the S3 timer (Bl_TimingManager); the
 *                       MainFunction checks expiry and resets the session to
 *                       default via Bl_Uds_ResetToDefaultSession;
 *                       [Modify] functional addressing (0x7DF): requests on
 *                       the functional channel are accepted only for
 *                       TesterPresent 0x3E (refresh S3, NO response); all
 *                       other services on 0x7DF are silently ignored
 *                       [Modify] sub-function gate now resolves the
 *                       supported sub-functions through the service's
 *                       sub-service id table in Bl_UdsService_Lcfg
 *                       (p_SubTable/u8_SubCnt) instead of a bitmap — single
 *                       source of sub-function ids shared with the Uds layer
 *                       [Modify] sub-function gate simplified: calls
 *                       Bl_UdsService_Find(sid, sub) directly — the Dcm
 *                       config table no longer carries any sub-function
 *                       reference (discrimination metadata only)
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Dcm.h"
#include "Bl_Dcm_Lcfg.h"
#include "Bl_CanTp.h"
#include "Bl_Uds.h"
#include "Bl_UdsService.h"      /* sub-function gate (Bl_UdsService_Find) */
#include "Bl_TimingManager.h"

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
    bl_uint8_t u8_FuncAddr;
    bl_uint8_t u8_Sid;
    bl_uint8_t u8_Sub;
    bl_uint32_t u32_DataLen;

    if ((p_Sdu == BL_NULL_PTR) || (u32_SduLen == 0U))
    {
        return;
    }

    /* functional addressing (0x7DF): only broadcast services are allowed,
       and no response is sent (multi-ECU bus would be flooded otherwise).
       TesterPresent 0x3E on the functional channel is handled inline (its
       only effect is refreshing S3 — no other state to update). */
    u8_FuncAddr = (u16_PduId == BL_CANTP_CANIF_FUNC_RX_PDU_ID) ? 1U : 0U;

    u8_Sid = p_Sdu[0];
    if (u8_FuncAddr != 0U)
    {
        if (u8_Sid == BL_UDS_SID_TESTER_PRESENT)
        {
            /* valid functional TesterPresent: refresh S3 only, no response */
            (void)Bl_TimingManager_Start(BL_TIMINGMANAGER_TIMER_S3);
        }
        return;     /* all other services on functional addressing: ignore */
    }

    /* any valid physical diagnostic request refreshes the session timeout (S3) */
    (void)Bl_TimingManager_Start(BL_TIMINGMANAGER_TIMER_S3);

    p_Info = s_Bl_Dcm_FindService(u8_Sid);
    if (p_Info == BL_NULL_PTR)
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_SERVICE_NOT_SUPPORTED);
        return;
    }

    /* data-segment length gate: data = request length minus SID and
       sub-function. Protocol range check only (rejects clearly malformed
       requests with 0x13); precise per-request validation stays in handlers.
       min==max => fixed data length. */
    u32_DataLen = u32_SduLen - 1U - p_Info->u8_SubFuncLen;
    if ((u32_DataLen < p_Info->u8_MinDataLen) ||
        (u32_DataLen > p_Info->u16_MaxDataLen))
    {
        Bl_Uds_SendNrc(u8_Sid, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    /* sub-function gating (only for services that carry a sub-function byte)
       ISO 14229 sub-function is always 1 byte; the request may carry extra
       data after it (e.g. 0x27 key, 0x34 address), so only the presence of
       the field is checked here — value validity is decided by the
       UdsService sub-service table: Bl_UdsService_Find(sid, sub) returns
       NULL when the sub-function is not configured -> 0x12. The sub-function
       id is defined in exactly one place (Bl_UdsService_Lcfg). */
    if (p_Info->u8_SubFuncLen > 0U)
    {
        u8_Sub = p_Sdu[1] & 0x7FU;      /* strip the suppress-positive-response bit */

        if (Bl_UdsService_Find(u8_Sid, u8_Sub) == BL_NULL_PTR)
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

/**
 * @brief  Dcm cyclic function (AUTOSAR Dcm_MainFunction style)
 * @note   Periodically called from the scheduler main loop (after CanTp).
 *         Drives the UDS response TX queue: when CanTp is busy (single
 *         session) the head response is retried on every call, keeping
 *         back-to-back responses ordered and self-healing a rejected
 *         Transmit window. All UDS state (session/security/download) lives
 *         in Bl_Uds; Dcm only discriminates and dispatches.
 * @param  None
 * @retval None
 */
void Bl_Dcm_MainFunction(void)
{
    Bl_Uds_ProcessResponseQueue();

    /* S3 session timeout: no request for S3_TIMEOUT_MS -> back to default
       session (security + download state reset by the Uds layer) */
    if (Bl_TimingManager_IsExpired(BL_TIMINGMANAGER_TIMER_S3) != 0U)
    {
        Bl_Uds_ResetToDefaultSession();
    }
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

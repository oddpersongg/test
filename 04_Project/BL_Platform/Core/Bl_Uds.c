/**
 ******************************************************************************
 * @file    Bl_Uds.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Uds module source file (UDS / ISO 14229 service implementations)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, diagnostic service functions:
 *                       0x10/0x11/0x27/0x34/0x36/0x37/0x3E; session, security
 *                       and download state; response/NRC helpers; overrides
 *                       Bl_CanTp_UpperRxErrorIndication to reset download state
 *                       [Fix] response TX queue: back-to-back requests used to
 *                       drop the middle response because Bl_CanTp_Transmit is
 *                       single-session (returns NOT_OK while busy) and the old
 *                       send path ignored the result. Responses are now queued
 *                       and flushed on Bl_CanTp_UpperTxConfirmation, preserving
 *                       order. Overrides Bl_CanTp_UpperTxConfirmation.
 *                       [Modify] queue flush exposed as Bl_Uds_ProcessResponseQueue()
 *                       and additionally driven periodically by
 *                       Bl_Dcm_MainFunction() (AUTOSAR Dcm_MainFunction style
 *                       bounded retry; self-heals rejected-Transmit window)
 *                       [Modify] P2/P2* advertisement in the 0x10 response now
 *                       reads Bl_TimingManager_GetTimeoutMs() at runtime
 *                       (centralized timing config, not compile-time macros)
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Uds.h"
#include "Bl_CanTp.h"
#include "Bl_TimingManager.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief response TX queue entry (holds a copy of one pending response)
 */
typedef struct {
    bl_uint8_t u8_Len;                            /**< response length            */
    bl_uint8_t p_Data[BL_UDS_RESPONSE_BUFFER_LEN]; /**< response data (copy)       */
} Bl_Uds_TxQueueEntry_t;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief response SDU buffer (built by the service handlers) */
static bl_uint8_t s_Bl_Uds_Resp[BL_UDS_RESPONSE_BUFFER_LEN];

/** @brief response TX queue (CanTp TX is single-session, so responses wait here) */
static Bl_Uds_TxQueueEntry_t s_Bl_Uds_TxQueue[BL_UDS_RESPONSE_QUEUE_DEPTH];

/** @brief TX queue head / count (ring) */
static bl_uint8_t s_Bl_Uds_TxHead = 0U;
static bl_uint8_t s_Bl_Uds_TxCount = 0U;

/** @brief 1 = the head response has been handed to CanTp (in flight);
 *         popped only on Bl_CanTp_UpperTxConfirmation so multi-frame SDUs
 *         (p_Sdu referenced across CFs) stay valid */
static bl_uint8_t s_Bl_Uds_TxInFlight = 0U;

/** @brief current diagnostic session */
static bl_uint8_t s_Bl_Uds_Session = BL_UDS_SESSION_DEFAULT;

/** @brief current security level (0 = locked) */
static bl_uint8_t s_Bl_Uds_SecurityLevel = 0U;

/** @brief download session state */
static bl_uint8_t s_Bl_Uds_DlState = BL_UDS_DL_STATE_IDLE;

/** @brief requested download address / length (0x34) */
static bl_uint32_t s_Bl_Uds_DlAddr = 0U;
static bl_uint32_t s_Bl_Uds_DlLen = 0U;

/** @brief bytes accepted so far (0x36) */
static bl_uint32_t s_Bl_Uds_DlAccepted = 0U;

/** @brief last issued block sequence counter (0x36) */
static bl_uint8_t s_Bl_Uds_DlLastBlock = 0U;

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

static void s_Bl_Uds_SendResponse(bl_uint8_t u8_Len);
static bl_uint32_t s_Bl_Uds_ReadBytes(const bl_uint8_t *p_Src, bl_uint8_t u8_N);
static void s_Bl_Uds_TxQueuePush(bl_uint8_t u8_Len);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize the UDS service layer
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_Uds_Init(void)
{
    s_Bl_Uds_Session       = BL_UDS_SESSION_DEFAULT;
    s_Bl_Uds_SecurityLevel = 0U;
    s_Bl_Uds_DlState       = BL_UDS_DL_STATE_IDLE;
    s_Bl_Uds_DlAddr        = 0U;
    s_Bl_Uds_DlLen         = 0U;
    s_Bl_Uds_DlAccepted    = 0U;
    s_Bl_Uds_DlLastBlock   = 0U;
    s_Bl_Uds_TxHead        = 0U;
    s_Bl_Uds_TxCount       = 0U;

    /* start the S3 session timer (refreshed on every request by Dcm) */
    (void)Bl_TimingManager_Start(BL_TIMINGMANAGER_TIMER_S3);

    return BL_E_OK;
}

/**
 * @brief  get the current diagnostic session
 * @param  None
 * @retval session value
 */
bl_uint8_t Bl_Uds_GetSession(void)
{
    return s_Bl_Uds_Session;
}

/**
 * @brief  reset to the default diagnostic session (S3 timeout)
 * @param  None
 * @retval None
 */
void Bl_Uds_ResetToDefaultSession(void)
{
    s_Bl_Uds_Session       = BL_UDS_SESSION_DEFAULT;
    s_Bl_Uds_SecurityLevel = 0U;
    s_Bl_Uds_DlState       = BL_UDS_DL_STATE_IDLE;
    s_Bl_Uds_DlAccepted    = 0U;
    s_Bl_Uds_DlLastBlock   = 0U;
}

/**
 * @brief  get the current security level (0 = locked)
 * @param  None
 * @retval security level
 */
bl_uint8_t Bl_Uds_GetSecurityLevel(void)
{
    return s_Bl_Uds_SecurityLevel;
}

/**
 * @brief  send a negative response (0x7F SID NRC)
 * @param  u8_Sid : request SID
 * @param  u8_Nrc : negative response code
 * @retval None
 */
void Bl_Uds_SendNrc(bl_uint8_t u8_Sid, bl_uint8_t u8_Nrc)
{
    s_Bl_Uds_Resp[0] = 0x7FU;
    s_Bl_Uds_Resp[1] = u8_Sid;
    s_Bl_Uds_Resp[2] = u8_Nrc;

    s_Bl_Uds_SendResponse(3U);
}

/**
 * @brief  diagnostic session control (0x10)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_DiagSessionControl(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t u8_Sub;

    if (u32_ReqLen < 2U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Sub = p_Req[1];
    if ((u8_Sub != BL_UDS_SESSION_DEFAULT) &&
        (u8_Sub != BL_UDS_SESSION_PROGRAMMING) &&
        (u8_Sub != BL_UDS_SESSION_EXTENDED))
    {
        Bl_Uds_SendNrc(BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    /* session change resets security and any in-progress download */
    s_Bl_Uds_Session       = u8_Sub;
    s_Bl_Uds_SecurityLevel = 0U;
    s_Bl_Uds_DlState       = BL_UDS_DL_STATE_IDLE;
    s_Bl_Uds_DlAccepted    = 0U;
    s_Bl_Uds_DlLastBlock   = 0U;

    s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL + 0x40U);
    s_Bl_Uds_Resp[1] = u8_Sub;
    /* P2 / P2* advertised at runtime from the centralized timing config */
    s_Bl_Uds_Resp[2] = (bl_uint8_t)(Bl_TimingManager_GetTimeoutMs(BL_TIMINGMANAGER_TIMER_P2) >> 8U);
    s_Bl_Uds_Resp[3] = (bl_uint8_t)(Bl_TimingManager_GetTimeoutMs(BL_TIMINGMANAGER_TIMER_P2) & 0xFFU);
    s_Bl_Uds_Resp[4] = (bl_uint8_t)(Bl_TimingManager_GetTimeoutMs(BL_TIMINGMANAGER_TIMER_P2STAR) >> 8U);
    s_Bl_Uds_Resp[5] = (bl_uint8_t)(Bl_TimingManager_GetTimeoutMs(BL_TIMINGMANAGER_TIMER_P2STAR) & 0xFFU);

    s_Bl_Uds_SendResponse(6U);
}

/**
 * @brief  ECU reset (0x11)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_ECUReset(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t u8_Sub;

    if (u32_ReqLen < 2U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_ECU_RESET, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Sub = p_Req[1];
    if (u8_Sub != 0x01U)    /* Phase 1: hard reset only */
    {
        Bl_Uds_SendNrc(BL_UDS_SID_ECU_RESET, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_ECU_RESET + 0x40U);
    s_Bl_Uds_Resp[1] = u8_Sub;
    s_Bl_Uds_SendResponse(2U);

    /* TODO: trigger the actual reset (e.g. NVIC_SystemReset) after the
       response is on the bus (delay + reset via the Apdapter layer) */
}

/**
 * @brief  security access (0x27)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_SecurityAccess(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t u8_Sub;

    if (u32_ReqLen < 2U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_SECURITY_ACCESS, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Sub = p_Req[1];
    if (u8_Sub == 0x01U)    /* request seed */
    {
        s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_SECURITY_ACCESS + 0x40U);
        s_Bl_Uds_Resp[1] = u8_Sub;
        s_Bl_Uds_Resp[2] = BL_UDS_SECURITY_SEED;
        s_Bl_Uds_SendResponse(3U);
    }
    else if (u8_Sub == 0x02U)   /* send key */
    {
        if (u32_ReqLen < 3U)
        {
            Bl_Uds_SendNrc(BL_UDS_SID_SECURITY_ACCESS, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
            return;
        }
        if (p_Req[2] == BL_UDS_SECURITY_KEY)
        {
            s_Bl_Uds_SecurityLevel = 1U;
            s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_SECURITY_ACCESS + 0x40U);
            s_Bl_Uds_Resp[1] = u8_Sub;
            s_Bl_Uds_SendResponse(2U);
        }
        else
        {
            Bl_Uds_SendNrc(BL_UDS_SID_SECURITY_ACCESS, BL_UDS_NRC_INVALID_KEY);
        }
    }
    else
    {
        Bl_Uds_SendNrc(BL_UDS_SID_SECURITY_ACCESS, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
    }
}

/**
 * @brief  request download (0x34)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_RequestDownload(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t u8_Afl;
    bl_uint8_t u8_AddrLen;
    bl_uint8_t u8_LenLen;
    bl_uint32_t u32_Addr;
    bl_uint32_t u32_Len;
    const bl_uint8_t *p_Cursor;

    if (u32_ReqLen < 4U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_REQUEST_DOWNLOAD, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Afl = p_Req[2];
    u8_AddrLen = (bl_uint8_t)(u8_Afl >> 4U);    /* address length in bytes */
    u8_LenLen  = (bl_uint8_t)(u8_Afl & 0x0FU);  /* length length in bytes   */
    if ((u8_AddrLen == 0U) || (u8_AddrLen > 4U) || (u8_LenLen == 0U) || (u8_LenLen > 4U))
    {
        Bl_Uds_SendNrc(BL_UDS_SID_REQUEST_DOWNLOAD, BL_UDS_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }
    if ((bl_uint32_t)(3U + u8_AddrLen + u8_LenLen) > u32_ReqLen)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_REQUEST_DOWNLOAD, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    p_Cursor = &p_Req[3];
    u32_Addr = s_Bl_Uds_ReadBytes(p_Cursor, u8_AddrLen);
    p_Cursor += u8_AddrLen;
    u32_Len = s_Bl_Uds_ReadBytes(p_Cursor, u8_LenLen);

    /* address range validation against the application flash area */
    if ((u32_Len == 0U) ||
        (u32_Addr < BL_UDS_APP_FLASH_BASE_ADDR) ||
        ((u32_Addr + u32_Len) > (BL_UDS_APP_FLASH_BASE_ADDR + BL_UDS_APP_FLASH_MAX_SIZE)))
    {
        Bl_Uds_SendNrc(BL_UDS_SID_REQUEST_DOWNLOAD, BL_UDS_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    s_Bl_Uds_DlAddr     = u32_Addr;
    s_Bl_Uds_DlLen      = u32_Len;
    s_Bl_Uds_DlAccepted = 0U;
    s_Bl_Uds_DlLastBlock = 0U;
    s_Bl_Uds_DlState    = BL_UDS_DL_STATE_READY;

    /* positive response: 0x74, lengthFormat (2-byte max block length), 0x0800 */
    s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_REQUEST_DOWNLOAD + 0x40U);
    s_Bl_Uds_Resp[1] = 0x20U;
    s_Bl_Uds_Resp[2] = (bl_uint8_t)((BL_UDS_TRANSFER_BLOCK_SIZE >> 8U) & 0xFFU);
    s_Bl_Uds_Resp[3] = (bl_uint8_t)(BL_UDS_TRANSFER_BLOCK_SIZE & 0xFFU);
    s_Bl_Uds_SendResponse(4U);
}

/**
 * @brief  transfer data (0x36)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_TransferData(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t  u8_Block;
    bl_uint32_t u32_DataLen;

    if (s_Bl_Uds_DlState == BL_UDS_DL_STATE_IDLE)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TRANSFER_DATA, BL_UDS_NRC_REQUEST_SEQUENCE_ERROR);
        return;
    }
    if (u32_ReqLen < 2U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TRANSFER_DATA, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Block = p_Req[1];
    if (u8_Block != (bl_uint8_t)(s_Bl_Uds_DlLastBlock + 1U))
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TRANSFER_DATA, BL_UDS_NRC_REQUEST_SEQUENCE_ERROR);
        return;
    }

    u32_DataLen = u32_ReqLen - 2U;
    if ((s_Bl_Uds_DlAccepted + u32_DataLen) > s_Bl_Uds_DlLen)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TRANSFER_DATA, BL_UDS_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }
    /* defense-in-depth: the destination address must stay inside the app area */
    if ((s_Bl_Uds_DlAddr + s_Bl_Uds_DlAccepted + u32_DataLen) >
        (BL_UDS_APP_FLASH_BASE_ADDR + BL_UDS_APP_FLASH_MAX_SIZE))
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TRANSFER_DATA, BL_UDS_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    /* TODO: write p_Req[2..] (u32_DataLen bytes) to flash at
       s_Bl_Uds_DlAddr + s_Bl_Uds_DlAccepted via the Bl_Flash driver (Apdapter) */
    s_Bl_Uds_DlAccepted += u32_DataLen;
    s_Bl_Uds_DlLastBlock = u8_Block;
    s_Bl_Uds_DlState     = BL_UDS_DL_STATE_TRANSFERRING;

    s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_TRANSFER_DATA + 0x40U);
    s_Bl_Uds_Resp[1] = u8_Block;
    s_Bl_Uds_SendResponse(2U);
}

/**
 * @brief  request transfer exit (0x37)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_RequestTransferExit(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    (void)u32_ReqLen;

    if (s_Bl_Uds_DlState == BL_UDS_DL_STATE_IDLE)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_REQUEST_TRANSFER_EXIT, BL_UDS_NRC_REQUEST_SEQUENCE_ERROR);
        return;
    }

    /* TODO: finalize the download (e.g. verify written data) */
    s_Bl_Uds_DlState = BL_UDS_DL_STATE_IDLE;
    s_Bl_Uds_DlAccepted = 0U;
    s_Bl_Uds_DlLastBlock = 0U;

    s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_REQUEST_TRANSFER_EXIT + 0x40U);
    s_Bl_Uds_SendResponse(1U);
}

/**
 * @brief  tester present (0x3E)
 * @param  p_Req     : request SDU
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
void Bl_Uds_TesterPresent(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen)
{
    bl_uint8_t u8_Sub;

    if (u32_ReqLen < 2U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TESTER_PRESENT, BL_UDS_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    u8_Sub = p_Req[1];
    if ((u8_Sub & 0x7FU) != 0x00U)
    {
        Bl_Uds_SendNrc(BL_UDS_SID_TESTER_PRESENT, BL_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
        return;
    }

    /* sub-function bit 7 = suppress positive response */
    if ((u8_Sub & 0x80U) == 0U)
    {
        s_Bl_Uds_Resp[0] = (bl_uint8_t)(BL_UDS_SID_TESTER_PRESENT + 0x40U);
        s_Bl_Uds_Resp[1] = 0x00U;
        s_Bl_Uds_SendResponse(2U);
    }
}

/**
 * @brief  CanTp upper-layer RX error indication (overrides the weak default)
 * @note   A multi-frame reception was aborted (SN error / N_Cr timeout):
 *         reset the download state so a new transfer starts clean.
 * @param  u16_PduId : CanIf PDU id of the aborted session
 * @retval None
 */
void Bl_CanTp_UpperRxErrorIndication(Bl_CanIf_PduIdType u16_PduId)
{
    (void)u16_PduId;

    s_Bl_Uds_DlState     = BL_UDS_DL_STATE_IDLE;
    s_Bl_Uds_DlAccepted  = 0U;
    s_Bl_Uds_DlLastBlock = 0U;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  CanTp upper-layer TX confirmation (overrides the weak default)
 * @note   A response finished transmitting (or failed). Try to flush the
 *         next queued response so back-to-back requests keep their order.
 * @param  u16_PduId : CanIf PDU id of the completed TX
 * @param  u8_Result : BL_E_OK / BL_E_NOT_OK
 * @retval None
 */
void Bl_CanTp_UpperTxConfirmation(Bl_CanIf_PduIdType u16_PduId,
                                  bl_uint8_t u8_Result)
{
    (void)u16_PduId;
    (void)u8_Result;

    /* the in-flight head response completed (or failed): pop it, then try
       the next queued response so back-to-back requests keep their order */
    if (s_Bl_Uds_TxInFlight != 0U)
    {
        s_Bl_Uds_TxInFlight = 0U;
        s_Bl_Uds_TxHead = (bl_uint8_t)((s_Bl_Uds_TxHead + 1U) %
                                       BL_UDS_RESPONSE_QUEUE_DEPTH);
        s_Bl_Uds_TxCount--;
    }

    Bl_Uds_ProcessResponseQueue();
}

/**
 * @brief  process the response TX queue (AUTOSAR Dcm_MainFunction style)
 * @note   Periodically called by Bl_Dcm_MainFunction() (and directly after
 *         enqueue / TxConfirmation). If CanTp is busy (single session), the
 *         head response stays queued and is retried on the next call — this
 *         is the bounded retry that replaces dropping on back-to-back
 *         requests, and it also self-heals the "CanTp busy without a pending
 *         confirmation" window (e.g. a rejected Transmit).
 * @param  None
 * @retval None
 */
void Bl_Uds_ProcessResponseQueue(void)
{
    if (s_Bl_Uds_TxCount == 0U)
    {
        return;
    }

    /* a response is already in flight: wait for its confirmation */
    if (s_Bl_Uds_TxInFlight != 0U)
    {
        return;
    }

    if (Bl_CanTp_Transmit(BL_CANTP_CANIF_TX_PDU_ID,
                          s_Bl_Uds_TxQueue[s_Bl_Uds_TxHead].p_Data,
                          s_Bl_Uds_TxQueue[s_Bl_Uds_TxHead].u8_Len) == BL_E_OK)
    {
        s_Bl_Uds_TxInFlight = 1U;
    }
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  send the response SDU through CanTp (diagnostic TX channel)
 * @note   CanTp TX is single-session: if a previous response is still in
 *         flight (back-to-back requests), the response is queued instead of
 *         dropped, and flushed on Bl_CanTp_UpperTxConfirmation.
 * @param  u8_Len : response length in bytes
 * @retval None
 */
static void s_Bl_Uds_SendResponse(bl_uint8_t u8_Len)
{
    if (u8_Len > BL_UDS_RESPONSE_BUFFER_LEN)
    {
        return;
    }

    s_Bl_Uds_TxQueuePush(u8_Len);
    Bl_Uds_ProcessResponseQueue();
}

/**
 * @brief  push a copy of the current response into the TX queue
 * @param  u8_Len : response length in bytes
 * @retval None
 */
static void s_Bl_Uds_TxQueuePush(bl_uint8_t u8_Len)
{
    bl_uint8_t u8_Tail;
    bl_uint8_t i;

    if (s_Bl_Uds_TxCount >= BL_UDS_RESPONSE_QUEUE_DEPTH)
    {
        return;     /* queue full: drop (should not happen in practice) */
    }

    u8_Tail = (bl_uint8_t)((s_Bl_Uds_TxHead + s_Bl_Uds_TxCount) %
                           BL_UDS_RESPONSE_QUEUE_DEPTH);

    s_Bl_Uds_TxQueue[u8_Tail].u8_Len = u8_Len;
    for (i = 0U; i < u8_Len; i++)
    {
        s_Bl_Uds_TxQueue[u8_Tail].p_Data[i] = s_Bl_Uds_Resp[i];
    }
    s_Bl_Uds_TxCount++;
}

/**
 * @brief  read a big-endian integer (1..4 bytes) from a request field
 * @param  p_Src : pointer to the field
 * @param  u8_N  : number of bytes to read
 * @retval the value
 */
static bl_uint32_t s_Bl_Uds_ReadBytes(const bl_uint8_t *p_Src, bl_uint8_t u8_N)
{
    bl_uint32_t u32_Val = 0U;
    bl_uint8_t i;

    for (i = 0U; i < u8_N; i++)
    {
        u32_Val = (u32_Val << 8U) | p_Src[i];
    }
    return u32_Val;
}

/******************************* EOF (End of File) ***************************/

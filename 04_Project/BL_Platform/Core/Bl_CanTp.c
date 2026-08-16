/**
 ******************************************************************************
 * @file    Bl_CanTp.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_CanTp module source file (ISO 15765-2 transport layer, Phase 1)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, simplified Phase 1 transport
 *                       layer: PCI parse/encode (SF/FF/CF/FC), single RX +
 *                       single TX session, flow control (BS/STmin), N_As /
 *                       N_Bs / N_Cr timeouts, reassembly into the DCM
 *                       download buffer (BL_DCM_BUFFER_SIZE)
 *                       [Modify] functional RX PDU id (0x7DF) accepted; RX
 *                       session records its source PDU id so multi-frame
 *                       delivery/abort reports the correct channel
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_CanTp.h"
#include "Bl_CanIf.h"
#include "Bl_Dcm.h"
#include "Bl_TaskSchedule.h"    /* system tick (Bl_TaskSchedule_GetTickMs) */

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief PCI types */
#define BL_CANTP_PCI_TYPE_SF            0U
#define BL_CANTP_PCI_TYPE_FF            1U
#define BL_CANTP_PCI_TYPE_CF            2U
#define BL_CANTP_PCI_TYPE_FC            3U

/** @brief flow status (FS) values */
#define BL_CANTP_FC_CTS                 0U
#define BL_CANTP_FC_WAIT                1U
#define BL_CANTP_FC_OVF                 2U

/** @brief TX session states */
#define BL_CANTP_TX_IDLE                0U
#define BL_CANTP_TX_SF_WAIT_CONF        1U
#define BL_CANTP_TX_FF_WAIT_CONF        2U
#define BL_CANTP_TX_FF_WAIT_FC          3U
#define BL_CANTP_TX_CF_WAIT_CONF        4U
#define BL_CANTP_TX_CF_PACING           5U
#define BL_CANTP_TX_CF_WAIT_FC          6U

/** @brief RX session states */
#define BL_CANTP_RX_IDLE                0U
#define BL_CANTP_RX_FF                  1U

/** @brief TX timer kinds */
#define BL_CANTP_TX_TIMER_NONE          0U
#define BL_CANTP_TX_TIMER_AS            1U
#define BL_CANTP_TX_TIMER_BS            2U

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief TX session runtime data (one multi/single-frame transmission)
 */
typedef struct {
    const bl_uint8_t *p_Sdu;          /**< SDU source pointer                 */
    bl_uint32_t      u32_SduLen;      /**< total SDU length                   */
    bl_uint32_t      u32_SentLen;     /**< bytes already sent                 */
    bl_uint8_t       u8_State;        /**< TX state                           */
    bl_uint8_t       u8_Sn;           /**< next CF sequence number (0..15)    */
    bl_uint8_t       u8_Bs;           /**< block size from FC (0 = unlimited) */
    bl_uint8_t       u8_StminMs;      /**< STmin from FC (ms, 0 = none)       */
    bl_uint8_t       u8_CfInBlock;    /**< CFs sent in current block          */
    bl_uint32_t      u32_NextCfTick;  /**< next CF send tick (STmin pacing)   */
    bl_uint32_t      u32_TimerStart;  /**< active timeout start tick          */
    bl_uint8_t       u8_TimerKind;    /**< BL_CANTP_TX_TIMER_*                */
} Bl_CanTp_TxSession_t;

/**
 * @brief RX session runtime data (multi-frame reception)
 */
typedef struct {
    bl_uint8_t       *p_Buf;          /**< reassembly buffer                  */
    bl_uint32_t      u32_SduLen;      /**< expected total length (from FF)    */
    bl_uint32_t      u32_RxLen;       /**< bytes received so far              */
    bl_uint8_t       u8_State;        /**< RX state                           */
    bl_uint8_t       u8_Sn;           /**< expected next CF SN (0..15)        */
    bl_uint8_t       u8_CfInBlock;    /**< CFs received in current block      */
    bl_uint8_t       u8_Bs;           /**< BS used in our FC                  */
    bl_uint16_t      u16_PduId;       /**< CanIf PDU id of the session        */
    bl_uint32_t      u32_TimerStart;  /**< N_Cr timer start tick              */
} Bl_CanTp_RxSession_t;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief init complete flag */
static bl_uint8_t s_Bl_CanTp_Ready = BL_E_NOT_OK;

/** @brief TX session state */
static Bl_CanTp_TxSession_t s_Bl_CanTp_Tx;

/** @brief RX session state */
static Bl_CanTp_RxSession_t s_Bl_CanTp_Rx;

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

static bl_ret_t s_Bl_CanTp_TxSendSf(void);
static bl_ret_t s_Bl_CanTp_TxSendFf(void);
static bl_ret_t s_Bl_CanTp_TxSendCf(void);
static bl_ret_t s_Bl_CanTp_TxSendFc(bl_uint8_t u8_Fs, bl_uint8_t u8_Bs, bl_uint8_t u8_Stmin);
static void     s_Bl_CanTp_TxOnCfConfirmed(void);
static void     s_Bl_CanTp_TxOnFc(bl_uint8_t u8_Fs, bl_uint8_t u8_Bs, bl_uint8_t u8_Stmin);
static void     s_Bl_CanTp_TxAbort(bl_uint8_t u8_Result);
static void     s_Bl_CanTp_TxMainFunction(void);
static bl_uint8_t s_Bl_CanTp_FcStminToMs(bl_uint8_t u8_Stmin);
static void     s_Bl_CanTp_RxStartFf(Bl_CanIf_PduIdType u16_PduId, const Bl_Can_PduType *p_Pdu);
static void     s_Bl_CanTp_RxHandleCf(const Bl_Can_PduType *p_Pdu);
static void     s_Bl_CanTp_RxAbort(bl_uint8_t u8_Result);
static void     s_Bl_CanTp_RxMainFunction(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize the CanTp layer
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_CanTp_Init(void)
{
    s_Bl_CanTp_Tx.u8_State = BL_CANTP_TX_IDLE;
    s_Bl_CanTp_Tx.u8_TimerKind = BL_CANTP_TX_TIMER_NONE;
    s_Bl_CanTp_Rx.u8_State = BL_CANTP_RX_IDLE;
    s_Bl_CanTp_Ready = BL_E_OK;

    return BL_E_OK;
}

/**
 * @brief  de-initialize the CanTp layer
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_CanTp_Deinit(void)
{
    s_Bl_CanTp_Ready = BL_E_NOT_OK;
    s_Bl_CanTp_Tx.u8_State = BL_CANTP_TX_IDLE;
    s_Bl_CanTp_Tx.u8_TimerKind = BL_CANTP_TX_TIMER_NONE;
    s_Bl_CanTp_Rx.u8_State = BL_CANTP_RX_IDLE;

    return BL_E_OK;
}

/**
 * @brief  transmit one SDU through the transport layer (upper-layer API)
 * @param  u16_PduId  : CanIf PDU id (must be BL_CANTP_CANIF_TX_PDU_ID)
 * @param  p_Sdu      : pointer to the SDU data
 * @param  u32_SduLen : SDU length in bytes (1 .. sizeof(BL_DCM_BUFFER_SIZE))
 * @retval BL_E_OK     : request accepted
 * @retval BL_E_NOT_OK : invalid params / busy / PDU id mismatch
 */
bl_ret_t Bl_CanTp_Transmit(Bl_CanIf_PduIdType u16_PduId,
                           const bl_uint8_t *p_Sdu,
                           bl_uint32_t u32_SduLen)
{
    if ((s_Bl_CanTp_Ready != BL_E_OK) ||
        (p_Sdu == BL_NULL_PTR) ||
        (u32_SduLen == 0U) ||
        (u32_SduLen > sizeof(BL_DCM_BUFFER_SIZE)))
    {
        return BL_E_NOT_OK;
    }

    if ((u16_PduId != BL_CANTP_CANIF_TX_PDU_ID) ||
        (s_Bl_CanTp_Tx.u8_State != BL_CANTP_TX_IDLE))
    {
        return BL_E_NOT_OK;
    }

    s_Bl_CanTp_Tx.p_Sdu       = p_Sdu;
    s_Bl_CanTp_Tx.u32_SduLen  = u32_SduLen;
    s_Bl_CanTp_Tx.u32_SentLen = 0U;
    s_Bl_CanTp_Tx.u8_Sn       = 0U;
    s_Bl_CanTp_Tx.u8_Bs       = 0U;
    s_Bl_CanTp_Tx.u8_StminMs  = 0U;
    s_Bl_CanTp_Tx.u8_CfInBlock = 0U;

    if (u32_SduLen <= BL_CANTP_SF_MAX_DATA_LEN)
    {
        /* single frame */
        if (s_Bl_CanTp_TxSendSf() != BL_E_OK)
        {
            s_Bl_CanTp_Tx.u8_State = BL_CANTP_TX_IDLE;
            return BL_E_NOT_OK;
        }
        s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_SF_WAIT_CONF;
        s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_AS;
        s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
    }
    else
    {
        /* first frame (multi-frame) */
        if (s_Bl_CanTp_TxSendFf() != BL_E_OK)
        {
            s_Bl_CanTp_Tx.u8_State = BL_CANTP_TX_IDLE;
            return BL_E_NOT_OK;
        }
        s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_FF_WAIT_CONF;
        s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_AS;
        s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
    }

    return BL_E_OK;
}

/**
 * @brief  CanTp cyclic function (call from the task scheduler main loop)
 * @param  None
 * @retval None
 */
void Bl_CanTp_MainFunction(void)
{
    if (s_Bl_CanTp_Ready != BL_E_OK)
    {
        return;
    }

    s_Bl_CanTp_TxMainFunction();
    s_Bl_CanTp_RxMainFunction();
}

/**
 * @brief  lower-facing RX indication (called by CanIf_RxIndication)
 * @param  u16_PduId : CanIf PDU id the frame was routed to
 * @param  p_Pdu     : received CAN L-PDU (PCI + payload)
 * @retval None
 */
void Bl_CanTp_RxIndication(Bl_CanIf_PduIdType u16_PduId,
                           const Bl_Can_PduType *p_Pdu)
{
    bl_uint8_t u8_Pci;
    bl_uint8_t u8_Type;
    bl_uint8_t u8_Len;

    if ((s_Bl_CanTp_Ready != BL_E_OK) || (p_Pdu == BL_NULL_PTR))
    {
        return;
    }

    if ((u16_PduId != BL_CANTP_CANIF_RX_PDU_ID) &&
        (u16_PduId != BL_CANTP_CANIF_FUNC_RX_PDU_ID))
    {
        return;
    }

    u8_Pci  = p_Pdu->data[0];
    u8_Type = u8_Pci >> 4;

    switch (u8_Type)
    {
    case BL_CANTP_PCI_TYPE_FC:
        /* flow control for our outgoing multi-frame TX */
        s_Bl_CanTp_TxOnFc((bl_uint8_t)(u8_Pci & 0x0FU),
                          p_Pdu->data[1], p_Pdu->data[2]);
        break;

    case BL_CANTP_PCI_TYPE_SF:
        u8_Len = (bl_uint8_t)(u8_Pci & 0x0FU);
        if ((u8_Len == 0U) || (u8_Len > BL_CANTP_SF_MAX_DATA_LEN))
        {
            break;  /* invalid single frame */
        }
        /* deliver single frame (transient buffer: consume before return) */
        Bl_CanTp_UpperRxIndication(u16_PduId, &p_Pdu->data[1], u8_Len);
        break;

    case BL_CANTP_PCI_TYPE_FF:
        s_Bl_CanTp_RxStartFf(u16_PduId, p_Pdu);
        break;

    case BL_CANTP_PCI_TYPE_CF:
        s_Bl_CanTp_RxHandleCf(p_Pdu);
        break;

    default:
        break;
    }
}

/**
 * @brief  lower-facing TX confirmation (called by CanIf_TxConfirmation)
 * @param  u16_PduId : CanIf PDU id that completed transmission
 * @retval None
 */
void Bl_CanTp_TxConfirmation(Bl_CanIf_PduIdType u16_PduId)
{
    if ((s_Bl_CanTp_Ready != BL_E_OK) ||
        (u16_PduId != BL_CANTP_CANIF_TX_PDU_ID))
    {
        return;
    }

    switch (s_Bl_CanTp_Tx.u8_State)
    {
    case BL_CANTP_TX_SF_WAIT_CONF:
        s_Bl_CanTp_Tx.u8_State     = BL_CANTP_TX_IDLE;
        s_Bl_CanTp_Tx.u8_TimerKind = BL_CANTP_TX_TIMER_NONE;
        Bl_CanTp_UpperTxConfirmation(BL_CANTP_CANIF_TX_PDU_ID, BL_E_OK);
        break;

    case BL_CANTP_TX_FF_WAIT_CONF:
        /* FF confirmed -> wait for the flow control frame (N_Bs) */
        s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_FF_WAIT_FC;
        s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_BS;
        s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
        break;

    case BL_CANTP_TX_CF_WAIT_CONF:
        s_Bl_CanTp_TxOnCfConfirmed();
        break;

    default:
        /* confirmation of an FC frame or unexpected: ignore */
        break;
    }
}

/**
 * @brief  upper-layer RX indication (weak default no-op; UDS will override)
 * @param  u16_PduId  : CanIf PDU id the SDU belongs to
 * @param  p_Sdu      : pointer to the SDU data
 * @param  u32_SduLen : SDU length in bytes
 * @retval None
 */
__weak void Bl_CanTp_UpperRxIndication(Bl_CanIf_PduIdType u16_PduId,
                                       const bl_uint8_t *p_Sdu,
                                       bl_uint32_t u32_SduLen)
{
    (void)u16_PduId;
    (void)p_Sdu;
    (void)u32_SduLen;
}

/**
 * @brief  upper-layer TX confirmation (weak default no-op; UDS will override)
 * @param  u16_PduId : CanIf PDU id the SDU belongs to
 * @param  u8_Result : BL_E_OK = sent, BL_E_NOT_OK = aborted
 * @retval None
 */
__weak void Bl_CanTp_UpperTxConfirmation(Bl_CanIf_PduIdType u16_PduId,
                                         bl_uint8_t u8_Result)
{
    (void)u16_PduId;
    (void)u8_Result;
}

/**
 * @brief  upper-layer RX error indication (weak default no-op; UDS may override)
 * @param  u16_PduId : CanIf PDU id of the aborted session
 * @retval None
 */
__weak void Bl_CanTp_UpperRxErrorIndication(Bl_CanIf_PduIdType u16_PduId)
{
    (void)u16_PduId;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  send a single frame (PCI: 0x0N)
 * @param  None
 * @retval BL_E_OK / BL_E_NOT_OK
 */
static bl_ret_t s_Bl_CanTp_TxSendSf(void)
{
    bl_uint8_t u8_Frame[8];
    bl_uint8_t u8_Len = (bl_uint8_t)s_Bl_CanTp_Tx.u32_SduLen;
    bl_uint8_t i;

    u8_Frame[0] = u8_Len;   /* PCI: SF + length */
    for (i = 0U; i < u8_Len; i++)
    {
        u8_Frame[1 + i] = s_Bl_CanTp_Tx.p_Sdu[i];
    }

    s_Bl_CanTp_Tx.u32_SentLen = u8_Len;

    return Bl_CanIf_Transmit(BL_CANTP_CANIF_TX_PDU_ID, u8_Frame,
                             (bl_uint8_t)(u8_Len + 1U));
}

/**
 * @brief  send the first frame (PCI: 0x1XX XX + 6 data bytes)
 * @param  None
 * @retval BL_E_OK / BL_E_NOT_OK
 */
static bl_ret_t s_Bl_CanTp_TxSendFf(void)
{
    bl_uint8_t u8_Frame[8];
    bl_uint8_t i;
    bl_uint32_t u32_Len = s_Bl_CanTp_Tx.u32_SduLen;

    u8_Frame[0] = (bl_uint8_t)(0x10U | ((u32_Len >> 8U) & 0x0FU));
    u8_Frame[1] = (bl_uint8_t)(u32_Len & 0xFFU);
    for (i = 0U; i < BL_CANTP_FF_DATA_LEN; i++)
    {
        u8_Frame[2 + i] = s_Bl_CanTp_Tx.p_Sdu[i];
    }

    s_Bl_CanTp_Tx.u32_SentLen = BL_CANTP_FF_DATA_LEN;

    return Bl_CanIf_Transmit(BL_CANTP_CANIF_TX_PDU_ID, u8_Frame, 8U);
}

/**
 * @brief  send the next consecutive frame (PCI: 0x2N)
 * @param  None
 * @retval BL_E_OK / BL_E_NOT_OK
 */
static bl_ret_t s_Bl_CanTp_TxSendCf(void)
{
    bl_uint8_t u8_Frame[8];
    bl_uint8_t i;
    bl_uint32_t u32_Remain = s_Bl_CanTp_Tx.u32_SduLen - s_Bl_CanTp_Tx.u32_SentLen;
    bl_uint8_t u8_N = (u32_Remain >= BL_CANTP_CF_DATA_LEN)
                      ? (bl_uint8_t)BL_CANTP_CF_DATA_LEN : (bl_uint8_t)u32_Remain;

    u8_Frame[0] = (bl_uint8_t)(0x20U | s_Bl_CanTp_Tx.u8_Sn);
    for (i = 0U; i < u8_N; i++)
    {
        u8_Frame[1 + i] = s_Bl_CanTp_Tx.p_Sdu[s_Bl_CanTp_Tx.u32_SentLen + i];
    }

    s_Bl_CanTp_Tx.u32_SentLen += u8_N;
    s_Bl_CanTp_Tx.u8_Sn         = (bl_uint8_t)((s_Bl_CanTp_Tx.u8_Sn + 1U) & 0x0FU);
    s_Bl_CanTp_Tx.u8_CfInBlock++;

    return Bl_CanIf_Transmit(BL_CANTP_CANIF_TX_PDU_ID, u8_Frame,
                             (bl_uint8_t)(u8_N + 1U));
}

/**
 * @brief  send a flow control frame (PCI: 0x3F, BS, STmin)
 * @param  u8_Fs    : flow status (CTS / WAIT / OVF)
 * @param  u8_Bs    : block size
 * @param  u8_Stmin : STmin
 * @retval BL_E_OK / BL_E_NOT_OK
 */
static bl_ret_t s_Bl_CanTp_TxSendFc(bl_uint8_t u8_Fs, bl_uint8_t u8_Bs, bl_uint8_t u8_Stmin)
{
    bl_uint8_t u8_Frame[3];

    u8_Frame[0] = (bl_uint8_t)(0x30U | u8_Fs);
    u8_Frame[1] = u8_Bs;
    u8_Frame[2] = u8_Stmin;

    return Bl_CanIf_Transmit(BL_CANTP_CANIF_TX_PDU_ID, u8_Frame, BL_CANTP_FC_DLC);
}

/**
 * @brief  advance the TX state machine after a CF frame is confirmed
 * @param  None
 * @retval None
 */
static void s_Bl_CanTp_TxOnCfConfirmed(void)
{
    bl_uint8_t u8_StminMs;

    if (s_Bl_CanTp_Tx.u32_SentLen >= s_Bl_CanTp_Tx.u32_SduLen)
    {
        /* all data sent */
        s_Bl_CanTp_Tx.u8_State     = BL_CANTP_TX_IDLE;
        s_Bl_CanTp_Tx.u8_TimerKind = BL_CANTP_TX_TIMER_NONE;
        Bl_CanTp_UpperTxConfirmation(BL_CANTP_CANIF_TX_PDU_ID, BL_E_OK);
        return;
    }

    /* block full: wait for the next flow control frame */
    if ((s_Bl_CanTp_Tx.u8_Bs > 0U) &&
        (s_Bl_CanTp_Tx.u8_CfInBlock >= s_Bl_CanTp_Tx.u8_Bs))
    {
        s_Bl_CanTp_Tx.u8_CfInBlock = 0U;
        s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_CF_WAIT_FC;
        s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_BS;
        s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
        return;
    }

    /* STmin pacing between CFs */
    u8_StminMs = s_Bl_CanTp_Tx.u8_StminMs;
    if (u8_StminMs > 0U)
    {
        s_Bl_CanTp_Tx.u8_State      = BL_CANTP_TX_CF_PACING;
        s_Bl_CanTp_Tx.u32_NextCfTick = Bl_TaskSchedule_GetTickMs() + u8_StminMs;
    }
    else
    {
        (void)s_Bl_CanTp_TxSendCf();
        s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_CF_WAIT_CONF;
        s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_AS;
        s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
    }
}

/**
 * @brief  process a received flow control frame
 * @param  u8_Fs    : flow status
 * @param  u8_Bs    : block size
 * @param  u8_Stmin : STmin (raw encoding)
 * @retval None
 */
static void s_Bl_CanTp_TxOnFc(bl_uint8_t u8_Fs, bl_uint8_t u8_Bs, bl_uint8_t u8_Stmin)
{
    switch (s_Bl_CanTp_Tx.u8_State)
    {
    case BL_CANTP_TX_FF_WAIT_FC:
    case BL_CANTP_TX_CF_WAIT_FC:
        if (u8_Fs == BL_CANTP_FC_CTS)
        {
            s_Bl_CanTp_Tx.u8_Bs       = u8_Bs;
            s_Bl_CanTp_Tx.u8_StminMs  = s_Bl_CanTp_FcStminToMs(u8_Stmin);
            s_Bl_CanTp_Tx.u8_CfInBlock = 0U;
            (void)s_Bl_CanTp_TxSendCf();
            s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_CF_WAIT_CONF;
            s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_AS;
            s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
        }
        else if (u8_Fs == BL_CANTP_FC_WAIT)
        {
            /* keep waiting, restart N_Bs */
            s_Bl_CanTp_Tx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
        }
        else
        {
            /* OVF: abort the transmission */
            s_Bl_CanTp_TxAbort(BL_E_NOT_OK);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief  abort the TX session and notify the upper layer
 * @param  u8_Result : BL_E_NOT_OK for aborted
 * @retval None
 */
static void s_Bl_CanTp_TxAbort(bl_uint8_t u8_Result)
{
    s_Bl_CanTp_Tx.u8_State     = BL_CANTP_TX_IDLE;
    s_Bl_CanTp_Tx.u8_TimerKind = BL_CANTP_TX_TIMER_NONE;
    Bl_CanTp_UpperTxConfirmation(BL_CANTP_CANIF_TX_PDU_ID, u8_Result);
}

/**
 * @brief  TX cyclic: timeout detection + STmin pacing
 * @param  None
 * @retval None
 */
static void s_Bl_CanTp_TxMainFunction(void)
{
    bl_uint32_t u32_Now = Bl_TaskSchedule_GetTickMs();
    bl_uint32_t u32_Timeout;

    if (s_Bl_CanTp_Tx.u8_State == BL_CANTP_TX_IDLE)
    {
        return;
    }

    /* active timeout */
    if (s_Bl_CanTp_Tx.u8_TimerKind != BL_CANTP_TX_TIMER_NONE)
    {
        u32_Timeout = (s_Bl_CanTp_Tx.u8_TimerKind == BL_CANTP_TX_TIMER_AS)
                      ? BL_CANTP_N_AS_TIMEOUT_MS : BL_CANTP_N_BS_TIMEOUT_MS;

        if ((u32_Now - s_Bl_CanTp_Tx.u32_TimerStart) >= u32_Timeout)
        {
            s_Bl_CanTp_TxAbort(BL_E_NOT_OK);
            return;
        }
    }

    /* STmin pacing */
    if (s_Bl_CanTp_Tx.u8_State == BL_CANTP_TX_CF_PACING)
    {
        if ((u32_Now - s_Bl_CanTp_Tx.u32_NextCfTick) < 0x80000000UL)
        {
            (void)s_Bl_CanTp_TxSendCf();
            s_Bl_CanTp_Tx.u8_State       = BL_CANTP_TX_CF_WAIT_CONF;
            s_Bl_CanTp_Tx.u8_TimerKind   = BL_CANTP_TX_TIMER_AS;
            s_Bl_CanTp_Tx.u32_TimerStart = u32_Now;
        }
    }
}

/**
 * @brief  decode the STmin byte into milliseconds (Phase 1: sub-ms ranges = 0)
 * @param  u8_Stmin : raw STmin byte
 * @retval delay in ms
 */
static bl_uint8_t s_Bl_CanTp_FcStminToMs(bl_uint8_t u8_Stmin)
{
    if ((u8_Stmin >= 0x01U) && (u8_Stmin <= 0x7FU))
    {
        return u8_Stmin;    /* 1..127 ms */
    }
    return 0U;              /* 0x00 or µs range (0xF1..0xF9): no delay */
}

/**
 * @brief  start a multi-frame RX session from a first frame
 * @param  p_Pdu : received FF frame
 * @retval None
 */
static void s_Bl_CanTp_RxStartFf(Bl_CanIf_PduIdType u16_PduId, const Bl_Can_PduType *p_Pdu)
{
    bl_uint32_t u32_Len = ((bl_uint32_t)(p_Pdu->data[0] & 0x0FU) << 8U) |
                          (bl_uint32_t)p_Pdu->data[1];
    bl_uint8_t i;

    if (u32_Len < (BL_CANTP_FF_DATA_LEN + 1U))
    {
        return;     /* malformed FF (multi-frame length must be > 7) */
    }

    if (u32_Len > sizeof(BL_DCM_BUFFER_SIZE))
    {
        /* buffer too small: reject with flow control OVF */
        (void)s_Bl_CanTp_TxSendFc(BL_CANTP_FC_OVF, 0U, 0U);
        return;
    }

    /* (re)start the session */
    s_Bl_CanTp_Rx.p_Buf        = BL_DCM_BUFFER_SIZE;
    s_Bl_CanTp_Rx.u32_SduLen   = u32_Len;
    s_Bl_CanTp_Rx.u32_RxLen    = 0U;
    s_Bl_CanTp_Rx.u8_Sn        = 1U;
    s_Bl_CanTp_Rx.u8_CfInBlock = 0U;
    s_Bl_CanTp_Rx.u8_Bs        = BL_CANTP_DEFAULT_BS;
    s_Bl_CanTp_Rx.u16_PduId    = u16_PduId;

    for (i = 0U; i < BL_CANTP_FF_DATA_LEN; i++)
    {
        s_Bl_CanTp_Rx.p_Buf[i] = p_Pdu->data[2 + i];
    }
    s_Bl_CanTp_Rx.u32_RxLen = BL_CANTP_FF_DATA_LEN;

    s_Bl_CanTp_Rx.u8_State       = BL_CANTP_RX_FF;
    s_Bl_CanTp_Rx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();

    /* send flow control: continue, our default BS/STmin */
    (void)s_Bl_CanTp_TxSendFc(BL_CANTP_FC_CTS, BL_CANTP_DEFAULT_BS, BL_CANTP_DEFAULT_STMIN);
}

/**
 * @brief  handle a consecutive frame of the RX session
 * @param  p_Pdu : received CF frame
 * @retval None
 */
static void s_Bl_CanTp_RxHandleCf(const Bl_Can_PduType *p_Pdu)
{
    bl_uint8_t  u8_Sn;
    bl_uint32_t u32_Remain;
    bl_uint8_t  u8_N;
    bl_uint8_t  i;

    if (s_Bl_CanTp_Rx.u8_State != BL_CANTP_RX_FF)
    {
        return;     /* no active session */
    }

    u8_Sn = (bl_uint8_t)(p_Pdu->data[0] & 0x0FU);
    if (u8_Sn != s_Bl_CanTp_Rx.u8_Sn)
    {
        s_Bl_CanTp_RxAbort(BL_E_NOT_OK);    /* sequence error */
        return;
    }

    u32_Remain = s_Bl_CanTp_Rx.u32_SduLen - s_Bl_CanTp_Rx.u32_RxLen;
    u8_N = (u32_Remain >= BL_CANTP_CF_DATA_LEN)
           ? (bl_uint8_t)BL_CANTP_CF_DATA_LEN : (bl_uint8_t)u32_Remain;

    for (i = 0U; i < u8_N; i++)
    {
        s_Bl_CanTp_Rx.p_Buf[s_Bl_CanTp_Rx.u32_RxLen + i] = p_Pdu->data[1 + i];
    }
    s_Bl_CanTp_Rx.u32_RxLen += u8_N;
    s_Bl_CanTp_Rx.u8_Sn         = (bl_uint8_t)((s_Bl_CanTp_Rx.u8_Sn + 1U) & 0x0FU);
    s_Bl_CanTp_Rx.u8_CfInBlock++;

    if (s_Bl_CanTp_Rx.u32_RxLen >= s_Bl_CanTp_Rx.u32_SduLen)
    {
        /* complete: deliver the whole SDU */
        Bl_CanIf_PduIdType u16_PduId = s_Bl_CanTp_Rx.u16_PduId;
        bl_uint8_t        *p_Buf     = s_Bl_CanTp_Rx.p_Buf;
        bl_uint32_t        u32_Len   = s_Bl_CanTp_Rx.u32_SduLen;

        s_Bl_CanTp_Rx.u8_State = BL_CANTP_RX_IDLE;
        Bl_CanTp_UpperRxIndication(u16_PduId, p_Buf, u32_Len);
        return;
    }

    /* block size: re-send FC every BS consecutive frames */
    if ((s_Bl_CanTp_Rx.u8_Bs > 0U) &&
        (s_Bl_CanTp_Rx.u8_CfInBlock >= s_Bl_CanTp_Rx.u8_Bs))
    {
        s_Bl_CanTp_Rx.u8_CfInBlock = 0U;
        (void)s_Bl_CanTp_TxSendFc(BL_CANTP_FC_CTS, BL_CANTP_DEFAULT_BS, BL_CANTP_DEFAULT_STMIN);
    }

    /* restart N_Cr */
    s_Bl_CanTp_Rx.u32_TimerStart = Bl_TaskSchedule_GetTickMs();
}

/**
 * @brief  abort the RX session and notify the upper layer
 * @param  u8_Result : BL_E_NOT_OK for aborted
 * @retval None
 */
static void s_Bl_CanTp_RxAbort(bl_uint8_t u8_Result)
{
    Bl_CanIf_PduIdType u16_PduId = s_Bl_CanTp_Rx.u16_PduId;

    s_Bl_CanTp_Rx.u8_State = BL_CANTP_RX_IDLE;

    if (u8_Result != BL_E_OK)
    {
        Bl_CanTp_UpperRxErrorIndication(u16_PduId);
    }
}

/**
 * @brief  RX cyclic: N_Cr timeout detection
 * @param  None
 * @retval None
 */
static void s_Bl_CanTp_RxMainFunction(void)
{
    if (s_Bl_CanTp_Rx.u8_State != BL_CANTP_RX_FF)
    {
        return;
    }

    if ((Bl_TaskSchedule_GetTickMs() - s_Bl_CanTp_Rx.u32_TimerStart) >= BL_CANTP_N_CR_TIMEOUT_MS)
    {
        s_Bl_CanTp_RxAbort(BL_E_NOT_OK);
    }
}

/******************************* EOF (End of File) ***************************/

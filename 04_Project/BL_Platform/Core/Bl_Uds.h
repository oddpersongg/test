/**
 ******************************************************************************
 * @file    Bl_Uds.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Uds module header file (UDS / ISO 14229 service implementations)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, diagnostic service functions:
 *                       0x10 / 0x11 / 0x27 / 0x34 / 0x36 / 0x37 / 0x3E with
 *                       session, security and download state; response/NRC
 *                       helpers. Bl_Dcm (dispatcher) calls into these.
 *                       [Modify] handler type Bl_Uds_ServiceFunc_t moved to
 *                       the Config layer (Bl_Dcm_Lcfg.h) together with the
 *                       diagnostic service table
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_UDS_H__
#define __BL_UDS_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Uds_Cfg.h"
#include "Bl_CanIf.h"   /* Bl_CanIf_PduIdType (for CanTp upper hooks) */

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/* Note: the service handler type Bl_Uds_ServiceFunc_t and the diagnostic
 * service table Bl_Dcm_Service_t live in the Config layer
 * (Bl_Dcm_Lcfg.h) — the dispatcher config binds SID -> handler here. */

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize the UDS service layer (reset session/security/download state)
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_Uds_Init(void);

/**
 * @brief  get the current diagnostic session
 * @param  None
 * @retval BL_UDS_SESSION_DEFAULT / _PROGRAMMING / _EXTENDED
 */
bl_uint8_t Bl_Uds_GetSession(void);

/**
 * @brief  reset to the default diagnostic session
 * @note   Called by the Dcm layer on S3 session timeout. Clears security
 *         level and any in-progress download (same as a 0x10 01 request
 *         would do), but does not send a response.
 * @param  None
 * @retval None
 */
void Bl_Uds_ResetToDefaultSession(void);

/**
 * @brief  get the current security level (0 = locked)
 * @param  None
 * @retval security level
 */
bl_uint8_t Bl_Uds_GetSecurityLevel(void);

/**
 * @brief  process the response TX queue (driven by Bl_Dcm_MainFunction)
 * @note   AUTOSAR Dcm_MainFunction style: periodically retries transmitting
 *         the head queued response when CanTp is busy (single session).
 *         Also invoked after enqueue and on TxConfirmation.
 * @param  None
 * @retval None
 */
void Bl_Uds_ProcessResponseQueue(void);

/**
 * @brief  send a negative response (0x7F SID NRC)
 * @param  u8_Sid : request SID
 * @param  u8_Nrc : negative response code
 * @retval None
 */
void Bl_Uds_SendNrc(bl_uint8_t u8_Sid, bl_uint8_t u8_Nrc);

/* ---- diagnostic service handlers (called by the Bl_Dcm dispatcher) ---- */

void Bl_Uds_DiagSessionControl(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);  /* 0x10 */

/* 0x10 per-sub-service response functions (referenced by Bl_UdsService_Lcfg.c) */
void Bl_Uds_DiagSessionDefaultResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);
void Bl_Uds_DiagSessionProgrammingResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);
void Bl_Uds_DiagSessionExtendedResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);

void Bl_Uds_ECUReset(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);            /* 0x11 */

/* 0x11 per-sub-service response functions (referenced by Bl_UdsService_Lcfg.c) */
void Bl_Uds_EcuResetHardResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);         /* 01 hardReset */
void Bl_Uds_EcuResetKeyOffOnResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);     /* 02 keyOffOnReset */
void Bl_Uds_EcuResetSoftResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);         /* 03 softReset */
void Bl_Uds_EcuResetFastSoftResp(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);     /* 04 fastSoftReset */

void Bl_Uds_SecurityAccess(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);      /* 0x27 */
void Bl_Uds_RequestDownload(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);     /* 0x34 */
void Bl_Uds_TransferData(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);        /* 0x36 */
void Bl_Uds_RequestTransferExit(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen); /* 0x37 */
void Bl_Uds_TesterPresent(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);       /* 0x3E */

/* ---- CanTp upper-layer hooks overridden by the UDS layer ---- */

/**
 * @brief  CanTp upper-layer TX confirmation (override): flush the next
 *         queued response (back-to-back requests keep their order)
 * @param  u16_PduId : CanIf PDU id of the completed TX
 * @param  u8_Result : BL_E_OK / BL_E_NOT_OK
 * @retval None
 */
void Bl_CanTp_UpperTxConfirmation(Bl_CanIf_PduIdType u16_PduId, bl_uint8_t u8_Result);

/**
 * @brief  CanTp upper-layer RX error indication (override): reset download
 *         state when a multi-frame reception is aborted
 * @param  u16_PduId : CanIf PDU id of the aborted session
 * @retval None
 */
void Bl_CanTp_UpperRxErrorIndication(Bl_CanIf_PduIdType u16_PduId);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UDS_H__ */

/******************************* EOF (End of File) ***************************/

/**
 ******************************************************************************
 * @file    Bl_Dcm_Lcfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Dcm link-time configuration header file
 *          (diagnostic service table: SID -> handler + dispatch metadata)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, diagnostic service table moved
 *                       here from Bl_Dcm.c so services can be added / removed /
 *                       re-configured in the Config layer without touching the
 *                       Core dispatcher logic
 *                       [Modify] sub-function support is no longer a bitmap:
 *                       the Dcm gate resolves supported sub-functions through
 *                       the UdsService sub-service table (Bl_UdsService_Find).
 *                       Dcm table trimmed to discrimination metadata only
 *                       (removed p_SubTable/u8_SubCnt/u8_RespDataLen — the
 *                       latter was never consumed anywhere)
 *                       [Modify] u8_SuppressBit replaced by the
 *                       Bl_Dcm_SuppressBitType enum (ALLOWED / DISALLOWED)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_DCM_LCFG_H__
#define __BL_DCM_LCFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Dcm_Cfg.h"
#include "Bl_Uds_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief number of service table entries */
#define BL_DCM_SERVICE_CNT   7U

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief diagnostic service handler type (implemented in Bl_Uds, invoked by Bl_Dcm)
 * @param  p_Req     : request SDU (points into the Dcm receive buffer)
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
typedef void (*Bl_Uds_ServiceFunc_t)(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);

/**
 * @brief whether a service allows the suppress-positive-response bit
 *        (sub-function byte bit7 = 0x80, ISO 14229 suppressPosRspMsgIndicationBit)
 */
typedef enum
{
    BL_DCM_SUPPRESS_BIT_DISALLOWED = 0U,  /**< bit7 set on the sub-function -> NRC 0x12  */
    BL_DCM_SUPPRESS_BIT_ALLOWED    = 1U   /**< e.g. 3E 80: execute, but send no response */
} Bl_Dcm_SuppressBitType;

/**
 * @brief diagnostic service table entry
 * @note  Dispatch-level discrimination metadata only — the table decides
 *        whether a request may enter the service handler (p_Func). Supported
 *        sub-functions are NOT listed here: the Dcm sub-function gate asks
 *        the UdsService sub-service table (Bl_UdsService_Find) instead, so
 *        each sub-function id is defined in exactly one place.
 */
typedef struct {
    bl_uint8_t           u8_Sid;             /**< service ID (main service byte, e.g. 0x10)           */
    bl_uint8_t                 u8_SubFuncLen;  /**< sub-function byte count. ISO 14229 sub-function is
                                                    ALWAYS 1 byte, so this is 0 (no sub-function) or 1 */
    Bl_Dcm_SuppressBitType     e_SuppressBit;  /**< suppress-positive-response bit (0x80) allowed?   */
    bl_uint8_t           u8_MinDataLen;      /**< data-segment length MINIMUM (data = request length
                                                  minus SID and sub-function). 0 for services with
                                                  no data bytes.                                     */
    bl_uint16_t          u16_MaxDataLen;     /**< data-segment length MAXIMUM; == MinDataLen means the
                                                  data segment is fixed length. 16-bit because 0x36
                                                  blocks can be 2049+ bytes. Protocol range gate only —
                                                  precise per-request validation stays in the handler. */
    bl_uint8_t           u8_SessionMask;     /**< allowed sessions (bit N-1 = session N)              */
    bl_uint8_t           u8_SecurityNeeded;  /**< 1 = requires security access                         */
    bl_uint8_t           u8_P2Override;      /**< P2  override in ms (0xFF = use default from
                                                  Bl_TimingManager). Reserved for the 0x78
                                                  pending-response path (Dcm): when a service
                                                  runs past P2, Dcm sends 0x78 first and the
                                                  final response within P2* — per-service
                                                  overrides are consumed there.                */
    bl_uint8_t           u8_P2StarOverride;  /**< P2* override in ms (0xFF = use default);
                                                  same consumer as u8_P2Override              */
    Bl_Uds_ServiceFunc_t p_Func;             /**< handler in Bl_Uds                                    */
} Bl_Dcm_Service_t;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/** @brief diagnostic service config table (link-time const, defined in Bl_Dcm_Lcfg.c) */
extern const Bl_Dcm_Service_t g_Bl_Dcm_ServiceConfig[BL_DCM_SERVICE_CNT];

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_DCM_LCFG_H__ */

/******************************* EOF (End of File) ***************************/

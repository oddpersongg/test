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
 *                       Bl_Dcm_SuppressBitType enum (ENABLE / DISABLE)
 *                       [Modify] u8_SessionMask replaced by the
 *                       Bl_Dcm_SessionMaskType enum bits (config ORs
 *                       BL_DCM_SESSION_MASK_DEFAULT/PROGRAMMING/EXTENDED)
 *                       [Remove] u8_P2Override/u8_P2StarOverride reserved
 *                       fields (no consumer yet) — to be re-added together
 *                       with the 0x78 pending-response path in
 *                       Bl_Dcm_MainFunction (Bl_TimingManager P2/P2* timers)
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
    BL_DCM_SUPPRESS_BIT_DISABLE = 0U,  /**< bit7 set on the sub-function -> NRC 0x12  */
    BL_DCM_SUPPRESS_BIT_ENABLE  = 1U   /**< e.g. 3E 80: execute, but send no response */
} Bl_Dcm_SuppressBitType;

/**
 * @brief session-mask bits (bit N-1 = session N allowed). Values are mask
 *        bits, NOT session numbers — OR them to build a service's mask,
 *        e.g. (BL_DCM_SESSION_MASK_DEFAULT | BL_DCM_SESSION_MASK_EXTENDED).
 */
typedef enum
{
    BL_DCM_SESSION_MASK_DEFAULT     = 0x01U,  /**< bit0: default session (0x01)     */
    BL_DCM_SESSION_MASK_PROGRAMMING = 0x02U,  /**< bit1: programming session (0x02) */
    BL_DCM_SESSION_MASK_EXTENDED    = 0x04U   /**< bit2: extended session (0x03)    */
} Bl_Dcm_SessionMaskType;

/** @brief all three sessions allowed (shorthand used in the service table) */
#define BL_DCM_SESSION_MASK_ALL \
    (Bl_Dcm_SessionMaskType)(BL_DCM_SESSION_MASK_DEFAULT | \
                             BL_DCM_SESSION_MASK_PROGRAMMING | \
                             BL_DCM_SESSION_MASK_EXTENDED)

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
    bl_uint8_t           u8_SubFuncLen;      /**< sub-function byte count. ISO 14229 sub-function is
                                                  ALWAYS 1 byte, so this is 0 (no sub-function) or 1 */
    Bl_Dcm_SuppressBitType e_SuppressBit;    /**< suppress-positive-response bit (0x80) allowed?     */
    bl_uint8_t           u8_MinDataLen;      /**< data-segment length MINIMUM (data = request length
                                                  minus SID and sub-function). 0 for services with
                                                  no data bytes.                                     */
    bl_uint16_t          u16_MaxDataLen;     /**< data-segment length MAXIMUM; == MinDataLen means the
                                                  data segment is fixed length. 16-bit because 0x36
                                                  blocks can be 2049+ bytes. Protocol range gate only —
                                                  precise per-request validation stays in the handler. */
    Bl_Dcm_SessionMaskType e_SessionMask;    /**< allowed sessions (OR of BL_DCM_SESSION_MASK_* bits) */
    bl_uint8_t           u8_SecurityNeeded;  /**< 1 = requires security access                         */
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

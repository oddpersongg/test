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
 *                       each service now references its sub-service id table
 *                       in Bl_UdsService_Lcfg (p_SubTable + u8_SubCnt) — one
 *                       definition of the supported sub-functions for both
 *                       the Dcm gate and the Uds dispatch
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
#include "Bl_UdsService_Lcfg.h"    /* Bl_UdsService_SubCfg_t (sub-service id tables) */

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
 * @brief diagnostic service table entry
 * @note  Dispatch-level metadata only; the actual service behaviour is
 *        implemented by the handler (Bl_Uds functions).
 */
typedef struct {
    bl_uint8_t           u8_Sid;             /**< service ID (main service byte, e.g. 0x10)           */
    bl_uint8_t           u8_SubFuncLen;      /**< sub-function byte count. ISO 14229 sub-function is
                                                  ALWAYS 1 byte, so this is 0 (no sub-function) or 1 */
    const Bl_UdsService_SubCfg_t *p_SubTable;/**< the service's sub-service id table (single source:
                                                  Bl_UdsService_Lcfg). Used for the 0x12 sub-function
                                                  gate — entries are matched on u8_SubFunc. NULL when
                                                  the service has no sub-function (u8_SubFuncLen==0) */
    bl_uint8_t           u8_SubCnt;          /**< entries in p_SubTable (0 when no sub-function)    */
    bl_uint8_t           u8_SuppressBit;     /**< 1 = sub-function 0x80 (suppress positive response)
                                                  is allowed for this service                        */
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
    bl_uint8_t           u8_RespDataLen;     /**< positive-response data length (excluding SID +
                                                  sub-function); fixed for constant-length services,
                                                  upper bound for variable-length ones                */
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

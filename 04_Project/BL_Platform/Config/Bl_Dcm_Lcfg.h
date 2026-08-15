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
 * @brief diagnostic service table entry
 * @note  Dispatch-level metadata only; the actual service behaviour is
 *        implemented by the handler (Bl_Uds functions).
 */
typedef struct {
    bl_uint8_t           u8_Sid;             /**< service ID (main service byte, e.g. 0x10)           */
    bl_uint8_t           u8_SubFuncLen;      /**< sub-function byte count (0x10 01 -> 1; 0 = none);
                                                  reserved for per-service sub-function handling     */
    bl_uint8_t           u8_SubFuncSupported;/**< sub-function bitmap (bit N = sub-function 0xN,
                                                  up to 0x07); 0xFF = any sub-function accepted      */
    bl_uint8_t           u8_SuppressBit;     /**< 1 = sub-function 0x80 (suppress positive response)
                                                  is allowed for this service                        */
    bl_uint8_t           u8_MinLen;          /**< minimum request length                              */
    bl_uint8_t           u8_SessionMask;     /**< allowed sessions (bit N-1 = session N)              */
    bl_uint8_t           u8_SecurityNeeded;  /**< 1 = requires security access                         */
    bl_uint8_t           u8_P2Override;      /**< P2  override in ms (0xFF = use default from
                                                  Bl_TimingManager; consumed by Dcm for slow services) */
    bl_uint8_t           u8_P2StarOverride;  /**< P2* override in ms (0xFF = use default)              */
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

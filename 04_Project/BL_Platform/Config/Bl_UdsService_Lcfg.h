/**
 ******************************************************************************
 * @file    Bl_UdsService_Lcfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService link-time configuration header file
 *          (centralized UDS sub-service config table)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, centralized UDS sub-service
 *                       table: one table holds every service's sub-functions
 *                       (0x10 sessions, 0x27 security levels, ...). Each entry
 *                       binds a (SID, sub-function) pair to its response
 *                       function; Bl_UdsService finds and dispatches.
 *                       Merged from the former Bl_Uds_DiagSession_Lcfg.
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_UDSSERVICE_LCFG_H__
#define __BL_UDSSERVICE_LCFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Uds_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief number of sub-service table entries (SID+subFunc pairs) */
#define BL_UDSSERVICE_SUB_CNT   3U

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief UDS sub-service response function type
 * @param  p_Req     : request SDU (points into the Dcm receive buffer)
 * @param  u32_ReqLen: request SDU length
 * @retval None
 */
typedef void (*Bl_UdsService_SubFunc_t)(const bl_uint8_t *p_Req,
                                        bl_uint32_t u32_ReqLen);

/**
 * @brief UDS sub-service config entry (one per supported sub-function)
 * @note  Binds a (service SID, sub-function value) pair to its dedicated
 *        response function. Service-specific side-effect flags (reset
 *        security / download on session change, security-level pairing for
 *        0x27, etc.) can be added per service as the table grows.
 */
typedef struct {
    bl_uint8_t              u8_Sid;        /**< service ID (e.g. 0x10)             */
    bl_uint8_t              u8_SubFunc;    /**< sub-function value (0x01/0x02/...) */
    bl_uint8_t              u8_SubFuncName;/**< semantic name/level of the sub-func */
    bl_uint8_t              u8_ResetSecurity; /**< 1 = clear security on this sub  */
    bl_uint8_t              u8_ResetDownload; /**< 1 = clear download on this sub  */
    bl_uint8_t              u8_RespDataLen;   /**< positive-resp data len (excl. SID+sub) */
    Bl_UdsService_SubFunc_t p_Func;            /**< per-sub-service response function    */
} Bl_UdsService_SubCfg_t;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/** @brief centralized UDS sub-service config table (link-time const, defined in Bl_UdsService_Lcfg.c) */
extern const Bl_UdsService_SubCfg_t g_Bl_UdsService_SubConfig[BL_UDSSERVICE_SUB_CNT];

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_UDSSERVICE_LCFG_H__ */

/******************************* EOF (End of File) ***************************/

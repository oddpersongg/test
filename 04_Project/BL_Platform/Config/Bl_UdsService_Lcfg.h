/**
 ******************************************************************************
 * @file    Bl_UdsService_Lcfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService link-time configuration header file
 *          (per-service UDS sub-service config tables + service index)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, per-service UDS sub-service
 *                       tables (0x10 sessions, 0x11 reset types, ...) plus a
 *                       service index table. Each service's sub-functions live
 *                       in its own const table; Bl_UdsService_Find resolves
 *                       (SID, sub) via the index then searches that table.
 *                       Merged from the former Bl_Uds_DiagSession_Lcfg.
 *                       [Modify] split the single big table into per-service
 *                       tables + service index for config isolation and
 *                       readability
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

/** @brief number of services that have sub-service tables (index entries) */
#define BL_UDSSERVICE_SVC_CNT   2U

/** @brief sub-service entry counts per service */
#define BL_UDSSERVICE_DIAGSESSION_SUB_CNT   3U   /**< 0x10 sessions       */
#define BL_UDSSERVICE_ECURESET_SUB_CNT      4U   /**< 0x11 reset types    */

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
 * @note  Binds a sub-function value to its dedicated response function, plus
 *        service-specific side-effect flags (reset security / download on
 *        session change, etc.).
 */
typedef struct {
    bl_uint8_t              u8_SubFunc;        /**< sub-function value (0x01/0x02/...) */
    bl_uint8_t              u8_SubFuncName;    /**< semantic name/level of the sub-func */
    bl_uint8_t              u8_ResetSecurity;  /**< 1 = clear security on this sub      */
    bl_uint8_t              u8_ResetDownload;  /**< 1 = clear download on this sub      */
    bl_uint8_t              u8_RespDataLen;    /**< positive-resp data len (excl. SID+sub) */
    Bl_UdsService_SubFunc_t p_Func;            /**< per-sub-service response function    */
} Bl_UdsService_SubCfg_t;

/**
 * @brief service index entry: maps a SID to its sub-service table
 */
typedef struct {
    bl_uint8_t                  u8_Sid;    /**< service ID (e.g. 0x10)          */
    const Bl_UdsService_SubCfg_t *p_SubTable; /**< this service's sub table      */
    bl_uint8_t                  u8_SubCnt; /**< entries in the sub table        */
} Bl_UdsService_SvcIndex_t;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/** @brief 0x10 session sub-service table (defined in Bl_UdsService_Lcfg.c) */
extern const Bl_UdsService_SubCfg_t g_Bl_UdsService_DiagSessionSubConfig[BL_UDSSERVICE_DIAGSESSION_SUB_CNT];

/** @brief 0x11 reset sub-service table (defined in Bl_UdsService_Lcfg.c) */
extern const Bl_UdsService_SubCfg_t g_Bl_UdsService_EcuResetSubConfig[BL_UDSSERVICE_ECURESET_SUB_CNT];

/** @brief service index table (defined in Bl_UdsService_Lcfg.c) */
extern const Bl_UdsService_SvcIndex_t g_Bl_UdsService_SvcIndex[BL_UDSSERVICE_SVC_CNT];

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_UDSSERVICE_LCFG_H__ */

/******************************* EOF (End of File) ***************************/

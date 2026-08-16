/**
 ******************************************************************************
 * @file    Bl_UdsService.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService module header file
 *          (centralized UDS sub-service lookup & dispatch)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, centralized sub-service lookup
 *                       and dispatch over the Config-layer sub-service table
 *                       (Bl_UdsService_Lcfg.h/.c)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_UDSSERVICE_H__
#define __BL_UDSSERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_UdsService_Lcfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  find a sub-service config entry by (SID, sub-function)
 * @param  u8_Sid : service ID
 * @param  u8_Sub : sub-function value (without the suppress bit)
 * @retval pointer to the config entry, or BL_NULL_PTR if not found
 */
const Bl_UdsService_SubCfg_t *Bl_UdsService_Find(bl_uint8_t u8_Sid,
                                                 bl_uint8_t u8_Sub);

/**
 * @brief  dispatch a request to its sub-service response function
 * @note   Looks up (SID from p_Req[0], sub from p_Req[1]) in the centralized
 *         sub-service table and calls the bound response function. The caller
 *         (a service dispatcher like Bl_Uds_DiagSessionControl) applies any
 *         service-specific side effects before/after via the returned entry.
 * @param  p_Req     : request SDU (SID at [0], sub-function at [1])
 * @param  u32_ReqLen: request SDU length
 * @retval BL_E_OK     : sub-service found and dispatched
 * @retval BL_E_NOT_OK : not found / invalid params (caller replies 0x12)
 */
bl_ret_t Bl_UdsService_Dispatch(const bl_uint8_t *p_Req,
                                bl_uint32_t u32_ReqLen);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UDSSERVICE_H__ */

/******************************* EOF (End of File) ***************************/

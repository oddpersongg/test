/**
 ******************************************************************************
 * @file    Bl_UdsService.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService module source file
 *          (centralized UDS sub-service lookup & dispatch)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, linear search over the
 *                       centralized sub-service config table
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_UdsService.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  find a sub-service config entry by (SID, sub-function)
 * @param  u8_Sid : service ID
 * @param  u8_Sub : sub-function value (without the suppress bit)
 * @retval pointer to the config entry, or BL_NULL_PTR if not found
 */
const Bl_UdsService_SubCfg_t *Bl_UdsService_Find(bl_uint8_t u8_Sid,
                                                 bl_uint8_t u8_Sub)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_UDSSERVICE_SUB_CNT; i++)
    {
        if ((g_Bl_UdsService_SubConfig[i].u8_Sid == u8_Sid) &&
            (g_Bl_UdsService_SubConfig[i].u8_SubFunc == u8_Sub))
        {
            return &g_Bl_UdsService_SubConfig[i];
        }
    }
    return BL_NULL_PTR;
}

/**
 * @brief  dispatch a request to its sub-service response function
 * @param  p_Req     : request SDU (SID at [0], sub-function at [1])
 * @param  u32_ReqLen: request SDU length
 * @retval BL_E_OK     : sub-service found and dispatched
 * @retval BL_E_NOT_OK : not found / invalid params
 */
bl_ret_t Bl_UdsService_Dispatch(const bl_uint8_t *p_Req,
                                bl_uint32_t u32_ReqLen)
{
    const Bl_UdsService_SubCfg_t *p_Cfg;

    if ((p_Req == BL_NULL_PTR) || (u32_ReqLen < 2U))
    {
        return BL_E_NOT_OK;
    }

    p_Cfg = Bl_UdsService_Find(p_Req[0], (bl_uint8_t)(p_Req[1] & 0x7FU));
    if (p_Cfg == BL_NULL_PTR)
    {
        return BL_E_NOT_OK;
    }

    if (p_Cfg->p_Func != BL_NULL_PTR)
    {
        p_Cfg->p_Func(p_Req, u32_ReqLen);
    }

    return BL_E_OK;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

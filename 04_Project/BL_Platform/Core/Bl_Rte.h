/**
 ******************************************************************************
 * @file    Bl_Rte.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_Rte module header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-12   [New] module created, basic functionality implemented
 *                       [New] Bl_Rte_SysInit added, wraps DriverAdapter init
 *                       [New] Bl_Rte_ProcessInit added for post-init task registration
 *                       [New] Bl_Rte_Init added as overall init entry
 *                       [New] Bl_Rte_Deinit added for system de-initialization
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_RTE_H__
#define __BL_RTE_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"

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
 * @brief  overall rte init, calls SysInit, ProcessInit then user tasks
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Rte_Init(void);

/**
 * @brief  overall rte deinit, reverse of Init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Rte_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_RTE_H__ */

/******************************* EOF (End of File) ***************************/

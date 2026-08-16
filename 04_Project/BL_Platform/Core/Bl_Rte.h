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

/**
 * @brief  system reset through the RTE
 * @note   Delegates to the adapter layer (Bl_DriverAdapter_SystemReset) so
 *         Core modules (e.g. Bl_Uds 0x11 ECUReset) never touch chip-specific
 *         reset code directly. The reset type is forwarded verbatim.
 * @param  u8_ResetType : reset type (0x01 hard / 0x02 key-off-on /
 *                        0x03 soft / 0x04 fast-soft)
 * @retval BL_E_OK     : reset issued (never returns on success)
 * @retval BL_E_NOT_OK : reset type not supported by the adapter
 */
bl_ret_t Bl_Rte_SystemReset(bl_uint8_t u8_ResetType);

#ifdef __cplusplus
}
#endif

#endif /* __BL_RTE_H__ */

/******************************* EOF (End of File) ***************************/

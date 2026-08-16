/**
 ******************************************************************************
 * @file    Bl_DriverAdapter.h
 * @author  <author_name>
 * @version V1.0.0
 * @date    2026-08-09
 * @brief   Bl_DriverAdapter module header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-09  [New] Module created
 *                       [New] Deinit added for module de-initialization
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_DRIVER_ADAPTER_H__
#define __BL_DRIVER_ADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
/* User Add */
#include "OLED.h"
/* User Add */

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
 * @brief  module init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_DriverAdapter_Init(void);

/**
 * @brief  module deinit
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_DriverAdapter_Deinit(void);

/**
 * @brief  system reset through the adapter layer (chip-specific)
 * @note   Called by the RTE on UDS 0x11 ECUReset. Chip-specific: the reset
 *         type is mapped to the actual reset mechanism of this hardware.
 *         On STM32F1 all four types collapse to NVIC_SystemReset() (there is
 *         no separate power control for key-off/on); a port with real power
 *         management can differentiate here.
 * @param  u8_ResetType : reset type, 0x01 hard / 0x02 key-off-on /
 *                        0x03 soft / 0x04 fast-soft (UDS 0x11 sub-functions)
 * @retval BL_E_OK     : reset issued (this function never returns on success)
 * @retval BL_E_NOT_OK : reset type not supported
 */
bl_ret_t Bl_DriverAdapter_SystemReset(bl_uint8_t u8_ResetType);

#ifdef __cplusplus
}
#endif

#endif /* __BL_DRIVER_ADAPTER_H__ */

/******************************* EOF (End of File) ***************************/

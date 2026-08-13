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

#ifdef __cplusplus
}
#endif

#endif /* __BL_DRIVER_ADAPTER_H__ */

/******************************* EOF (End of File) ***************************/

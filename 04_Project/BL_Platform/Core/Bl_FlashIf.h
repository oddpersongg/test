/**
 ******************************************************************************
 * @file    Bl_FlashIf.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_FlashIf module header file
 *          (synchronous flash access wrapper over the Fls driver)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-16  [New] module created, synchronous wrapper over the
 *                      AUTOSAR-style Bl_Fls driver: starts the Fls job,
 *                      pumps Fls_MainFunction until it completes, then
 *                      maps the job result to BL_E_OK / BL_E_NOT_OK
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_FLASH_IF_H__
#define __BL_FLASH_IF_H__

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
 * @brief  erase one flash sector synchronously
 * @param  u32_Address : sector-aligned start address
 * @retval BL_E_OK     : sector erased
 * @retval BL_E_NOT_OK : job rejected or failed
 */
bl_ret_t Bl_FlashIf_ErasePage(bl_uint32_t u32_Address);

/**
 * @brief  write data to flash synchronously
 * @param  u32_Address : destination flash address (page aligned)
 * @param  p_Data      : source data pointer (page aligned)
 * @param  u32_Length  : length in bytes (page multiple)
 * @retval BL_E_OK     : written
 * @retval BL_E_NOT_OK : job rejected or failed
 */
bl_ret_t Bl_FlashIf_Write(bl_uint32_t u32_Address,
                          const bl_uint8_t *p_Data,
                          bl_uint32_t u32_Length);

/**
 * @brief  read data from flash synchronously
 * @param  u32_Address : source flash address
 * @param  p_Data      : destination buffer pointer
 * @param  u32_Length  : length in bytes
 * @retval BL_E_OK     : read
 * @retval BL_E_NOT_OK : job rejected or failed
 */
bl_ret_t Bl_FlashIf_Read(bl_uint32_t u32_Address,
                         bl_uint8_t *p_Data,
                         bl_uint32_t u32_Length);

#ifdef __cplusplus
}
#endif

#endif /* __BL_FLASH_IF_H__ */

/******************************* EOF (End of File) ***************************/

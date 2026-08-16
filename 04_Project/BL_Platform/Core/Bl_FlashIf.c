/**
 ******************************************************************************
 * @file    Bl_FlashIf.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_FlashIf module source file
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
 *                      AUTOSAR-style Bl_Fls driver: start job -> pump
 *                      Fls_MainFunction until IDLE -> map job result
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_FlashIf.h"
#include "Bl_Fls.h"     /* adapter-layer AUTOSAR Fls driver (stable interface) */
#include "Bl_Fls_Cfg.h" /* sector size (single-page erase)                    */

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
 * @brief  erase one flash sector synchronously
 * @param  u32_Address : sector-aligned start address
 * @retval BL_E_OK     : sector erased
 * @retval BL_E_NOT_OK : job rejected or failed
 */
bl_ret_t Bl_FlashIf_ErasePage(bl_uint32_t u32_Address)
{
    if (Bl_Fls_Erase(u32_Address, (bl_uint32_t)BL_FLS_SECTOR_SIZE) != BL_E_OK)
    {
        return BL_E_NOT_OK;     /* job rejected (busy / bad params / range) */
    }

    while (Bl_Fls_GetStatus() == BL_FLS_STATUS_BUSY)
    {
        Bl_Fls_MainFunction();
    }

    return (Bl_Fls_GetJobResult() == BL_FLS_JOB_OK) ? BL_E_OK : BL_E_NOT_OK;
}

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
                          bl_uint32_t u32_Length)
{
    if (Bl_Fls_Write(p_Data, u32_Address, u32_Length) != BL_E_OK)
    {
        return BL_E_NOT_OK;     /* job rejected (busy / bad params / range) */
    }

    while (Bl_Fls_GetStatus() == BL_FLS_STATUS_BUSY)
    {
        Bl_Fls_MainFunction();
    }

    return (Bl_Fls_GetJobResult() == BL_FLS_JOB_OK) ? BL_E_OK : BL_E_NOT_OK;
}

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
                         bl_uint32_t u32_Length)
{
    if (Bl_Fls_Read(u32_Address, p_Data, u32_Length) != BL_E_OK)
    {
        return BL_E_NOT_OK;     /* job rejected (busy / bad params / range) */
    }

    while (Bl_Fls_GetStatus() == BL_FLS_STATUS_BUSY)
    {
        Bl_Fls_MainFunction();
    }

    return (Bl_Fls_GetJobResult() == BL_FLS_JOB_OK) ? BL_E_OK : BL_E_NOT_OK;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

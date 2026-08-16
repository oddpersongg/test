/**
 ******************************************************************************
 * @file    Bl_Fls_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Fls pre-compile configuration header file
 *          (AUTOSAR Fls driver config, chip-specific)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-16  [New] module created, AUTOSAR Fls-style config:
 *                      page size (write granularity), sector size, and the
 *                      address range the driver may operate on (the app
 *                      flash area; the bootloader code area is excluded so
 *                      an Erase/Write can never touch the running image)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_FLS_CFG_H__
#define __BL_FLS_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief write granularity (bytes). STM32F1 programs halfwords only. */
#define BL_FLS_PAGE_SIZE                2U

/** @brief sector (page-erase unit) size. STM32F103VE high density: 2KB. */
#define BL_FLS_SECTOR_SIZE              2048U

/** @brief number of entries in the sector table (Bl_Fls.c) */
#define BL_FLS_SECTOR_CNT               1U

/** @brief operating range: app flash area only (sector-aligned).
 *         BL_FLS_BASE_ADDR is the first sector of the app area
 *         (== BL_UDS_APP_FLASH_BASE_ADDR); BL_FLS_END_ADDR is exclusive. */
#define BL_FLS_BASE_ADDR                0x08008000UL
#define BL_FLS_END_ADDR                 0x08080000UL   /* 512KB - 32KB boot */

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_FLS_CFG_H__ */

/******************************* EOF (End of File) ***************************/

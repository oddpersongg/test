/**
 ******************************************************************************
 * @file    Bl_Fls.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Fls module header file
 *          (AUTOSAR Fls driver, asynchronous job model)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-16  [New] module created, AUTOSAR Fls-style internal
 *                      flash driver: asynchronous Erase/Write/Read jobs
 *                      (return immediately, Fls_MainFunction advances the
 *                      job, Fls_GetStatus/Fls_GetJobResult query it), page /
 *                      sector alignment + sector-table address range checks,
 *                      job end/error notifications (weak, overridable)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_FLS_H__
#define __BL_FLS_H__

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

/**
 * @brief  driver status (AUTOSAR MemIf_StatusType semantics)
 * @note   0 = MEMIF_UNINIT, 1 = MEMIF_IDLE, 2 = MEMIF_BUSY,
 *         (MEMIF_BUSY_INTERNAL not used: jobs run to completion in
 *         Fls_MainFunction)
 */
typedef enum
{
    BL_FLS_STATUS_UNINIT = 0U,  /**< driver not initialized            */
    BL_FLS_STATUS_IDLE,         /**< no job running                    */
    BL_FLS_STATUS_BUSY          /**< a job is being processed          */
} Bl_Fls_StatusType;

/**
 * @brief  last job result (AUTOSAR MemIf_JobResultType semantics)
 * @note   0 = MEMIF_JOB_OK, 1 = MEMIF_JOB_FAILED, 2 = MEMIF_JOB_PENDING,
 *         3 = MEMIF_JOB_CANCELED
 */
typedef enum
{
    BL_FLS_JOB_OK = 0U,         /**< last job completed successfully   */
    BL_FLS_JOB_FAILED,          /**< last job failed                   */
    BL_FLS_JOB_PENDING,         /**< job in progress                   */
    BL_FLS_JOB_CANCELED         /**< job was canceled (not implemented) */
} Bl_Fls_JobResultType;

/**
 * @brief  sector table entry (AUTOSAR FlsSector_Type semantics)
 */
typedef struct
{
    bl_uint32_t u32_StartAddr;  /**< sector start address              */
    bl_uint32_t u32_Size;       /**< sector size in bytes              */
} Bl_Fls_SectorType;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize the Fls driver (UNINIT -> IDLE)
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Fls_Init(void);

/**
 * @brief  de-initialize the Fls driver (IDLE -> UNINIT)
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Fls_Deinit(void);

/**
 * @brief  start an erase job (asynchronous, AUTOSAR Fls_Erase)
 * @note   Address and length must be sector-aligned (BL_FLS_SECTOR_SIZE)
 *         and inside the sector table range. The job runs in
 *         Bl_Fls_MainFunction; completion is signaled via
 *         Bl_Fls_JobEndNotification / Bl_Fls_JobErrorNotification.
 * @param  u32_Address : start address of the area to erase
 * @param  u32_Length  : length in bytes (multiple of sector size)
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Erase(bl_uint32_t u32_Address, bl_uint32_t u32_Length);

/**
 * @brief  start a write job (asynchronous, AUTOSAR Fls_Write)
 * @note   Source pointer, destination address and length must be page
 *         aligned (BL_FLS_PAGE_SIZE) and the range must be inside the
 *         sector table. The destination must be erased first.
 * @param  p_Source    : pointer to the source data (RAM)
 * @param  u32_Address : destination flash address
 * @param  u32_Length  : length in bytes (multiple of page size)
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Write(const bl_uint8_t *p_Source,
                      bl_uint32_t u32_Address,
                      bl_uint32_t u32_Length);

/**
 * @brief  start a read job (asynchronous, AUTOSAR Fls_Read)
 * @param  u32_Address : source flash address
 * @param  p_Dest      : pointer to the destination buffer (RAM)
 * @param  u32_Length  : length in bytes
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Read(bl_uint32_t u32_Address,
                     bl_uint8_t *p_Dest,
                     bl_uint32_t u32_Length);

/**
 * @brief  Fls cyclic function: advances the current job to completion
 * @note   Must be called cyclically (scheduler main loop). Executes the
 *         pending Erase/Write/Read synchronously against the hardware,
 *         then returns the driver to IDLE and fires the job notification.
 * @param  None
 * @retval None
 */
void Bl_Fls_MainFunction(void);

/**
 * @brief  get the driver status
 * @param  None
 * @retval BL_FLS_STATUS_UNINIT / _IDLE / _BUSY
 */
Bl_Fls_StatusType Bl_Fls_GetStatus(void);

/**
 * @brief  get the result of the last completed job
 * @param  None
 * @retval BL_FLS_JOB_OK / _FAILED / _PENDING / _CANCELED
 */
Bl_Fls_JobResultType Bl_Fls_GetJobResult(void);

/**
 * @brief  job end notification (weak default; overridable, AUTOSAR
 *         Fls_JobEndNotification). Called when a job completes OK.
 * @param  None
 * @retval None
 */
void Bl_Fls_JobEndNotification(void);

/**
 * @brief  job error notification (weak default; overridable, AUTOSAR
 *         Fls_JobErrorNotification). Called when a job fails.
 * @param  None
 * @retval None
 */
void Bl_Fls_JobErrorNotification(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_FLS_H__ */

/******************************* EOF (End of File) ***************************/

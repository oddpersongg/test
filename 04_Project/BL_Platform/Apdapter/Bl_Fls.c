/**
 ******************************************************************************
 * @file    Bl_Fls.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Fls module source file
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
 *                      flash driver on the STM32F1 HAL: async Erase/Write/
 *                      Read job API (Fls_MainFunction executes the job),
 *                      page/sector alignment + sector-table range checks,
 *                      weak job end/error notifications
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Fls.h"
#include "Bl_Fls_Cfg.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief  kind of job currently being processed
 */
typedef enum
{
    BL_FLS_JOB_NONE = 0U,
    BL_FLS_JOB_ERASE,
    BL_FLS_JOB_WRITE,
    BL_FLS_JOB_READ
} Bl_Fls_JobKindType;

/**
 * @brief  private driver state (single job at a time)
 */
typedef struct
{
    Bl_Fls_StatusType     e_Status;   /**< UNINIT / IDLE / BUSY          */
    Bl_Fls_JobResultType  e_Result;   /**< last job result               */
    Bl_Fls_JobKindType    e_Job;      /**< job in progress               */
    bl_uint32_t           u32_Address;/**< job address                   */
    bl_uint32_t           u32_Length; /**< job length                    */
    const bl_uint8_t     *p_Source;   /**< job source (write)            */
    bl_uint8_t           *p_Dest;     /**< job destination (read)        */
} Bl_Fls_PrivType;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief sector table: the address ranges this driver may operate on.
 *         Only the app flash area is listed — the bootloader code area
 *         (0x08000000..0x08007FFF) is unreachable, so an Erase/Write can
 *         never destroy the running image. */
static const Bl_Fls_SectorType s_Bl_Fls_SectorTable[BL_FLS_SECTOR_CNT] =
{
    { BL_FLS_BASE_ADDR, (BL_FLS_END_ADDR - BL_FLS_BASE_ADDR) }
};

/** @brief private driver state */
static Bl_Fls_PrivType s_Bl_Fls;

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

/**
 * @brief  check whether [u32_Addr, u32_Addr+u32_Len) lies inside the
 *         sector table range
 * @param  u32_Addr : start address
 * @param  u32_Len  : length in bytes
 * @retval 1 : in range, 0 : out of range
 */
static bl_uint8_t s_Bl_Fls_AddrInRange(bl_uint32_t u32_Addr, bl_uint32_t u32_Len);

/**
 * @brief  execute the currently pending job (hardware access)
 * @param  None
 * @retval BL_E_OK     : job finished successfully
 * @retval BL_E_NOT_OK : job failed
 */
static bl_ret_t s_Bl_Fls_ExecJob(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize the Fls driver (UNINIT -> IDLE)
 * @param  None
 * @retval BL_E_OK     : success
 */
bl_ret_t Bl_Fls_Init(void)
{
    s_Bl_Fls.e_Status  = BL_FLS_STATUS_IDLE;
    s_Bl_Fls.e_Result  = BL_FLS_JOB_OK;
    s_Bl_Fls.e_Job     = BL_FLS_JOB_NONE;
    s_Bl_Fls.u32_Address = 0U;
    s_Bl_Fls.u32_Length  = 0U;
    s_Bl_Fls.p_Source    = BL_NULL_PTR;
    s_Bl_Fls.p_Dest      = BL_NULL_PTR;

    return BL_E_OK;
}

/**
 * @brief  de-initialize the Fls driver (IDLE -> UNINIT)
 * @param  None
 * @retval BL_E_OK     : success
 */
bl_ret_t Bl_Fls_Deinit(void)
{
    s_Bl_Fls.e_Status = BL_FLS_STATUS_UNINIT;

    return BL_E_OK;
}

/**
 * @brief  start an erase job (asynchronous)
 * @param  u32_Address : start address (sector aligned)
 * @param  u32_Length  : length in bytes (sector multiple)
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Erase(bl_uint32_t u32_Address, bl_uint32_t u32_Length)
{
    if (s_Bl_Fls.e_Status != BL_FLS_STATUS_IDLE)
    {
        return BL_E_NOT_OK;     /* busy or not initialized */
    }
    if ((u32_Length == 0U) ||
        (u32_Address % BL_FLS_SECTOR_SIZE != 0U) ||
        (u32_Length % BL_FLS_SECTOR_SIZE != 0U))
    {
        return BL_E_NOT_OK;     /* not sector aligned */
    }
    if (s_Bl_Fls_AddrInRange(u32_Address, u32_Length) == 0U)
    {
        return BL_E_NOT_OK;     /* outside the sector table */
    }

    s_Bl_Fls.e_Job        = BL_FLS_JOB_ERASE;
    s_Bl_Fls.u32_Address  = u32_Address;
    s_Bl_Fls.u32_Length   = u32_Length;
    s_Bl_Fls.e_Result     = BL_FLS_JOB_PENDING;
    s_Bl_Fls.e_Status     = BL_FLS_STATUS_BUSY;

    return BL_E_OK;
}

/**
 * @brief  start a write job (asynchronous)
 * @param  p_Source    : pointer to the source data (RAM)
 * @param  u32_Address : destination flash address
 * @param  u32_Length  : length in bytes
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Write(const bl_uint8_t *p_Source,
                      bl_uint32_t u32_Address,
                      bl_uint32_t u32_Length)
{
    if (s_Bl_Fls.e_Status != BL_FLS_STATUS_IDLE)
    {
        return BL_E_NOT_OK;     /* busy or not initialized */
    }
    if ((p_Source == BL_NULL_PTR) ||
        (u32_Length == 0U) ||
        (u32_Length % BL_FLS_PAGE_SIZE != 0U) ||
        ((bl_uint32_t)p_Source % BL_FLS_PAGE_SIZE != 0U) ||
        (u32_Address % BL_FLS_PAGE_SIZE != 0U))
    {
        return BL_E_NOT_OK;     /* not page aligned / null source */
    }
    if (s_Bl_Fls_AddrInRange(u32_Address, u32_Length) == 0U)
    {
        return BL_E_NOT_OK;     /* outside the sector table */
    }

    s_Bl_Fls.e_Job        = BL_FLS_JOB_WRITE;
    s_Bl_Fls.p_Source     = p_Source;
    s_Bl_Fls.u32_Address  = u32_Address;
    s_Bl_Fls.u32_Length   = u32_Length;
    s_Bl_Fls.e_Result     = BL_FLS_JOB_PENDING;
    s_Bl_Fls.e_Status     = BL_FLS_STATUS_BUSY;

    return BL_E_OK;
}

/**
 * @brief  start a read job (asynchronous)
 * @param  u32_Address : source flash address
 * @param  p_Dest      : pointer to the destination buffer (RAM)
 * @param  u32_Length  : length in bytes
 * @retval BL_E_OK     : job started
 * @retval BL_E_NOT_OK : invalid parameter / busy / not initialized
 */
bl_ret_t Bl_Fls_Read(bl_uint32_t u32_Address,
                     bl_uint8_t *p_Dest,
                     bl_uint32_t u32_Length)
{
    if (s_Bl_Fls.e_Status != BL_FLS_STATUS_IDLE)
    {
        return BL_E_NOT_OK;     /* busy or not initialized */
    }
    if ((p_Dest == BL_NULL_PTR) || (u32_Length == 0U))
    {
        return BL_E_NOT_OK;
    }
    if (s_Bl_Fls_AddrInRange(u32_Address, u32_Length) == 0U)
    {
        return BL_E_NOT_OK;     /* outside the sector table */
    }

    s_Bl_Fls.e_Job        = BL_FLS_JOB_READ;
    s_Bl_Fls.p_Dest       = p_Dest;
    s_Bl_Fls.u32_Address  = u32_Address;
    s_Bl_Fls.u32_Length   = u32_Length;
    s_Bl_Fls.e_Result     = BL_FLS_JOB_PENDING;
    s_Bl_Fls.e_Status     = BL_FLS_STATUS_BUSY;

    return BL_E_OK;
}

/**
 * @brief  Fls cyclic function: advances the current job to completion
 * @param  None
 * @retval None
 */
void Bl_Fls_MainFunction(void)
{
    bl_ret_t e_Ok;

    if (s_Bl_Fls.e_Status != BL_FLS_STATUS_BUSY)
    {
        return;
    }

    e_Ok = s_Bl_Fls_ExecJob();

    s_Bl_Fls.e_Job        = BL_FLS_JOB_NONE;
    s_Bl_Fls.p_Source     = BL_NULL_PTR;
    s_Bl_Fls.p_Dest       = BL_NULL_PTR;
    s_Bl_Fls.e_Status     = BL_FLS_STATUS_IDLE;
    s_Bl_Fls.e_Result     = (e_Ok == BL_E_OK) ? BL_FLS_JOB_OK : BL_FLS_JOB_FAILED;

    /* AUTOSAR job notification */
    if (e_Ok == BL_E_OK)
    {
        Bl_Fls_JobEndNotification();
    }
    else
    {
        Bl_Fls_JobErrorNotification();
    }
}

/**
 * @brief  get the driver status
 * @param  None
 * @retval BL_FLS_STATUS_UNINIT / _IDLE / _BUSY
 */
Bl_Fls_StatusType Bl_Fls_GetStatus(void)
{
    return s_Bl_Fls.e_Status;
}

/**
 * @brief  get the result of the last completed job
 * @param  None
 * @retval BL_FLS_JOB_OK / _FAILED / _PENDING / _CANCELED
 */
Bl_Fls_JobResultType Bl_Fls_GetJobResult(void)
{
    return s_Bl_Fls.e_Result;
}

/**
 * @brief  job end notification (weak default; overridable)
 * @param  None
 * @retval None
 */
__weak void Bl_Fls_JobEndNotification(void)
{
}

/**
 * @brief  job error notification (weak default; overridable)
 * @param  None
 * @retval None
 */
__weak void Bl_Fls_JobErrorNotification(void)
{
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  check whether [u32_Addr, u32_Addr+u32_Len) lies inside the
 *         sector table range (overflow-safe comparison)
 * @param  u32_Addr : start address
 * @param  u32_Len  : length in bytes
 * @retval 1 : in range, 0 : out of range
 */
static bl_uint8_t s_Bl_Fls_AddrInRange(bl_uint32_t u32_Addr, bl_uint32_t u32_Len)
{
    bl_uint8_t i;

    for (i = 0U; i < BL_FLS_SECTOR_CNT; i++)
    {
        if ((u32_Addr >= s_Bl_Fls_SectorTable[i].u32_StartAddr) &&
            (u32_Addr <= (s_Bl_Fls_SectorTable[i].u32_StartAddr +
                          s_Bl_Fls_SectorTable[i].u32_Size)) &&
            (u32_Len <= (s_Bl_Fls_SectorTable[i].u32_StartAddr +
                         s_Bl_Fls_SectorTable[i].u32_Size - u32_Addr)))
        {
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief  execute the currently pending job (hardware access)
 * @note   STM32F1: erases via HAL_FLASHEx_Erase (page erase), writes via
 *         HAL_FLASH_Program (halfword). Both block until the operation
 *         finishes; the CPU stalls on internal flash access during a
 *         program/erase cycle (interrupts are deferred, not lost).
 * @param  None
 * @retval BL_E_OK     : job finished successfully
 * @retval BL_E_NOT_OK : job failed
 */
static bl_ret_t s_Bl_Fls_ExecJob(void)
{
    bl_ret_t e_Ret = BL_E_OK;
    bl_uint32_t u32_Offset;
    FLASH_EraseInitTypeDef s_Erase;
    bl_uint32_t u32_ErrorPage;

    switch (s_Bl_Fls.e_Job)
    {
    case BL_FLS_JOB_ERASE:
        s_Erase.TypeErase   = FLASH_TYPEERASE_PAGES;
        s_Erase.PageAddress = s_Bl_Fls.u32_Address;
        s_Erase.NbPages     = (bl_uint32_t)(s_Bl_Fls.u32_Length / BL_FLS_SECTOR_SIZE);
        u32_ErrorPage       = 0U;

        if (HAL_FLASH_Unlock() != HAL_OK)
        {
            e_Ret = BL_E_NOT_OK;
            break;
        }
        if (HAL_FLASHEx_Erase(&s_Erase, &u32_ErrorPage) != HAL_OK)
        {
            e_Ret = BL_E_NOT_OK;
        }
        (void)HAL_FLASH_Lock();
        break;

    case BL_FLS_JOB_WRITE:
        if (HAL_FLASH_Unlock() != HAL_OK)
        {
            e_Ret = BL_E_NOT_OK;
            break;
        }
        for (u32_Offset = 0U; u32_Offset < s_Bl_Fls.u32_Length;
             u32_Offset += BL_FLS_PAGE_SIZE)
        {
            bl_uint16_t u16_Val;

            u16_Val = (bl_uint16_t)((bl_uint16_t)s_Bl_Fls.p_Source[u32_Offset] |
                                    ((bl_uint16_t)s_Bl_Fls.p_Source[u32_Offset + 1U] << 8U));
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                  (bl_uint32_t)(s_Bl_Fls.u32_Address + u32_Offset),
                                  (bl_uint32_t)u16_Val) != HAL_OK)
            {
                e_Ret = BL_E_NOT_OK;
                break;
            }
        }
        (void)HAL_FLASH_Lock();
        break;

    case BL_FLS_JOB_READ:
        (void)memcpy(s_Bl_Fls.p_Dest, (const void *)s_Bl_Fls.u32_Address,
                     s_Bl_Fls.u32_Length);
        break;

    default:
        e_Ret = BL_E_NOT_OK;
        break;
    }

    return e_Ret;
}

/******************************* EOF (End of File) ***************************/

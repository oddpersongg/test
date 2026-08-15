/**
 ******************************************************************************
 * @file    Bl_Dcm.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-13
 * @brief   Bl_Dcm module header file (Dcm dispatcher, ISO 14229)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-13   [New] module created, download buffer reserved;
 *                       V1.0.0 (2026-08-16) dispatcher role: discriminates the
 *                       service (SID) and calls the handler implemented in
 *                       Bl_Uds; keeps the download / reassembly buffer
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_DCM_H__
#define __BL_DCM_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_Dcm_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/**
 * @brief download receive buffer (holds one TransferData block + protocol overhead)
 * @note  Large static RAM buffer where a received TransferData (0x36) block
 *        is placed before being written to flash. It doubles as the CanTp
 *        reassembly buffer (zero-copy). Data starts at offset 2 (SID + block
 *        counter); buffer length = BL_DCM_BUFFER_LEN = block + 2.
 */
extern bl_uint8_t BL_DCM_BUFFER_SIZE[BL_DCM_BUFFER_LEN];

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_DCM_H__ */

/******************************* EOF (End of File) ***************************/

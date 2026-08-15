/**
 ******************************************************************************
 * @file    Bl_Dcm_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-13
 * @brief   Bl_Dcm pre-compile configuration header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-13   [New] module created, pre-compile config
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_DCM_CFG_H__
#define __BL_DCM_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
/* TransferData block size is a UDS protocol constant (owned by Bl_Uds_Cfg.h) */
#include "Bl_Uds_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/**
 * @brief protocol overhead of the largest download request (0x36)
 * @note  SID (1) + block sequence counter (1), not part of the block data.
 */
#define BL_DCM_TRANSFER_OVERHEAD           2U

/**
 * @brief download / reassembly buffer length in bytes
 * @note  Must fit the largest incoming diagnostic request SDU, i.e. one full
 *        0x36 block plus its protocol overhead (2048 + 2 = 2050). The buffer
 *        doubles as the CanTp reassembly buffer (zero-copy).
 */
#define BL_DCM_BUFFER_LEN                  (BL_UDS_TRANSFER_BLOCK_SIZE + BL_DCM_TRANSFER_OVERHEAD)

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

#endif /* __BL_DCM_CFG_H__ */

/******************************* EOF (End of File) ***************************/

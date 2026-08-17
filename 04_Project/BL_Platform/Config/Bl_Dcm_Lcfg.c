/**
 ******************************************************************************
 * @file    Bl_Dcm_Lcfg.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_Dcm link-time configuration source file
 *          (diagnostic service table definition)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, diagnostic service table
 *                       (7 services: 0x10/0x11/0x27/0x34/0x36/0x37/0x3E)
 *                       moved here from Bl_Dcm.c
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Dcm_Lcfg.h"
#include "Bl_Uds.h"     /* handler declarations (implemented in Core/Bl_Uds.c) */

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

/**
 * @brief diagnostic service config table (SID lookup order is irrelevant)
 * @note  Edit this table to add / remove / re-configure services. Each entry
 *        is split over two lines: (SID, subFuncLen, suppressBit) then
 *        (minDataLen, maxDataLen, sessionMask, securityNeeded, handler).
 *        Supported sub-functions are NOT configured here — the Dcm 0x12 gate
 *        asks the UdsService sub-service table (Bl_UdsService_Find).
 */
const Bl_Dcm_Service_t g_Bl_Dcm_ServiceConfig[BL_DCM_SERVICE_CNT] =
{
    /* SID                          subLen  suppressBit                    */
    /*  minData maxData  sessionMask        sec  handler                   */
    { BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, 1U, BL_DCM_SUPPRESS_BIT_DISABLE,
      0U, 0U, BL_DCM_SESSION_MASK_ALL, 0U, Bl_Uds_DiagSessionControl },

    { BL_UDS_SID_ECU_RESET,                  1U, BL_DCM_SUPPRESS_BIT_DISABLE,
      0U, 0U, BL_DCM_SESSION_MASK_ALL, 0U, Bl_Uds_ECUReset },

    { BL_UDS_SID_SECURITY_ACCESS,            1U, BL_DCM_SUPPRESS_BIT_DISABLE,
      0U, 8U, BL_DCM_SESSION_MASK_ALL, 0U, Bl_Uds_SecurityAccess },

    { BL_UDS_SID_REQUEST_DOWNLOAD,           0U, BL_DCM_SUPPRESS_BIT_DISABLE,
      3U, 10U, BL_DCM_SESSION_MASK_PROGRAMMING, 1U, Bl_Uds_RequestDownload },

    /* 0x36 data segment = block sequence counter (1) + payload (1..2048):
       max = TRANSFER_BLOCK_SIZE + 1, linked to the configured block size
       (buffer = block + 2 overhead, Dcm buffer holds the whole request) */
    { BL_UDS_SID_TRANSFER_DATA,              0U, BL_DCM_SUPPRESS_BIT_DISABLE,
      2U, (bl_uint16_t)(BL_UDS_TRANSFER_BLOCK_SIZE + 1U), BL_DCM_SESSION_MASK_PROGRAMMING, 1U, Bl_Uds_TransferData },

    { BL_UDS_SID_REQUEST_TRANSFER_EXIT,      0U, BL_DCM_SUPPRESS_BIT_DISABLE,
      0U, 0U, BL_DCM_SESSION_MASK_PROGRAMMING, 1U, Bl_Uds_RequestTransferExit },

    { BL_UDS_SID_TESTER_PRESENT,             1U, BL_DCM_SUPPRESS_BIT_ENABLE,
      0U, 0U, BL_DCM_SESSION_MASK_ALL, 0U, Bl_Uds_TesterPresent },
};

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

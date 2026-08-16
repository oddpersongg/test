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
 * @note  Edit this table to add / remove / re-configure services. Columns:
 *        SID | subFuncLen(0/1) | subFuncSupported(bit N = sub 0xN, 0xFF=any)
 *        | suppressBit | minDataLen | maxDataLen (data = request minus SID
 *        and sub-function; min==max => fixed length) | sessionMask(bit0
 *        default | bit1 prog | bit2 ext) | securityNeeded | P2Override |
 *        P2StarOverride | respDataLen | handler
 */
const Bl_Dcm_Service_t g_Bl_Dcm_ServiceConfig[BL_DCM_SERVICE_CNT] =
{
    /* SID    subLen subMask  supp  minData maxData  sessionMask sec  P2ovr P2*ovr respData handler */
    { BL_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, 1U, 0x0EU, 0U, 0U, 0U,      0x07U, 0U, 0xFFU, 0xFFU, 4U, Bl_Uds_DiagSessionControl },
    { BL_UDS_SID_ECU_RESET,                  1U, 0x1EU, 0U, 0U, 0U,      0x07U, 0U, 0xFFU, 0xFFU, 1U, Bl_Uds_ECUReset },
    { BL_UDS_SID_SECURITY_ACCESS,            1U, 0x06U, 0U, 0U, 8U,     0x07U, 0U, 0xFFU, 0xFFU, 1U, Bl_Uds_SecurityAccess },
    { BL_UDS_SID_REQUEST_DOWNLOAD,           0U, 0x00U, 0U, 3U, 10U,    0x02U, 1U, 0xFFU, 0xFFU, 2U, Bl_Uds_RequestDownload },
    { BL_UDS_SID_TRANSFER_DATA,              0U, 0x00U, 0U, 2U, 2049U,  0x02U, 1U, 0xFFU, 0xFFU, 1U, Bl_Uds_TransferData },
    { BL_UDS_SID_REQUEST_TRANSFER_EXIT,      0U, 0x00U, 0U, 0U, 0U,     0x02U, 1U, 0xFFU, 0xFFU, 0U, Bl_Uds_RequestTransferExit },
    { BL_UDS_SID_TESTER_PRESENT,             1U, 0x01U, 1U, 0U, 0U,     0x07U, 0U, 0xFFU, 0xFFU, 0U, Bl_Uds_TesterPresent },
};

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

/**
 ******************************************************************************
 * @file    Bl_UdsService_Lcfg.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService link-time configuration source file
 *          (centralized UDS sub-service config table definition)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, centralized UDS sub-service
 *                       table (0x10: default/programming/extended sessions)
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_UdsService_Lcfg.h"
#include "Bl_Uds.h"     /* sub-service response function declarations */

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
 * @brief centralized UDS sub-service config table
 * @note  Edit this table to add / remove / re-configure sub-functions for any
 *        service. Each entry: sid | subFunc | subFuncName | resetSecurity |
 *        resetDownload | respDataLen | response function.
 *        0x10: session sub-functions.
 */
const Bl_UdsService_SubCfg_t g_Bl_UdsService_SubConfig[BL_UDSSERVICE_SUB_CNT] =
{
    /* sid  sub  name                   resetSec resetDl respDataLen func                            */
    { 0x10U, 0x01U, BL_UDS_SESSION_DEFAULT,    1U,   1U,    4U, Bl_Uds_DiagSessionDefaultResp },
    { 0x10U, 0x02U, BL_UDS_SESSION_PROGRAMMING, 1U,   1U,    4U, Bl_Uds_DiagSessionProgrammingResp },
    { 0x10U, 0x03U, BL_UDS_SESSION_EXTENDED,    1U,   1U,    4U, Bl_Uds_DiagSessionExtendedResp },
};

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

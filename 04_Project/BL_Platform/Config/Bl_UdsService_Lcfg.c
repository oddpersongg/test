/**
 ******************************************************************************
 * @file    Bl_UdsService_Lcfg.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_UdsService link-time configuration source file
 *          (per-service UDS sub-service config tables + service index)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, per-service sub-service tables:
 *                       0x10 sessions (default/programming/extended) and
 *                       0x11 reset types (hard/keyOffOn/soft/fastSoft), plus
 *                       the service index table
 *                       [Modify] added 0x27 security-access (seed/key) and
 *                       0x3E tester-present tables; index now covers all four
 *                       sub-function-carrying services (Dcm references these
 *                       tables as the single source of sub-function ids)
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
 * @brief 0x10 DiagnosticSessionControl sub-service table
 * @note  Edit to add / remove / re-configure session sub-functions.
 *        Columns: subFunc | name | resetSecurity | resetDownload |
 *        response function.
 */
const Bl_UdsService_SubCfg_t g_Bl_UdsService_DiagSessionSubConfig[BL_UDSSERVICE_DIAGSESSION_SUB_CNT] =
{
    /* sub  name                   resetSec resetDl func                  */
    { 0x01U, BL_UDS_SESSION_DEFAULT,    1U,   1U,    Bl_Uds_DiagSessionResp },
    { 0x02U, BL_UDS_SESSION_PROGRAMMING, 1U,   1U,    Bl_Uds_DiagSessionResp },
    { 0x03U, BL_UDS_SESSION_EXTENDED,    1U,   1U,    Bl_Uds_DiagSessionResp },
};

/**
 * @brief 0x11 ECUReset sub-service table
 * @note  Edit to add / remove / re-configure reset types.
 */
const Bl_UdsService_SubCfg_t g_Bl_UdsService_EcuResetSubConfig[BL_UDSSERVICE_ECURESET_SUB_CNT] =
{
    /* sub  name  resetSec resetDl func               */
    { 0x01U, 0x01U,  0U,   0U,    Bl_Uds_EcuResetResp },
    { 0x02U, 0x02U,  0U,   0U,    Bl_Uds_EcuResetResp },
    { 0x03U, 0x03U,  0U,   0U,    Bl_Uds_EcuResetResp },
    { 0x04U, 0x04U,  0U,   0U,    Bl_Uds_EcuResetResp },
};

/**
 * @brief 0x27 SecurityAccess sub-service table
 * @note  Sub-function gate only (single source of sub ids for the Dcm
 *        dispatcher): the 0x27 handler in Bl_Uds still implements the
 *        seed/key logic, so p_Func stays NULL here.
 */
const Bl_UdsService_SubCfg_t g_Bl_UdsService_SecurityAccessSubConfig[BL_UDSSERVICE_SECURITYACCESS_SUB_CNT] =
{
    /* sub  name  resetSec resetDl func  */
    { 0x01U, 0x01U,  0U,   0U,    BL_NULL_PTR },  /* request seed */
    { 0x02U, 0x02U,  0U,   0U,    BL_NULL_PTR },  /* send key     */
};

/**
 * @brief 0x3E TesterPresent sub-service table
 * @note  Sub-function gate only; the 0x3E handler in Bl_Uds handles the
 *        suppress-positive-response bit, p_Func stays NULL here.
 */
const Bl_UdsService_SubCfg_t g_Bl_UdsService_TesterPresentSubConfig[BL_UDSSERVICE_TESTERPRESENT_SUB_CNT] =
{
    /* sub  name  resetSec resetDl func  */
    { 0x00U, 0x00U,  0U,   0U,    BL_NULL_PTR },
};

/**
 * @brief service index table: SID -> its sub-service table
 * @note  Edit to register a new service's sub-service table.
 */
const Bl_UdsService_SvcIndex_t g_Bl_UdsService_SvcIndex[BL_UDSSERVICE_SVC_CNT] =
{
    /* sid  sub-table                              cnt  */
    { 0x10U, g_Bl_UdsService_DiagSessionSubConfig,     BL_UDSSERVICE_DIAGSESSION_SUB_CNT },
    { 0x11U, g_Bl_UdsService_EcuResetSubConfig,        BL_UDSSERVICE_ECURESET_SUB_CNT },
    { 0x27U, g_Bl_UdsService_SecurityAccessSubConfig,  BL_UDSSERVICE_SECURITYACCESS_SUB_CNT },
    { 0x3EU, g_Bl_UdsService_TesterPresentSubConfig,   BL_UDSSERVICE_TESTERPRESENT_SUB_CNT },
};

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

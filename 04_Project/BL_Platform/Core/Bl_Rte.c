/**
 ******************************************************************************
 * @file    Bl_Rte.c
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_Rte module source file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, RTE init/deinit glue: driver
 *                       adapter (SysInit/Deinit) + scheduler + TimingManager
 *                       + CanIf + CanTp + Uds (ProcessInit/Deinit) + user
 *                       tasks, reverse order
 *                       [New] Bl_Rte_SystemReset added: RTE entry that
 *                       delegates the UDS 0x11 reset to the adapter layer
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Rte.h"
#include "Bl_DriverAdapter.h"
#include "Bl_TaskSchedule.h"
#include "Bl_TaskUserdef.h"
#include "Bl_CanIf.h"
#include "Bl_CanTp.h"
#include "Bl_Uds.h"
#include "Bl_TimingManager.h"

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

static bl_ret_t s_Bl_Rte_SysInit(void);
static bl_ret_t s_Bl_Rte_SysDeinit(void);
static bl_ret_t s_Bl_Rte_ProcessInit(void);
static bl_ret_t s_Bl_Rte_ProcessDeinit(void);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  overall rte init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Rte_Init(void)
{
    bl_ret_t e_Ret;

    e_Ret  = s_Bl_Rte_SysInit();
    e_Ret |= s_Bl_Rte_ProcessInit();

    Bl_TaskUserdef_Init();

    return e_Ret;
}

/**
 * @brief  overall rte deinit, reverse of Init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
bl_ret_t Bl_Rte_Deinit(void)
{
    bl_ret_t e_Ret;

    Bl_TaskUserdef_Deinit();

    e_Ret  = s_Bl_Rte_ProcessDeinit();
    e_Ret |= s_Bl_Rte_SysDeinit();

    return e_Ret;
}

/**
 * @brief  system reset through the RTE (delegates to the adapter layer)
 * @param  u8_ResetType : reset type, forwarded verbatim
 * @retval BL_E_OK     : reset issued (never returns on success)
 * @retval BL_E_NOT_OK : reset type not supported by the adapter
 */
bl_ret_t Bl_Rte_SystemReset(bl_uint8_t u8_ResetType)
{
    return Bl_DriverAdapter_SystemReset(u8_ResetType);
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  system init, wraps DriverAdapter init
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_Rte_SysInit(void)
{
    bl_ret_t e_Ret;

    e_Ret  = Bl_DriverAdapter_Init();

    return e_Ret;
}

/**
 * @brief  system deinit, wraps DriverAdapter deinit
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_Rte_SysDeinit(void)
{
    bl_ret_t e_Ret;

    e_Ret  = Bl_DriverAdapter_Deinit();

    return e_Ret;
}

/**
 * @brief  process init, init task scheduler
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_Rte_ProcessInit(void)
{
    bl_ret_t e_Ret = BL_E_OK;

    Bl_TaskSchedule_Init();
    e_Ret |= Bl_TimingManager_Init();
    e_Ret |= Bl_CanIf_Init();
    e_Ret |= Bl_CanTp_Init();
    e_Ret |= Bl_Uds_Init();

    return e_Ret;
}

/**
 * @brief  process deinit, deinit task scheduler
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
static bl_ret_t s_Bl_Rte_ProcessDeinit(void)
{
    /* TODO: add scheduler deinit when available */
    (void)Bl_CanIf_Deinit();
    (void)Bl_CanTp_Deinit();

    return BL_E_OK;
}

/******************************* EOF (End of File) ***************************/

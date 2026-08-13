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
 * V1.0.0   2026-08-12   [New] module created, basic functionality implemented
 *                       [New] Bl_Rte_Init / Bl_Rte_Deinit added
 *                       [Modify] SysInit / ProcessInit became static
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Rte.h"
#include "Bl_DriverAdapter.h"
#include "Bl_TaskSchedule.h"
#include "Bl_TaskUserdef.h"

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
    Bl_TaskSchedule_Init();

    return BL_E_OK;
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

    return BL_E_OK;
}

/******************************* EOF (End of File) ***************************/

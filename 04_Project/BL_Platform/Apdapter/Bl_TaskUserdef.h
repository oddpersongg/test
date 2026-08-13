/**
 ******************************************************************************
 * @file    Bl_TaskUserdef.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-12
 * @brief   Bl_TaskUserdef module header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-12   [New] module created, basic functionality implemented
 *                       [New] Bl_TaskUserdef_Deinit added to clear all tasks
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_TASKUSERDEF_H__
#define __BL_TASKUSERDEF_H__

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

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  user-defined task init, register application tasks
 * @param  None
 * @retval BL_E_OK     : success
 * @retval BL_E_NOT_OK : failure
 */
void Bl_TaskUserdef_Init(void);

/**
 * @brief  user-defined task deinit, unregister all application tasks
 * @param  None
 * @retval None
 */
void Bl_TaskUserdef_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_TASKUSERDEF_H__ */

/******************************* EOF (End of File) ***************************/

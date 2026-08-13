/**
 ******************************************************************************
 * @file    Bl_Can.c
 * @author  LENOVO
 * @version V1.0.0
 * @date    2026-08-11
 * @brief   Bl_Can module source file (AUTOSAR CAN Driver adapter)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-11   [New] module created, AUTOSAR Can driver skeleton
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Can.h"
/* User Add */
#include "stm32f1xx_hal.h"
/* User Add */

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

/** @brief Can module global config */
Bl_Can_ConfigType g_Bl_Can_Config;

/****************************************************************
 *              Global Function Definitions
 ***************************************************************/

/**
 * @brief  initialize CAN module
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_Can_Init(void)
{
    /* User Add */

    /* User Add */
    return BL_E_OK;
}

/**
 * @brief  de-initialize CAN module
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_Can_DeInit(void)
{
    /* User Add */

    /* User Add */
    return BL_E_OK;
}

/**
 * @brief  write (transmit) a CAN L-PDU via specified HOH
 * @param  hoh      : TX Hardware Object Handle
 * @param  pPduInfo : pointer to L-PDU to transmit
 * @retval BL_E_OK     : TX request accepted
 * @retval BL_E_NOT_OK : TX request rejected
 * @retval BL_CAN_BUSY : HOH busy
 */
bl_ret_t Bl_Can_Write(Bl_Can_HohType hoh,
                      const Bl_Can_PduType *pPduInfo)
{
    /* User Add */

    /* User Add */
    (void)hoh;
    (void)pPduInfo;
    return BL_E_OK;
}

/**
 * @brief  main function for CAN TX / RX processing (cyclic call)
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunction(void)
{
    /* User Add */

    /* User Add */
}

/**
 * @brief  TX confirmation (called from TX interrupt or MainFunction)
 * @param  hoh : HOH that completed transmission
 * @retval None
 */
void Bl_Can_TxConfirmation(Bl_Can_HohType hoh)
{
    /* User Add */

    /* User Add */
    (void)hoh;
}

/**
 * @brief  RX indication (called from RX interrupt or MainFunction)
 * @param  hoh      : HOH that received a message
 * @param  pPduInfo : pointer to received L-PDU
 * @retval None
 */
void Bl_Can_RxIndication(Bl_Can_HohType hoh,
                         const Bl_Can_PduType *pPduInfo)
{
    /* User Add */

    /* User Add */
    (void)hoh;
    (void)pPduInfo;
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/******************************* EOF (End of File) ***************************/

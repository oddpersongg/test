/**
 ******************************************************************************
 * @file    Bl_Can.h
 * @author  LENOVO
 * @version V1.0.0
 * @date    2026-08-11
 * @brief   Bl_Can module header file (AUTOSAR CAN Driver adapter)
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
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CAN_H__
#define __BL_CAN_H__

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

/** @brief module version */
#define BL_CAN_VERSION_MAJOR             1
#define BL_CAN_VERSION_MINOR             0
#define BL_CAN_VERSION_PATCH             0

/** @brief CAN payload length (max DLC = 8) */
#define BL_CAN_PDU_DATA_LENGTH           8

/** @brief CAN HOH (Hardware Object Handle) count */
#define BL_CAN_MAX_HOH                   32

/** @brief CAN busy status (extended from BL_E_OK / BL_E_NOT_OK) */
#define BL_CAN_BUSY                      2

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief CAN Hardware Object Handle (maps to HTH/HRH)
 */
typedef bl_uint16_t Bl_Can_HohType;

/**
 * @brief CAN identifier (standard 11-bit or extended 29-bit)
 */
typedef bl_uint32_t Bl_Can_CanIdType;

/**
 * @brief CAN hardware handle (for PDU-based filtering)
 */
typedef bl_uint16_t Bl_Can_HwHandleType;

/**
 * @brief CAN L-PDU (data link layer protocol data unit)
 */
typedef struct {
    Bl_Can_CanIdType    id;              /**< CAN identifier               */
    Bl_Can_HwHandleType swPduHandle;     /**< SW PDU handle (rx only)      */
    bl_uint8_t          dlc;             /**< data length code [0..8]      */
    bl_uint8_t          data[BL_CAN_PDU_DATA_LENGTH]; /**< payload data    */
} Bl_Can_PduType;

/**
 * @brief CAN module global config (config-tool generated)
 */
typedef struct {
    bl_uint32_t placeholder;             /**< config placeholder, to be expanded */
} Bl_Can_ConfigType;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

extern Bl_Can_ConfigType g_Bl_Can_Config;

/****************************************************************
 *                Function Declarations
 ***************************************************************/

/**
 * @brief  initialize CAN module
 * @param  None
 * @retval BL_E_OK     : init succeeded
 * @retval BL_E_NOT_OK : init failed
 */
bl_ret_t Bl_Can_Init(void);

/**
 * @brief  de-initialize CAN module
 * @param  None
 * @retval BL_E_OK     : deinit succeeded
 * @retval BL_E_NOT_OK : deinit failed
 */
bl_ret_t Bl_Can_DeInit(void);

/**
 * @brief  write (transmit) a CAN L-PDU via specified HOH
 * @param  hoh      : TX Hardware Object Handle
 * @param  pPduInfo : pointer to L-PDU to transmit
 * @retval BL_E_OK     : TX request accepted
 * @retval BL_E_NOT_OK : TX request rejected
 * @retval BL_CAN_BUSY : HOH busy
 */
bl_ret_t Bl_Can_Write(Bl_Can_HohType hoh,
                      const Bl_Can_PduType *pPduInfo);

/**
 * @brief  main function for CAN TX / RX processing (cyclic call)
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunction(void);

/**
 * @brief  TX confirmation (called from TX interrupt)
 * @param  hoh : HOH that completed transmission
 * @retval None
 */
void Bl_Can_TxConfirmation(Bl_Can_HohType hoh);

/**
 * @brief  RX indication (called from RX interrupt)
 * @param  hoh      : HOH that received a message
 * @param  pPduInfo : pointer to received L-PDU
 * @retval None
 */
void Bl_Can_RxIndication(Bl_Can_HohType hoh,
                         const Bl_Can_PduType *pPduInfo);

#ifdef __cplusplus
}
#endif

#endif /* __BL_CAN_H__ */

/******************************* EOF (End of File) ***************************/

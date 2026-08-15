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
 * V1.0.0   2026-08-15   [New] module created, AUTOSAR-style CAN driver interface:
 *                       Write / MainFunctions / GetRxOverflow; driver calls
 *                       CanIf_RxIndication / CanIf_TxConfirmation (AUTOSAR MCAL)
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
#include "Bl_Can_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief module version */
#define BL_CAN_VERSION_MAJOR             1
#define BL_CAN_VERSION_MINOR             0
#define BL_CAN_VERSION_PATCH             0

/** @brief CAN payload length (max DLC = 8) */
#define BL_CAN_PDU_DATA_LENGTH           8

/** @brief CAN busy status (extended from BL_E_OK / BL_E_NOT_OK) */
#define BL_CAN_BUSY                      2

/** @brief invalid HOH / software PDU handle */
#define BL_CAN_INVALID_HOH               0xFFFFU

/**
 * @brief CAN identifier encoding (AUTOSAR Can_IdType)
 *        bit31 = IDE (1 = extended), bit30 = RTR (1 = remote), bit28..0 = ID
 */
#define BL_CAN_IDE_FLAG                  0x80000000UL
#define BL_CAN_RTR_FLAG                  0x40000000UL
#define BL_CAN_ID_MASK                   0x1FFFFFFFUL

/**
 * @brief HOH (Hardware Object Handle) type
 */
#define BL_CAN_HOH_TYPE_TX               0U      /**< TX hardware object   */
#define BL_CAN_HOH_TYPE_RX               1U      /**< RX hardware object   */

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief CAN Hardware Object Handle (maps to HTH/HRH)
 */
typedef bl_uint16_t Bl_Can_HohType;

/**
 * @brief CAN identifier (standard 11-bit or extended 29-bit, AUTOSAR encoded)
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
 * @brief CAN HOH (Hardware Object Handle) configuration
 *
 * - TX HOH : u8_HwObj / id / mask are unused (mailbox auto selected by HAL)
 * - RX HOH : u8_HwObj = RX FIFO (0/1), id/mask define the acceptance filter
 *            (mask = 0 means "accept all")
 */
typedef struct {
    Bl_Can_HohType   hoh;               /**< HOH id, unique                 */
    bl_uint8_t       u8_Type;           /**< BL_CAN_HOH_TYPE_TX / _RX       */
    bl_uint8_t       u8_HwObj;          /**< RX: FIFO 0/1                   */
    Bl_Can_CanIdType id;                /**< RX: filter id (AUTOSAR encoded)*/
    Bl_Can_CanIdType mask;              /**< RX: filter mask                */
} Bl_Can_HohConfig_t;

/**
 * @brief CAN module global config (config-tool generated)
 */
typedef struct {
    bl_uint8_t                u8_HohCount;    /**< HOH config entry count  */
    const Bl_Can_HohConfig_t *p_HohConfig;    /**< HOH config array        */
} Bl_Can_ConfigType;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/** @brief Can module global config (defined in Bl_Can.c) */
extern const Bl_Can_ConfigType g_Bl_Can_Config;

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
 * @brief  write (enqueue) a CAN L-PDU for transmission
 * @note   This only enqueues the L-PDU into the software TX queue and returns.
 *         The real transmission is performed in Bl_Can_MainFunctionWrite().
 * @param  hoh      : TX Hardware Object Handle
 * @param  pPduInfo : pointer to L-PDU to transmit
 * @retval BL_E_OK     : TX request accepted (enqueued)
 * @retval BL_E_NOT_OK : TX request rejected (invalid HOH or queue full)
 * @retval BL_CAN_BUSY : HOH already has a pending TX request
 */
bl_ret_t Bl_Can_Write(Bl_Can_HohType hoh,
                      const Bl_Can_PduType *pPduInfo);

/**
 * @brief  CAN MainFunction write (cyclic TX processing)
 * @note   Must be called cyclically (e.g. from the task scheduler).
 *         - dequeues TX requests and submits them to hardware mailboxes
 *         - polls TX mailboxes and raises TxConfirmation on completion
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionWrite(void);

/**
 * @brief  CAN MainFunction read (cyclic RX processing)
 * @note   Must be called cyclically (e.g. from the task scheduler).
 *         - drains the software RX queue and raises RxIndication per frame
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionRead(void);

/**
 * @brief  CAN RX interrupt service routine (called from HAL RX callbacks)
 * @note   Reads the specified RX FIFO and pushes frames into the software RX queue.
 *         Called in interrupt context.
 * @param  u8_Fifo : RX FIFO number (0/1)
 * @retval None
 */
void Bl_Can_RxIsr(bl_uint8_t u8_Fifo);

/**
 * @brief  CAN MainFunction bus-off (bus-off recovery processing)
 * @note   Cyclic software bus-off recovery: detects ESR.BOF, stops the CAN,
 *         waits BL_CAN_BUSOFF_RECOVERY_TIME_MS, then restarts it.
 *         See Bl_Can.c. Disabled by setting BL_CAN_BUSOFF_RECOVERY_ENABLE to 0.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionBusOff(void);

/**
 * @brief  CAN MainFunction mode (mode transition processing)
 * @note   Not implemented yet, reserved for AUTOSAR-style mode handling.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionMode(void);

/**
 * @brief  CAN MainFunction wakeup (wakeup processing)
 * @note   Not implemented yet, reserved for AUTOSAR-style wakeup handling.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionWakeup(void);

/**
 * @brief  get RX queue overflow counter (frames dropped by the software RX queue)
 * @note   Incremented in ISR context when the software RX queue is full.
 *         Reset by Bl_Can_Init() / Bl_Can_DeInit().
 * @param  None
 * @retval number of frames dropped because the RX queue was full
 */
bl_uint16_t Bl_Can_GetRxOverflow(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_CAN_H__ */

/******************************* EOF (End of File) ***************************/

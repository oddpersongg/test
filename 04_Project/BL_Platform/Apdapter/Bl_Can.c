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
 * V1.0.0   2026-08-15   [New] module created, AUTOSAR-style CAN driver: async
 *                       Write (SW TX queue 8), MainFunctionWrite/Read (SW RX ring
 *                       128 + ISR), MainFunctionBusOff software recovery,
 *                       Bl_Can_GetRxOverflow; driver calls CanIf_RxIndication /
 *                       CanIf_TxConfirmation (AUTOSAR MCAL symbols)
 */

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Can.h"
#include "Bl_CanIf.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"

/** @brief CAN peripheral handle (defined in main.c, low-level init in stm32f1xx_hal_msp.c) */
extern CAN_HandleTypeDef hcan;

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief TX queue slot states */
#define BL_CAN_TX_SLOT_FREE            0U
#define BL_CAN_TX_SLOT_READY           1U
#define BL_CAN_TX_SLOT_PENDING         2U

/** @brief standard CAN id bit mask (11 bits) */
#define BL_CAN_STD_ID_MASK             0x7FFU

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief software TX queue entry (software transmit buffer)
 */
typedef struct {
    bl_uint8_t    u8_State;      /**< FREE / READY / PENDING              */
    Bl_Can_HohType hoh;          /**< TX HOH of this request              */
    Bl_Can_PduType s_Pdu;        /**< L-PDU to transmit                   */
    bl_uint32_t   u32_TxMailbox; /**< hardware mailbox when PENDING       */
} Bl_Can_TxSlot_t;

/**
 * @brief software RX queue entry (software receive buffer)
 */
typedef struct {
    Bl_Can_PduType s_Pdu;        /**< received L-PDU                     */
    bl_uint8_t     u8_Fifo;      /**< source RX FIFO (0/1)               */
} Bl_Can_RxSlot_t;

/****************************************************************
 *                   Static Variables
 ***************************************************************/

/** @brief software TX queue */
static Bl_Can_TxSlot_t s_Bl_Can_TxQueue[BL_CAN_TX_QUEUE_DEPTH];

/** @brief init complete flag */
static bl_uint8_t s_Bl_Can_Ready = BL_E_NOT_OK;

/** @brief software RX queue (ISR producer / main-loop consumer) */
static Bl_Can_RxSlot_t s_Bl_Can_RxQueue[BL_CAN_RX_QUEUE_DEPTH];

/** @brief RX queue write index (ISR only) */
static volatile bl_uint8_t s_Bl_Can_RxHead = 0U;

/** @brief RX queue read index (main loop only) */
static volatile bl_uint8_t s_Bl_Can_RxTail = 0U;

/** @brief RX queue overflow counter (frames dropped, ISR only) */
static volatile bl_uint16_t s_Bl_Can_RxOverflow = 0U;

#if (BL_CAN_BUSOFF_RECOVERY_ENABLE == 1U)
/** @brief bus-off recovery state: 0 = normal, 1 = recovering (stopped, waiting to restart) */
static bl_uint8_t s_Bl_Can_BusOffState = 0U;

/** @brief bus-off recovery start tick (HAL_GetTick), valid while recovering */
static bl_uint32_t s_Bl_Can_BusOffStartTick = 0U;
#endif

/** @brief HOH configuration table (1 TX handle + 1 RX handle bound to FIFO0) */
static const Bl_Can_HohConfig_t s_Bl_Can_HohConfig[] =
{
    /* hoh, type,             hwObj, id,  mask */
    { 0U,  BL_CAN_HOH_TYPE_TX, 0U,   0U,  0U },
    { 1U,  BL_CAN_HOH_TYPE_RX, 0U,   0U,  0U },
};

/****************************************************************
 *             Static Function Declarations
 ***************************************************************/

static bl_uint32_t s_Bl_Can_BuildFilterReg(Bl_Can_CanIdType id);
static const Bl_Can_HohConfig_t *s_Bl_Can_FindHoh(Bl_Can_HohType hoh);
static const Bl_Can_HohConfig_t *s_Bl_Can_FindTxHoh(Bl_Can_HohType hoh);
static const Bl_Can_HohConfig_t *s_Bl_Can_FindRxHoh(bl_uint8_t u8_Fifo);
static void s_Bl_Can_PduToTxHeader(const Bl_Can_PduType *p_Pdu,
                                   CAN_TxHeaderTypeDef *p_Header);
static void s_Bl_Can_RxHeaderToPdu(const CAN_RxHeaderTypeDef *p_Header,
                                   const bl_uint8_t p_Data[BL_CAN_PDU_DATA_LENGTH],
                                   Bl_Can_PduType *p_Pdu);
static void s_Bl_Can_ProcessTx(void);
static void s_Bl_Can_ProcessTxConfirmation(void);
static void s_Bl_Can_RxPush(const Bl_Can_PduType *p_Pdu, bl_uint8_t u8_Fifo);

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/**
 * @brief CAN module configuration
 * @note  Hardware init (clock/GPIO/timing) is done by CubeMX MX_CAN_Init().
 *        Here we only configure the HOH (RX filter) mapping.
 */
const Bl_Can_ConfigType g_Bl_Can_Config =
{
    .u8_HohCount = (bl_uint8_t)(sizeof(s_Bl_Can_HohConfig) / sizeof(s_Bl_Can_HohConfig[0])),
    .p_HohConfig = s_Bl_Can_HohConfig,
};

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
    const Bl_Can_ConfigType *p_CanInfo = &g_Bl_Can_Config;
    bl_uint8_t u8_Bank = 0U;
    bl_uint8_t i;

    /* hcan.Instance / hcan.Init are configured by CubeMX MX_CAN_Init() in main.c.
       Bl_Can (adapter) invokes HAL_CAN_Init to own the high-level init. */
    if (HAL_CAN_Init(&hcan) != HAL_OK)
    {
        return BL_E_NOT_OK;
    }

    /* ---- configure RX acceptance filters (one bank per RX HOH) ---- */
    for (i = 0U; i < p_CanInfo->u8_HohCount; i++)
    {
        const Bl_Can_HohConfig_t *p_Hoh = &p_CanInfo->p_HohConfig[i];

        if (p_Hoh->u8_Type == BL_CAN_HOH_TYPE_RX)
        {
            CAN_FilterTypeDef s_Filter;
            bl_uint32_t u32_IdReg;
            bl_uint32_t u32_MaskReg;

            if (u8_Bank >= BL_CAN_MAX_FILTER_BANK)
            {
                break;
            }

            u32_IdReg   = s_Bl_Can_BuildFilterReg(p_Hoh->id);
            u32_MaskReg = s_Bl_Can_BuildFilterReg(p_Hoh->mask);

            s_Filter.FilterActivation     = CAN_FILTER_ENABLE;
            s_Filter.FilterBank           = u8_Bank;
            s_Filter.FilterFIFOAssignment = (p_Hoh->u8_HwObj == 0U) ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
            s_Filter.FilterMode           = CAN_FILTERMODE_IDMASK;
            s_Filter.FilterScale          = CAN_FILTERSCALE_32BIT;
            s_Filter.FilterIdHigh         = (u32_IdReg >> 16U) & 0xFFFFU;
            s_Filter.FilterIdLow          = u32_IdReg & 0xFFFFU;
            s_Filter.FilterMaskIdHigh     = (u32_MaskReg >> 16U) & 0xFFFFU;
            s_Filter.FilterMaskIdLow      = u32_MaskReg & 0xFFFFU;
            s_Filter.SlaveStartFilterBank = 0U;

            (void)HAL_CAN_ConfigFilter(&hcan, &s_Filter);
            u8_Bank++;
        }
    }

    /* ---- clear software TX queue ---- */
    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        s_Bl_Can_TxQueue[i].u8_State = BL_CAN_TX_SLOT_FREE;
        s_Bl_Can_TxQueue[i].hoh      = BL_CAN_INVALID_HOH;
    }

    /* ---- clear software RX queue ---- */
    s_Bl_Can_RxHead     = 0U;
    s_Bl_Can_RxTail     = 0U;
    s_Bl_Can_RxOverflow = 0U;

#if (BL_CAN_BUSOFF_RECOVERY_ENABLE == 1U)
    /* ---- reset bus-off recovery state ---- */
    s_Bl_Can_BusOffState     = 0U;
    s_Bl_Can_BusOffStartTick = 0U;
#endif

    /* ---- start CAN ---- */
    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        return BL_E_NOT_OK;
    }

    /* ---- enable RX message-pending interrupts (FIFO0 + FIFO1) ---- */
    if (HAL_CAN_ActivateNotification(&hcan,
                                     CAN_IT_RX_FIFO0_MSG_PENDING |
                                     CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK)
    {
        return BL_E_NOT_OK;
    }

    /* ---- enable NVIC for CAN1 RX0 / RX1 ---- */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);

    s_Bl_Can_Ready = BL_E_OK;

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
    bl_uint8_t i;

    if (s_Bl_Can_Ready != BL_E_OK)
    {
        return BL_E_OK;
    }

    s_Bl_Can_Ready = BL_E_NOT_OK;

    /* disable RX interrupts */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    (void)HAL_CAN_DeactivateNotification(&hcan,
                                         CAN_IT_RX_FIFO0_MSG_PENDING |
                                         CAN_IT_RX_FIFO1_MSG_PENDING);

    /* clear software TX queue */
    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        s_Bl_Can_TxQueue[i].u8_State = BL_CAN_TX_SLOT_FREE;
        s_Bl_Can_TxQueue[i].hoh      = BL_CAN_INVALID_HOH;
    }

    /* clear software RX queue */
    s_Bl_Can_RxHead     = 0U;
    s_Bl_Can_RxTail     = 0U;
    s_Bl_Can_RxOverflow = 0U;

#if (BL_CAN_BUSOFF_RECOVERY_ENABLE == 1U)
    /* reset bus-off recovery state */
    s_Bl_Can_BusOffState     = 0U;
    s_Bl_Can_BusOffStartTick = 0U;
#endif

    if (HAL_CAN_Stop(&hcan) != HAL_OK)
    {
        return BL_E_NOT_OK;
    }

    return BL_E_OK;
}

/**
 * @brief  get RX queue overflow counter (frames dropped by the software RX queue)
 * @note   The counter is incremented in ISR context when the software RX queue
 *         is full (main loop fell behind). Read anytime; reset by
 *         Bl_Can_Init() / Bl_Can_DeInit().
 * @param  None
 * @retval number of frames dropped because the RX queue was full
 */
bl_uint16_t Bl_Can_GetRxOverflow(void)
{
    return s_Bl_Can_RxOverflow;
}

/**
 * @brief  write (enqueue) a CAN L-PDU for transmission
 * @note   This only enqueues the L-PDU into the software TX queue and returns.
 *         The real transmission is performed in Bl_Can_MainFunctionWrite().
 * @param  hoh      : TX Hardware Object Handle
 * @param  pPduInfo : pointer to L-PDU to transmit
 * @retval BL_E_OK     : TX request accepted (enqueued)
 * @retval BL_E_NOT_OK : TX request rejected (invalid HOH / param / queue full)
 * @retval BL_CAN_BUSY : HOH already has a pending TX request
 */
bl_ret_t Bl_Can_Write(Bl_Can_HohType hoh,
                      const Bl_Can_PduType *pPduInfo)
{
    const Bl_Can_HohConfig_t *p_Hoh;
    bl_uint8_t i;

    if (s_Bl_Can_Ready != BL_E_OK)
    {
        return BL_E_NOT_OK;
    }

    p_Hoh = s_Bl_Can_FindTxHoh(hoh);
    if ((p_Hoh == BL_NULL_PTR) || (pPduInfo == BL_NULL_PTR))
    {
        return BL_E_NOT_OK;
    }

    /* HOH already has a queued / pending request */
    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        if ((s_Bl_Can_TxQueue[i].u8_State != BL_CAN_TX_SLOT_FREE) &&
            (s_Bl_Can_TxQueue[i].hoh == hoh))
        {
            return BL_CAN_BUSY;
        }
    }

    /* find a free queue slot and enqueue */
    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        if (s_Bl_Can_TxQueue[i].u8_State == BL_CAN_TX_SLOT_FREE)
        {
            s_Bl_Can_TxQueue[i].hoh           = hoh;
            s_Bl_Can_TxQueue[i].s_Pdu         = *pPduInfo;
            s_Bl_Can_TxQueue[i].u32_TxMailbox = 0U;
            s_Bl_Can_TxQueue[i].u8_State      = BL_CAN_TX_SLOT_READY;
            return BL_E_OK;
        }
    }

    return BL_E_NOT_OK;
}

/**
 * @brief  CAN MainFunction write (cyclic TX processing)
 * @note   Must be called cyclically (e.g. from the task scheduler).
 *         - dequeues TX requests and submits them to hardware mailboxes
 *         - polls TX mailboxes and raises TxConfirmation on completion
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionWrite(void)
{
    if (s_Bl_Can_Ready != BL_E_OK)
    {
        return;
    }

    s_Bl_Can_ProcessTx();
    s_Bl_Can_ProcessTxConfirmation();
}

/**
 * @brief  CAN MainFunction read (cyclic RX processing)
 * @note   Must be called cyclically (e.g. from the task scheduler).
 *         - drains the software RX queue and raises RxIndication per frame
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionRead(void)
{
    Bl_Can_PduType s_Pdu;
    bl_uint8_t u8_Fifo;
    const Bl_Can_HohConfig_t *p_Hoh;

    if (s_Bl_Can_Ready != BL_E_OK)
    {
        return;
    }

    while (s_Bl_Can_RxHead != s_Bl_Can_RxTail)
    {
        s_Pdu   = s_Bl_Can_RxQueue[s_Bl_Can_RxTail].s_Pdu;
        u8_Fifo = s_Bl_Can_RxQueue[s_Bl_Can_RxTail].u8_Fifo;
        s_Bl_Can_RxTail = (bl_uint8_t)((s_Bl_Can_RxTail + 1U) % BL_CAN_RX_QUEUE_DEPTH);

        p_Hoh = s_Bl_Can_FindRxHoh(u8_Fifo);
        if (p_Hoh != BL_NULL_PTR)
        {
            /* AUTOSAR: Can driver calls the CanIf RX indication */
            CanIf_RxIndication(p_Hoh->hoh, &s_Pdu);
        }
    }
}

/**
 * @brief  CAN MainFunction bus-off (bus-off recovery processing)
 * @note   Cyclic software bus-off recovery (AUTOSAR-style). Detects the
 *         bus-off state via ESR.BOFF, stops the CAN (enter init mode, which
 *         also resets the hardware error counters TEC/REC), waits
 *         BL_CAN_BUSOFF_RECOVERY_TIME_MS, then restarts it. After restart
 *         the bxCAN itself waits 128 x 11 recessive bits before rejoining
 *         the bus. Must be called cyclically (e.g. from the task scheduler).
 *         Disabled by setting BL_CAN_BUSOFF_RECOVERY_ENABLE to 0.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionBusOff(void)
{
    if (s_Bl_Can_Ready != BL_E_OK)
    {
        return;
    }

#if (BL_CAN_BUSOFF_RECOVERY_ENABLE == 1U)
    if (s_Bl_Can_BusOffState == 0U)
    {
        /* normal: watch for bus-off (ESR.BOFF set) */
        if ((hcan.Instance->ESR & CAN_ESR_BOFF) != 0U)
        {
            s_Bl_Can_BusOffState     = 1U;
            s_Bl_Can_BusOffStartTick = HAL_GetTick();

            /* enter init mode: halt the peripheral, hardware resets TEC/REC */
            (void)HAL_CAN_Stop(&hcan);
        }
    }
    else
    {
        /* recovering: wait the configured recovery time, then restart */
        if ((HAL_GetTick() - s_Bl_Can_BusOffStartTick) >= BL_CAN_BUSOFF_RECOVERY_TIME_MS)
        {
            (void)HAL_CAN_ResetError(&hcan);

            if (HAL_CAN_Start(&hcan) == HAL_OK)
            {
                s_Bl_Can_BusOffState = 0U;
            }
        }
    }
#endif
}

/**
 * @brief  CAN MainFunction mode (mode transition processing)
 * @note   Not implemented yet, reserved for AUTOSAR-style mode handling.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionMode(void)
{
    /* TODO: not implemented */
}

/**
 * @brief  CAN MainFunction wakeup (wakeup processing)
 * @note   Not implemented yet, reserved for AUTOSAR-style wakeup handling.
 * @param  None
 * @retval None
 */
void Bl_Can_MainFunctionWakeup(void)
{
    /* TODO: not implemented */
}

/**
 * @brief  CAN RX interrupt service routine (called from HAL RX callbacks)
 * @note   Reads the specified RX FIFO and pushes frames into the software RX queue.
 *         Called in interrupt context.
 * @param  u8_Fifo : RX FIFO number (0/1)
 * @retval None
 */
void Bl_Can_RxIsr(bl_uint8_t u8_Fifo)
{
    bl_uint32_t u32_Fifo;
    CAN_RxHeaderTypeDef s_Header;
    bl_uint8_t p_Data[BL_CAN_PDU_DATA_LENGTH];
    Bl_Can_PduType s_Pdu;

    u32_Fifo = (u8_Fifo == 0U) ? CAN_RX_FIFO0 : CAN_RX_FIFO1;

    /* read all pending frames (hardware FIFO depth 3 bounds this loop) */
    while (HAL_CAN_GetRxFifoFillLevel(&hcan, u32_Fifo) > 0U)
    {
        if (HAL_CAN_GetRxMessage(&hcan, u32_Fifo, &s_Header, p_Data) != HAL_OK)
        {
            break;
        }

        s_Bl_Can_RxHeaderToPdu(&s_Header, p_Data, &s_Pdu);
        s_Bl_Can_RxPush(&s_Pdu, u8_Fifo);
    }
}

/****************************************************************
 *              Static Function Definitions
 ***************************************************************/

/**
 * @brief  build a 32-bit CAN filter register value from an AUTOSAR Can_IdType
 * @param  id : AUTOSAR encoded CAN id (IDE/RTR/identifier)
 * @retval 32-bit filter register value (CAN_FxR1 layout)
 */
static bl_uint32_t s_Bl_Can_BuildFilterReg(Bl_Can_CanIdType id)
{
    bl_uint32_t u32_Reg;
    bl_uint32_t u32_Id = id & BL_CAN_ID_MASK;

    if ((id & BL_CAN_IDE_FLAG) != 0U)
    {
        /* extended: EXID[28:0] -> bits[31:3], IDE=1 -> bit2 */
        u32_Reg = (u32_Id << 3U) | (1U << 2U);
    }
    else
    {
        /* standard: STID[10:0] -> bits[31:21], IDE=0 */
        u32_Reg = (u32_Id << 21U);
    }

    if ((id & BL_CAN_RTR_FLAG) != 0U)
    {
        u32_Reg |= (1U << 1U);
    }

    return u32_Reg;
}

/**
 * @brief  find a HOH config entry by HOH id
 * @param  hoh : HOH id
 * @retval pointer to HOH config, or BL_NULL_PTR if not found
 */
static const Bl_Can_HohConfig_t *s_Bl_Can_FindHoh(Bl_Can_HohType hoh)
{
    bl_uint8_t i;

    for (i = 0U; i < g_Bl_Can_Config.u8_HohCount; i++)
    {
        if (g_Bl_Can_Config.p_HohConfig[i].hoh == hoh)
        {
            return &g_Bl_Can_Config.p_HohConfig[i];
        }
    }
    return BL_NULL_PTR;
}

/**
 * @brief  find a TX HOH config entry by HOH id
 * @param  hoh : HOH id
 * @retval pointer to TX HOH config, or BL_NULL_PTR if not found
 */
static const Bl_Can_HohConfig_t *s_Bl_Can_FindTxHoh(Bl_Can_HohType hoh)
{
    const Bl_Can_HohConfig_t *p_Hoh = s_Bl_Can_FindHoh(hoh);

    if ((p_Hoh != BL_NULL_PTR) && (p_Hoh->u8_Type == BL_CAN_HOH_TYPE_TX))
    {
        return p_Hoh;
    }
    return BL_NULL_PTR;
}

/**
 * @brief  find the RX HOH config entry bound to a RX FIFO
 * @param  u8_Fifo : RX FIFO number (0/1)
 * @retval pointer to RX HOH config, or BL_NULL_PTR if not found
 */
static const Bl_Can_HohConfig_t *s_Bl_Can_FindRxHoh(bl_uint8_t u8_Fifo)
{
    bl_uint8_t i;

    for (i = 0U; i < g_Bl_Can_Config.u8_HohCount; i++)
    {
        const Bl_Can_HohConfig_t *p_Hoh = &g_Bl_Can_Config.p_HohConfig[i];

        if ((p_Hoh->u8_Type == BL_CAN_HOH_TYPE_RX) && (p_Hoh->u8_HwObj == u8_Fifo))
        {
            return p_Hoh;
        }
    }
    return BL_NULL_PTR;
}

/**
 * @brief  convert a CAN L-PDU to a HAL TX header
 * @param  p_Pdu    : source L-PDU
 * @param  p_Header : destination HAL TX header
 * @retval None
 */
static void s_Bl_Can_PduToTxHeader(const Bl_Can_PduType *p_Pdu,
                                   CAN_TxHeaderTypeDef *p_Header)
{
    p_Header->StdId              = 0U;
    p_Header->ExtId              = 0U;
    p_Header->IDE                = CAN_ID_STD;
    p_Header->RTR                = CAN_RTR_DATA;
    p_Header->DLC                = (p_Pdu->dlc > 8U) ? 8U : p_Pdu->dlc;
    p_Header->TransmitGlobalTime = DISABLE;

    if ((p_Pdu->id & BL_CAN_IDE_FLAG) != 0U)
    {
        p_Header->IDE   = CAN_ID_EXT;
        p_Header->ExtId = p_Pdu->id & BL_CAN_ID_MASK;
    }
    else
    {
        p_Header->StdId = p_Pdu->id & BL_CAN_STD_ID_MASK;
    }

    if ((p_Pdu->id & BL_CAN_RTR_FLAG) != 0U)
    {
        p_Header->RTR = CAN_RTR_REMOTE;
    }
}

/**
 * @brief  convert a HAL RX header + data to a CAN L-PDU
 * @param  p_Header : source HAL RX header
 * @param  p_Data   : source RX data (8 bytes)
 * @param  p_Pdu    : destination L-PDU
 * @retval None
 */
static void s_Bl_Can_RxHeaderToPdu(const CAN_RxHeaderTypeDef *p_Header,
                                   const bl_uint8_t p_Data[BL_CAN_PDU_DATA_LENGTH],
                                   Bl_Can_PduType *p_Pdu)
{
    bl_uint8_t i;

    p_Pdu->id          = 0U;
    p_Pdu->swPduHandle = 0U;
    p_Pdu->dlc         = (bl_uint8_t)p_Header->DLC;

    if (p_Header->IDE == CAN_ID_EXT)
    {
        p_Pdu->id = ((bl_uint32_t)p_Header->ExtId) & BL_CAN_ID_MASK;
        p_Pdu->id |= BL_CAN_IDE_FLAG;
    }
    else
    {
        p_Pdu->id = ((bl_uint32_t)p_Header->StdId) & BL_CAN_STD_ID_MASK;
    }

    if (p_Header->RTR == CAN_RTR_REMOTE)
    {
        p_Pdu->id |= BL_CAN_RTR_FLAG;
    }

    for (i = 0U; i < BL_CAN_PDU_DATA_LENGTH; i++)
    {
        p_Pdu->data[i] = p_Data[i];
    }
}

/**
 * @brief  process queued TX requests: submit them to hardware mailboxes
 * @param  None
 * @retval None
 */
static void s_Bl_Can_ProcessTx(void)
{
    bl_uint8_t i;
    CAN_TxHeaderTypeDef s_Header;
    bl_uint32_t u32_Mailbox;

    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        if (s_Bl_Can_TxQueue[i].u8_State != BL_CAN_TX_SLOT_READY)
        {
            continue;
        }

        u32_Mailbox = 0U;
        s_Bl_Can_PduToTxHeader(&s_Bl_Can_TxQueue[i].s_Pdu, &s_Header);

        if (HAL_CAN_AddTxMessage(&hcan, &s_Header,
                                 s_Bl_Can_TxQueue[i].s_Pdu.data,
                                 &u32_Mailbox) == HAL_OK)
        {
            s_Bl_Can_TxQueue[i].u32_TxMailbox = u32_Mailbox;
            s_Bl_Can_TxQueue[i].u8_State      = BL_CAN_TX_SLOT_PENDING;
        }
        else
        {
            /* no free mailbox, stop submitting this cycle */
            break;
        }
    }
}

/**
 * @brief  poll pending TX requests and raise TxConfirmation on completion
 * @param  None
 * @retval None
 */
static void s_Bl_Can_ProcessTxConfirmation(void)
{
    bl_uint8_t i;
    Bl_Can_HohType hoh;

    for (i = 0U; i < BL_CAN_TX_QUEUE_DEPTH; i++)
    {
        if (s_Bl_Can_TxQueue[i].u8_State != BL_CAN_TX_SLOT_PENDING)
        {
            continue;
        }

        if (HAL_CAN_IsTxMessagePending(&hcan,
                                       s_Bl_Can_TxQueue[i].u32_TxMailbox) == 0U)
        {
            hoh = s_Bl_Can_TxQueue[i].hoh;
            s_Bl_Can_TxQueue[i].u8_State = BL_CAN_TX_SLOT_FREE;
            s_Bl_Can_TxQueue[i].hoh      = BL_CAN_INVALID_HOH;
            /* AUTOSAR: Can driver calls the CanIf TX confirmation */
            CanIf_TxConfirmation(hoh);
        }
    }
}

/**
 * @brief  push a received frame into the software RX queue (ISR context)
 * @param  p_Pdu   : received L-PDU
 * @param  u8_Fifo : source RX FIFO (0/1)
 * @retval None
 */
static void s_Bl_Can_RxPush(const Bl_Can_PduType *p_Pdu, bl_uint8_t u8_Fifo)
{
    bl_uint8_t u8_Next;

    u8_Next = (bl_uint8_t)((s_Bl_Can_RxHead + 1U) % BL_CAN_RX_QUEUE_DEPTH);

    if (u8_Next != s_Bl_Can_RxTail)
    {
        s_Bl_Can_RxQueue[s_Bl_Can_RxHead].s_Pdu   = *p_Pdu;
        s_Bl_Can_RxQueue[s_Bl_Can_RxHead].u8_Fifo = u8_Fifo;
        s_Bl_Can_RxHead = u8_Next;
    }
    else
    {
        /* queue full, drop frame */
        s_Bl_Can_RxOverflow++;
    }
}

/******************************* EOF (End of File) ***************************/

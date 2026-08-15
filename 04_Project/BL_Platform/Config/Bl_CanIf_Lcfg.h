/**
 ******************************************************************************
 * @file    Bl_CanIf_Lcfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-15
 * @brief   Bl_CanIf link-time configuration header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-15   [New] module created, PDU config type + extern table
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_CANIF_LCFG_H__
#define __BL_CANIF_LCFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/
#include "Bl_Types.h"
#include "Bl_CanIf_Cfg.h"

/****************************************************************
 *                         Macros
 ***************************************************************/

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/**
 * @brief CanIf PDU configuration entry (one per PDU)
 * @note  Uses plain bl_ types on purpose so the Config layer stays independent
 *        of the Apdapter layer; the values map 1:1 onto Bl_Can HOH / CanId
 *        semantics (see Bl_Can.h).
 */
typedef struct {
    bl_uint16_t u16_PduId;   /**< CanIf PDU id (routing key, unique)        */
    bl_uint16_t u16_Hoh;     /**< driver HOH id (see Bl_Can.h HOH table)     */
    bl_uint32_t u32_CanId;   /**< CAN id: TX sends this, RX matches this     */
    bl_uint32_t u32_IdMask;  /**< RX match mask (0 = match any / catch-all)  */
    bl_uint8_t  u8_Dir;      /**< direction: BL_CANIF_DIR_TX / BL_CANIF_DIR_RX */
} Bl_CanIf_PduConfigType;

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/** @brief CanIf PDU config table (link-time const, defined in Bl_CanIf_Lcfg.c) */
extern const Bl_CanIf_PduConfigType g_Bl_CanIf_PduConfig[BL_CANIF_PDU_CNT];

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_CANIF_LCFG_H__ */

/******************************* EOF (End of File) ***************************/

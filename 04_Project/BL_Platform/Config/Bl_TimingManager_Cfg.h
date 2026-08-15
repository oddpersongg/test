/**
 ******************************************************************************
 * @file    Bl_TimingManager_Cfg.h
 * @author  -
 * @version V1.0.0
 * @date    2026-08-16
 * @brief   Bl_TimingManager pre-compile configuration header file
 *          (centralized protocol timeouts: UDS S3/P2/P2*, CanTp N_As/N_Bs/N_Cr)
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ------------ ----------------------------------------------------
 * V1.0.0   2026-08-16   [New] module created, single source of truth for all
 *                       protocol timeouts; consumed via macro aliases by
 *                       Bl_CanTp_Cfg.h (N_*) and Bl_Uds_Cfg.h (P2/P2*)
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_TIMINGMANAGER_CFG_H__
#define __BL_TIMINGMANAGER_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/

/****************************************************************
 *                         Macros
 ***************************************************************/

/** @brief UDS session timing (ms) */
#define BL_TIMINGMANAGER_S3_TIMEOUT_MS          5000U   /**< session timeout (no TesterPresent -> back to default) */
#define BL_TIMINGMANAGER_P2_TIMEOUT_MS          50U     /**< server response time, advertised in 0x10 response       */
#define BL_TIMINGMANAGER_P2STAR_TIMEOUT_MS      5000U   /**< P2* for slow services (0x78 pending), advertised too    */

/** @brief ISO 15765-2 transport timing (ms) */
#define BL_TIMINGMANAGER_N_AS_TIMEOUT_MS        1000U   /**< TX: request -> frame confirmed by controller            */
#define BL_TIMINGMANAGER_N_BS_TIMEOUT_MS        1000U   /**< TX: wait for FC after FF/block                          */
#define BL_TIMINGMANAGER_N_CR_TIMEOUT_MS        1000U   /**< RX: wait for next CF                                    */

/* timer count comes from the Bl_TimingManager_TimerId_t enum (last member) */

/****************************************************************
 *                       Type Defs
 ***************************************************************/

/****************************************************************
 *                 Global Variables
 ***************************************************************/

/****************************************************************
 *                Function Declarations
 ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __BL_TIMINGMANAGER_CFG_H__ */

/******************************* EOF (End of File) ***************************/

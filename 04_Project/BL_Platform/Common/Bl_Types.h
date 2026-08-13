/**
 ******************************************************************************
 * @file    Bl_Types.h
 * @author  <author_name>
 * @version V1.0.0
 * @date    2026-08-09
 * @brief   Common type definitions header file
 ******************************************************************************
 */

/****************************************************************
 *                         Change Log
 ***************************************************************/
/**
 * Version  Date        Description
 * -------- ----------- ----------------------------------------------------
 * V1.0.0   2026-08-09  [New] Module created
 */

/****************************************************************
 *                     Include Guard
 ***************************************************************/
#ifndef __BL_TYPES_H__
#define __BL_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *                        Includes
 ***************************************************************/

/****************************************************************
 *                         Macros
 ***************************************************************/

#define BL_E_OK     0
#define BL_E_NOT_OK 1

/** @brief null value (non-pointer context) */
#define BL_NULL          0

/** @brief null pointer (pointer context) */
#define BL_NULL_PTR      ((void *)0)

/****************************************************************
 *                       Type Defs
 ***************************************************************/

typedef unsigned char      bl_uint8_t;
typedef unsigned short     bl_uint16_t;
typedef unsigned int       bl_uint32_t;
typedef unsigned long long bl_uint64_t;

typedef signed char        bl_int8_t;
typedef signed short       bl_int16_t;
typedef signed int         bl_int32_t;
typedef signed long long   bl_int64_t;

typedef float              bl_float32_t;
typedef double             bl_float64_t;

typedef unsigned char      bl_bool_t;

typedef unsigned int       bl_size_t;

/** @brief function return type, BL_E_OK (0) or BL_E_NOT_OK (1) */
typedef bl_uint8_t         bl_ret_t;

#ifdef __cplusplus
}
#endif

#endif /* __BL_TYPES_H__ */

/******************************* EOF (End of File) ***************************/

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

STM32F103VET6 (Cortex-M3, 512KB Flash, 64KB RAM) embedded firmware project using STM32CubeMX-generated HAL library. Keil MDK-ARM v5 build system with ARM Compiler 5 (V5.06 update 5).

## Build

```
# From 04_Project/MDK-ARM/, build with Keil command line:
D:/01_IDE_Tool/Keil5_MDK/UV4/UV4.exe -b project.uvprojx -j0 -o build.log

# Output: test/test.axf, test/test.hex
```

The project file is `04_Project/MDK-ARM/project.uvprojx`. It uses relative paths (`../Core/Src/`, `../Drivers/`, `../BL_Platform/`, `../CDD/`) from the `MDK-ARM/` directory.

## CubeMX Regeneration

`test.ioc` and `.mxproject` are at `04_Project/`. CubeMX can regenerate `Core/`, `Drivers/`, and `MDK-ARM/` in-place. After regeneration:

- **User code blocks preserved** — `USER CODE BEGIN/END` sections in `Core/Src/main.c`, `stm32f1xx_it.c`, `stm32f1xx_hal_msp.c` survive regeneration
- **Business code NOT in `.uvprojx`** — CubeMX regenerates `MDK-ARM/test.uvprojx` without BL_Platform/CDD groups. Sync from `project.uvprojx` or re-add the groups:
  - `BL_Platform/Common` — `../BL_Platform/Common/Bl_Types.h`
  - `BL_Platform/Apdapter` — `Bl_DriverAdapter.c/.h`, `Bl_Can.c/.h`, `Bl_Isr.c/.h`, `Bl_TaskUserdef.c/.h`
  - `BL_Platform/Core` — `Bl_TaskSchedule.c/.h`, `Bl_Rte.c/.h`
  - `BL_Platform/Config` — `Bl_TaskSchedule_Cfg.h`, `Bl_TaskSchedule_Lcfg.c/.h`
  - `CDD/OLED` — `../CDD/OLED/OLED.c/.h`, `../CDD/OLED/OLED_Font.h`
- **Include paths to add**: `../BL_Platform/Common;../BL_Platform/Apdapter;../BL_Platform/Core;../BL_Platform/Config;../CDD/OLED`

## Startup Sequence

`main()` calling order:

```
HAL_Init() → SystemClock_Config() → MX_GPIO_Init() → MX_I2C1_Init() → MX_TIM1_Init()
→ Bl_Rte_Init()                    // overall init: driver + scheduler + user tasks
→ Bl_TaskSchedule_MainFunction()   // infinite loop, never returns
```

`Bl_Rte_Init()` internally calls, in order:
1. `s_Bl_Rte_SysInit()` → `Bl_DriverAdapter_Init()` — OLED init + start TIM1 interrupt
2. `s_Bl_Rte_ProcessInit()` → `Bl_TaskSchedule_Init()` — scheduler init (reads Lcfg config)
3. `Bl_TaskUserdef_Init()` — register user application tasks

TIM1 (PSC=7, ARR=999, 8MHz HSI) fires at 1kHz → `HAL_TIM_PeriodElapsedCallback` (in `Bl_Isr`) → `Bl_TaskSchedule_TickInc()` drives the scheduler clock.

## Architecture

```
04_Project/
├── Core/                    HAL application layer (CubeMX generated)
│   ├── Inc/                 main.h, stm32f1xx_hal_conf.h, stm32f1xx_it.h
│   └── Src/                 main.c, stm32f1xx_hal_msp.c, stm32f1xx_it.c, system_stm32f1xx.c
├── Drivers/                 STM32CubeF1 HAL + CMSIS (CubeMX generated, do not edit)
│   ├── CMSIS/
│   └── STM32F1xx_HAL_Driver/
├── MDK-ARM/                 Keil toolchain (startup + project file + build output)
│   ├── project.uvprojx      ← main project file
│   ├── test.uvprojx         ← CubeMX-generated (not synced with BL_Platform/CDD groups)
│   └── startup_stm32f103xe.s
├── BL_Platform/             Business Logic / Platform abstraction (custom)
│   ├── Common/Bl_Types.h    Platform types (bl_uint8_t, bl_ret_t, BL_E_OK, etc.)
│   ├── Apdapter/            Chip-specific adapter layer (User Add markers, needs manual porting)
│   │   ├── Bl_DriverAdapter  Driver init/deinit hub (CDD + Timer)
│   │   ├── Bl_Isr            Interrupt callbacks (HAL_*_Callback overrides)
│   │   ├── Bl_TaskUserdef    User application task registration
│   │   └── Bl_Can            CAN driver skeleton
│   ├── Core/                Chip-agnostic core static code (no User Add markers)
│   │   ├── Bl_TaskSchedule   Cooperative task scheduler
│   │   └── Bl_Rte            Runtime Environment init/deinit glue
│   └── Config/              Pure configuration data (no logic)
│       ├── Bl_TaskSchedule_Cfg.h     Pre-compile config (macros, MAX_TASKS)
│       └── Bl_TaskSchedule_Lcfg.h/.c  Link-time const config (g_..._Config)
├── CDD/OLED/                Complex Device Driver — SSD1306 0.96" OLED via I2C1
├── test.ioc                 CubeMX project config (I2C1 on PB6/PB7, TIM1 update interrupt)
└── .mxproject               CubeMX metadata (paths relative to MDK-ARM/)
```

### Layering Principle

Dependency direction is one-way: **Apdapter → Core → Common/Config**.

- **Common / Core / Config** — chip-agnostic static code, portable to any MCU. No `/* User Add */` markers.
- **Apdapter** — chip-specific, must be manually adapted when porting. Uses `/* User Add Begin/End */` markers for the user-modifiable parts.

### Key architectural decisions

- **BL_Platform types**: Custom type system (`bl_uint8_t`, `bl_ret_t`, `BL_E_OK`) in `Bl_Types.h` — replaces `<stdint.h>` throughout business code. All BL_Platform code must use `bl_` types, never `<stdint.h>` types.
- **OLED I2C address**: 7-bit address `0x3C` (HAL uses `0x3C << 1 = 0x78` internally via `HAL_I2C_Master_Transmit`)
- **HAL module enabling**: `Core/Inc/stm32f1xx_hal_conf.h` — toggle `#define HAL_XXX_MODULE_ENABLED` for peripherals used
- **.mxproject paths**: Uses `..\Core\`, `..\Drivers\` relative to `MDK-ARM/` — this is intentional, don't remove the `..\` prefix. It allows project relocation without breaking CubeMX code regeneration
- **OLED init flow**: `MX_I2C1_Init()` → `HAL_I2C_MspInit()` configures PB6/PB7 AF_OD, then `OLED_Init()` uses `HAL_I2C_Master_Transmit()` for all I2C communication

### Bl_TaskSchedule — Cooperative Task Scheduler

Simple bare-metal scheduler, no RTOS. Located in `BL_Platform/Core/`. Chip-agnostic.

- **Clock source**: TIM1 update interrupt → `Bl_Isr`'s `HAL_TIM_PeriodElapsedCallback` → `Bl_TaskSchedule_TickInc()`
- **Main loop**: `Bl_TaskSchedule_MainFunction()` contains the only `while(1)` — delegates to static `s_Bl_TaskSchedule_Process()`, idle when no tasks are due
- **Init**: `Bl_TaskSchedule_Init(void)` reads config from `g_Bl_TaskSchedule_Config` (Lcfg const struct), no parameter
- **Config split**:
  - `Cfg.h` — pre-compile macros (`BL_TASKSCHEDULE_MAX_TASKS`)
  - `Lcfg.c/.h` — link-time const `g_Bl_TaskSchedule_Config` (`u8_MaxTasks`, `u16_TickPeriodMs`)
- **Registration**: `RegisterTask(&attr)` for N-times execution (auto-remove), `RegisterTaskInfinite(func, period)` for infinite execution (manual cancel via `UnregisterTask(id)`)
- **Timing**: 32-bit unsigned tick counter with subtraction, handles wrap-around correctly
- **Memory**: Static task table, no dynamic allocation

### Bl_Rte — Runtime Environment

Located in `BL_Platform/Core/`. Glue layer that orchestrates init/deinit. Chip-agnostic (depends only on Adapter *interfaces*, not implementations).

- `Bl_Rte_Init(void)` → `s_Bl_Rte_SysInit()` + `s_Bl_Rte_ProcessInit()` + `Bl_TaskUserdef_Init()`
- `Bl_Rte_Deinit(void)` → `Bl_TaskUserdef_Deinit()` + `s_Bl_Rte_ProcessDeinit()` + `s_Bl_Rte_SysDeinit()` (reverse order)
- `s_Bl_Rte_SysInit/SysDeinit` wrap `Bl_DriverAdapter_Init/Deinit` (use `bl_ret_t e_Ret` accumulate pattern for future extension)
- `s_Bl_Rte_ProcessInit/ProcessDeinit` wrap scheduler init/deinit

### Bl_DriverAdapter — Driver Initialization Hub (Apdapter)

- `Bl_DriverAdapter_Init()` runs two parallel static init functions (single return via `e_Ret |=`):
  - `s_Bl_DriverAdapter_CddInit()` — CDD modules (currently `OLED_Init()`)
  - `s_Bl_DriverAdapter_TimerInit()` — starts TIM1 in interrupt mode (`HAL_TIM_Base_Start_IT(&htim1)`)
- `Bl_DriverAdapter_Deinit()` mirrors with `s_Bl_DriverAdapter_CddDeinit()` + `s_Bl_DriverAdapter_TimerDeinit()`
- Init/deinit functions use `/* User Add Begin/End */` markers

### Bl_Isr — Interrupt Callbacks (Apdapter)

- Holds chip-specific interrupt callbacks (e.g. `HAL_TIM_PeriodElapsedCallback`). Migrated out of `Bl_DriverAdapter` so driver init and interrupt handling stay separate.
- ISR callbacks route hardware events to Core modules (e.g. TIM1 → `Bl_TaskSchedule_TickInc()`).

### Bl_TaskUserdef — User Task Registration (Apdapter)

- `Bl_TaskUserdef_Init(void)` — registers user application tasks (returns `void`; registration result isn't a simple OK/NOT_OK)
- `Bl_TaskUserdef_Deinit(void)` — unregisters all tasks it registered, tracking task IDs in a static array
- Example tasks: 1s OLED counter, KEY0 (PD10) removes counter task, KEY1 (PD9) restores it. KEY pins are input + pull-up.

## Coding Standards

From `01_InputFiles/AI编码规则/文件格式规范.md`:

- **Indentation**: 4 spaces, no tabs
- **File encoding**: UTF-8, CRLF
- **Module naming**: PascalCase (`LedControl`, `Bl_Can`)
- **Function naming**: `<ModuleName>_<Action>(<params>)` — e.g., `Bl_Can_Init()`, `OLED_ShowString()`
- **Variable prefixes**: `g_` global, `s_` static, `p_` pointer
- **Type naming**: `<ModuleName>_<Desc>_t` — e.g., `Bl_Can_ConfigType`, `LedControl_Status_t`
- **Macros**: `UPPER_SNAKE_CASE`
- **Section separators**: 65-char `/*...***/` blocks (e.g., `/**** Includes ****/`)
- **Function docs**: `@brief`, `@param`, `@retval` tags
- **Change log**: Version/Date/Description table in file header, tags: `[New]`, `[Modify]`, `[Fix]`, `[Remove]`, `[Optimize]`
- **EOF marker**: `/******************************* EOF (End of File) ***************************/` on last line

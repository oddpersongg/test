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

## CAN Test Bench (ZCANPRO / usbcan.dll)

Hardware loopback testing of the diagnostic stack (Bl_Can → CanIf → CanTp → Dcm → Uds) is done with a **ZLG USBCAN** adapter (VID_0471&PID_1200, driver `ZLGCAN`) and the ZCANPRO install at `D:\02_Engineering_Tool\ZCANPRO`.

- **ZCANPRO GUI cannot be scripted** (pure GUI, no CLI). It also **holds the USB device exclusively while running** — close it (`Stop-Process -Name ZCANPRO -Force`) before any scripted test.
- **Scripted access**: drive `kerneldlls\usbcan.dll` (ZLG VCI / ControlCAN API) directly via ctypes with ZCANPRO's bundled Python (`D:\02_Engineering_Tool\ZCANPRO\python38\python.exe`). Device type for this adapter is **`99`** in that DLL (ZCANPRO maps USBCAN2→99). 500 kbps = BTR `Timing0=0x00, Timing1=0x1C`.
- **Test tool**: `test/uds_tool.py` (repo root, kept) — generic UDS-over-ISO-TP tool. `python uds_tool.py` runs a built-in full sequence (0x10/0x27/0x34/0x36/0x37, 8 checks); `python uds_tool.py 10 01 27 02 A5` sends arbitrary SID+data sequences. It does ISO-TP SF/FF+CF framing and response reassembly automatically.
- **⚠️ ISO-TP framing is mandatory**: CanTp parses `data[0]` as PCI. A **raw** UDS request `0x7E0 [10 01]` is decoded as an FF frame (`0x1xxx`) and dropped — it MUST be framed: `0x7E0 [02 10 01]` (SF len=2 + 10 01). Response comes back framed too: `0x7E8 [06 50 01 00 32 13 88]`.
- **Known driver quirk**: after many open/close or Keil-flash cycles the USBCAN can get stuck (VCI_Transmit blocks ~1.7–5 s and returns 0, ErrCode=0x2). Software recovery (ResetCAN/InitCAN/ReadBoardInfo cycles) does NOT fix it — **physically replug the USB device**. A listen-only test (no transmit) confirms the driver is alive: it reports ErrCode=0 and no frames.
- **Flash firmware** (uses the debug probe configured in the project): `UV4.exe -f project.uvprojx -j0 -o flash.log` from `04_Project/MDK-ARM/` (Erase/Programming/Verify).
- **⚠️ Reset-and-Run must be enabled**: Keil's Flash Download "Reset and Run" checkbox lives in the GUI (Options → Debug → Settings → Flash Download), not in `uvprojx`/`uvoptx` XML. Without it the board does NOT run the new firmware after flashing — every CAN transmit then fails with no ACK (looks like a dead adapter, but is actually a not-running board). Verify the flash log ends with "Application running ..." and the board boots (OLED shows the 1 s uptime counter) before testing.

## CubeMX Regeneration

`test.ioc` and `.mxproject` are at `04_Project/`. CubeMX can regenerate `Core/`, `Drivers/`, and `MDK-ARM/` in-place. After regeneration:

- **User code blocks preserved** — `USER CODE BEGIN/END` sections in `Core/Src/main.c`, `stm32f1xx_it.c`, `stm32f1xx_hal_msp.c` survive regeneration
- **Business code NOT in `.uvprojx`** — CubeMX regenerates `MDK-ARM/test.uvprojx` without BL_Platform/CDD groups. Sync from `project.uvprojx` or re-add the groups:
  - `BL_Platform/Common` — `../BL_Platform/Common/Bl_Types.h`
  - `BL_Platform/Apdapter` — `Bl_DriverAdapter.c/.h`, `Bl_Can.c/.h`, `Bl_Can_Cfg.h`, `Bl_Fls.c/.h`, `Bl_Fls_Cfg.h`, `Bl_Isr.c/.h`, `Bl_TaskUserdef.c/.h`
  - `BL_Platform/Core` — `Bl_TaskSchedule.c/.h`, `Bl_Rte.c/.h`, `Bl_TimingManager.c/.h`, `Bl_Dcm.c/.h`, `Bl_Uds.c/.h`, `Bl_UdsService.c/.h`, `Bl_CanIf.c/.h`, `Bl_CanTp.c/.h`, `Bl_FlashIf.c/.h`
  - `BL_Platform/Config` — `Bl_TaskSchedule_Cfg.h`, `Bl_TaskSchedule_Lcfg.c/.h`, `Bl_TimingManager_Cfg.h`, `Bl_Dcm_Cfg.h`, `Bl_Dcm_Lcfg.c/.h`, `Bl_Uds_Cfg.h`, `Bl_UdsService_Lcfg.c/.h`, `Bl_CanIf_Cfg.h`, `Bl_CanIf_Lcfg.c/.h`, `Bl_CanTp_Cfg.h`
  - `CDD/OLED` — `../CDD/OLED/OLED.c/.h`, `../CDD/OLED/OLED_Font.h`
- **Include paths to add**: `../BL_Platform/Common;../BL_Platform/Apdapter;../BL_Platform/Core;../BL_Platform/Config;../CDD/OLED`

## Startup Sequence

`main()` calling order:

```
HAL_Init() → SystemClock_Config() → MX_GPIO_Init() → MX_I2C1_Init() → MX_TIM1_Init() → MX_CAN_Init()
→ Bl_Rte_Init()                    // overall init: driver + scheduler + user tasks
→ Bl_TaskSchedule_MainFunction()   // infinite loop, never returns
```

`MX_CAN_Init()` (CubeMX generated) configures `hcan.Instance/.Init` (500kbps) and calls `HAL_CAN_Init()` (which invokes `HAL_CAN_MspInit` for GPIO/clock/remap). It does NOT configure filters or start — `Bl_Can_Init()` does that.

`Bl_Rte_Init()` internally calls, in order:
1. `s_Bl_Rte_SysInit()` → `Bl_DriverAdapter_Init()` — OLED init + start TIM1 interrupt + `Bl_Can_Init()` (filter + start + RX interrupt) + `Bl_Fls_Init()` (AUTOSAR internal flash driver)
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
│   │   ├── Bl_DriverAdapter  Driver init/deinit hub (CDD + Timer + CAN + Fls)
│   │   ├── Bl_Isr            Interrupt callbacks (TIM1 tick + CAN RX FIFO callbacks)
│   │   ├── Bl_TaskUserdef    User application (uptime counter only)
│   │   ├── Bl_Can            CAN driver (AUTOSAR-style, interrupt RX + software queue)
│   │   ├── Bl_Can_Cfg.h      CAN pre-compile config (chip-specific: filter banks, queue depth)
│   │   ├── Bl_Fls            Internal flash driver (AUTOSAR Fls style: async Erase/Write/Read jobs + MainFunction + GetStatus/GetJobResult)
│   │   └── Bl_Fls_Cfg.h      Fls pre-compile config (chip-specific: page/sector size, app-area range)
│   ├── Core/                Chip-agnostic core static code (no User Add markers)
│   │   ├── Bl_TaskSchedule   Cooperative task scheduler (tick + GetTickMs; main loop drives CAN MainFunction + CanTp)
│   │   ├── Bl_Rte            Runtime Environment init/deinit glue
│   │   ├── Bl_TimingManager  Centralized protocol timers (S3/P2/P2* + CanTp N_*) over GetTickMs
│   │   ├── Bl_CanIf          CAN Interface layer (PDU dispatch, hardware-independent)
│   │   ├── Bl_CanTp          ISO 15765-2 transport layer (SF/FF/CF/FC, timeouts)
│   │   ├── Bl_FlashIf        Synchronous flash access wrapper (ErasePage/Write/Read over Bl_Fls, Core)
│   │   ├── Bl_Dcm            Dcm layer (ISO 14229; service discriminator: SID/NRC/session/security check, then dispatch to Bl_Uds)
│   │   ├── Bl_Uds            UDS service implementations (0x10/0x11/0x27/0x34/0x36/0x37/0x3E + NRC replies)
│   │   └── Bl_UdsService     Centralized UDS sub-service lookup & dispatch (over Bl_UdsService_Lcfg)
│   └── Config/              Pure configuration data (no logic)
│       ├── Bl_TaskSchedule_Cfg.h     Pre-compile config (macros, MAX_TASKS)
│       ├── Bl_TaskSchedule_Lcfg.h/.c  Link-time const config (g_..._Config)
│       ├── Bl_TimingManager_Cfg.h    Centralized protocol timeouts (S3/P2/P2*/N_*), single source
│       ├── Bl_Dcm_Cfg.h              Dcm config (download buffer size = block + 2 overhead, includes Bl_Uds_Cfg.h)
│       ├── Bl_Dcm_Lcfg.h/.c          Dcm link-time config: diagnostic service table (SID -> handler + dispatch metadata)
│       ├── Bl_Uds_Cfg.h              Uds config (SID/NRC/session/security/P2/P2*, block size, app flash range)
│       └── Bl_UdsService_Lcfg.h/.c   Centralized UDS sub-service table ((SID,subFunc) -> response function)
├── CDD/OLED/                Complex Device Driver — SSD1306 0.96" OLED via I2C1
├── test.ioc                 CubeMX project config (I2C1 on PB6/PB7, TIM1 update interrupt)
└── .mxproject               CubeMX metadata (paths relative to MDK-ARM/)
```

### Layering Principle

Dependency direction is one-way: **Apdapter → Core → Common/Config**.

- **Common / Core / Config** — chip-agnostic static code, portable to any MCU. No `/* User Add */` markers.
- **Apdapter** — chip-specific, must be manually adapted when porting. Uses `/* User Add Begin/End */` markers for the user-modifiable parts.

Note: the layer boundary crosses only through fixed interfaces — `Bl_TaskSchedule.c` (Core) calls the Apdapter's `Bl_Can.h` API (`Bl_Can_MainFunctionWrite/Read/BusOff`, `Bl_Can_Write`), and `Bl_Can.c` (Apdapter) calls into Core's `Bl_CanIf.h` (`CanIf_RxIndication/TxConfirmation`, named after the AUTOSAR MCAL symbols) — the AUTOSAR contract: lower layer calls upper layer callbacks, upper layer calls lower layer API. On porting, only `Bl_Can.c` and the CubeMX `hcan`/Msp init are re-adapted; both interfaces stay fixed.

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

### Bl_Can — CAN Driver (Apdapter, AUTOSAR-style)

STM32F1 bxCAN driver on `hcan` (CubeMX-generated handle). 500 kbps. Pins PB8(RX)/PB9(TX) via AFIO remap (`__HAL_AFIO_REMAP_CAN1_2()`) in `HAL_CAN_MspInit` in `stm32f1xx_hal_msp.c`.

- **Config**: `Bl_Can_Cfg.h` (Apdapter, chip-specific) — `BL_CAN_TX_QUEUE_DEPTH 8`, `BL_CAN_RX_QUEUE_DEPTH 128`, `BL_CAN_MAX_FILTER_BANK 14`. HOH table + `g_Bl_Can_Config` live in `Bl_Can.c`.
- **Init**: `Bl_Can_Init()` reuses CubeMX `hcan` — calls `HAL_CAN_Init(&hcan)`, configures RX filter (accept-all), `HAL_CAN_Start`, then enables RX FIFO0/FIFO1 message-pending interrupt + NVIC (`CAN1_RX0_IRQn`, `CAN1_RX1_IRQn`, prio 0).
- **Write (async)**: `Bl_Can_Write(hoh, &pdu)` only enqueues into software TX queue. `Bl_Can_MainFunctionWrite()` (scheduler loop) dequeues → `HAL_CAN_AddTxMessage` → polls `HAL_CAN_IsTxMessagePending` → calls `CanIf_TxConfirmation(hoh)` on completion (AUTOSAR: driver calls CanIf).
- **Read (interrupt-driven)**: CAN RX interrupt → `Bl_Can_RxIsr(fifo)` reads FIFO → pushes into software RX ring queue (SPSC, 127 usable of 128 slots). `Bl_Can_MainFunctionRead()` drains queue → calls `CanIf_RxIndication(hoh, &pdu)` (AUTOSAR: driver calls CanIf).
- **Driver → CanIf callbacks**: `Bl_Can.c` calls `CanIf_RxIndication(hoh, pdu)` / `CanIf_TxConfirmation(hoh)` directly (declared in `Bl_CanIf.h`, named after the AUTOSAR MCAL symbols so a real MCAL Can driver binds without an adapter) — there are NO driver-level `Bl_Can_RxIndication/TxConfirmation` anymore.
- **AUTOSAR MainFunctions**: `Bl_Can_MainFunctionWrite/Read` (implemented); `BusOff` (implemented — software recovery: ESR.BOF detect → `HAL_CAN_Stop` → wait `BL_CAN_BUSOFF_RECOVERY_TIME_MS` → `HAL_CAN_Start`, configurable via `BL_CAN_BUSOFF_RECOVERY_ENABLE`); `Mode/Wakeup` (stubs, TODO).
- **ID encoding**: AUTOSAR `Can_IdType` — bit31 IDE, bit30 RTR, bits28..0 ID (`BL_CAN_IDE_FLAG`/`BL_CAN_RTR_FLAG`/`BL_CAN_ID_MASK`).

### Bl_CanIf — CAN Interface (Core, hardware-independent)

AUTOSAR CanIf-style layer sitting between the application and the `Bl_Can` driver. The application only knows **PDU ids**; CanIf handles PDU ↔ (HOH + CAN id) routing.

- **Files**: `Core/Bl_CanIf.c/.h` (logic), `Config/Bl_CanIf_Cfg.h` (PDU id / direction / count macros), `Config/Bl_CanIf_Lcfg.c/.h` (link-time const PDU config table `g_Bl_CanIf_PduConfig`).
- **TX**: `Bl_CanIf_Transmit(u16_PduId, data, dlc)` — looks up the TX PDU config, builds `Bl_Can_PduType` (`swPduHandle = PDU id`, data zero-filled beyond dlc) and calls `Bl_Can_Write(hoh, &pdu)` (async enqueue).
- **RX (driver → CanIf)**: driver calls `CanIf_RxIndication(hoh, pdu)` (exact AUTOSAR MCAL symbol) — CanIf dispatches by PDU (HOH + CAN id mask, table in `Bl_CanIf_Lcfg.c`) → `Bl_CanTp_RxIndication(pduId, pdu)`. May run in ISR context.
- **TX confirmation (driver → CanIf)**: driver calls `CanIf_TxConfirmation(hoh)` — CanIf maps HOH back to PDU id → `Bl_CanTp_TxConfirmation(pduId)`.
- **Upper-layer handoff**: no weak hooks — CanIf calls CanTp entry points directly (PduR-style), keeping application-specific handling out of CanIf.
- **Init/deinit**: called from `Bl_Rte` (`s_Bl_Rte_ProcessInit/ProcessDeinit`), after the driver is up.
- Default config: PDU 0 = DIAG_RX (0x7E0, physical requests → CanTp), PDU 1 = DIAG_TX (0x7E8, responses/FC), PDU 2 = DIAG_FUNC_RX (0x7DF, functional requests — broadcast services only). Unmatched IDs are dropped.

### Bl_CanTp — ISO 15765-2 Transport Layer (Core, Phase 1)

- **Files**: `Core/Bl_CanTp.c/.h` (state machines), `Config/Bl_CanTp_Cfg.h` (frame lengths, FC params, N_* timeouts, CanIf PDU mapping).
- **Upper API**: `Bl_CanTp_Transmit(pduId, sdu, len)`; completion via weak `Bl_CanTp_UpperTxConfirmation(pduId, result)`, reception via `Bl_CanTp_UpperRxIndication(pduId, sdu, len)` (UDS will override).
- **Lower entries** (called by CanIf): `Bl_CanTp_RxIndication(pduId, pdu)` — parses PCI: SF → deliver, FF → start RX session + send FC, CF → append (SN check), FC → feed TX session; `Bl_CanTp_TxConfirmation(pduId)` — frame-level confirmations advance the TX state machine.
- **TX**: SF for ≤7 bytes; FF+CF with flow control (BS/STmin honored), STmin pacing + N_As/N_Bs in `Bl_CanTp_MainFunction()` (called by the scheduler main loop, timed by `Bl_TaskSchedule_GetTickMs`).
- **RX**: reassembles multi-frame SDUs into the Dcm buffer `BL_DCM_BUFFER_SIZE` (single RX session), sends FC (default BS=0, STmin=0), N_Cr timeout aborts.
- Scope: 11-bit physical addressing (0x7E0/0x7E8), single RX + single TX session, no extended addressing, sub-ms STmin treated as 0. Phase 2: 29-bit/extended addressing, WAIT/OVF handling, multi-connection.

### Bl_Dcm — UDS / ISO 14229 Layer, Discriminator (Core)

AUTOSAR Dcm-style **service discriminator** — decides what a request *is*, then hands it to `Bl_Uds` to *do* it (user decision: Dcm = discrimination only, Uds = service implementations).

- **Files**: `Core/Bl_Dcm.c/.h` (dispatcher), `Config/Bl_Dcm_Cfg.h` (buffer/config), `Config/Bl_Dcm_Lcfg.h/.c` (service table).
- **Entry**: overrides weak `Bl_CanTp_UpperRxIndication(pduId, sdu, len)` — parses SID, looks it up in the const service table `g_Bl_Dcm_ServiceConfig[]`, runs gating checks, then calls the `Bl_Uds` handler. **Functional addressing (0x7DF)**: requests on the functional PDU id are accepted only for TesterPresent 0x3E (refreshes S3, sends NO response); all other services on 0x7DF are silently ignored (ISO 14229: no response on functional addressing).
- **Service table struct** (`Bl_Dcm_Service_t` + `Bl_Uds_ServiceFunc_t`, defined in `Config/Bl_Dcm_Lcfg.h`, instantiated in `Bl_Dcm_Lcfg.c`): SID / sub-function byte count (`u8_SubFuncLen`, 0 = no sub-function, 1 = 1-byte sub-function — ISO 14229 sub-functions are always 1 byte) / **sub-service id table reference** (`p_SubTable` + `u8_SubCnt` → the service's sub-service table in `Bl_UdsService_Lcfg`, the single source of supported sub-function ids; the Dcm 0x12 gate matches `u8_SubFunc`, no bitmap) / suppress-positive-response flag (`u8_SuppressBit`) / min length / session mask / security needed / P2 & P2* overrides (0xFF = default from `Bl_TimingManager`, reserved for slow-service handling) / response data length (`u8_RespDataLen`, excluding SID+sub; fixed for constant-length responses, upper bound for variable ones) / handler fn. **Add/remove/re-configure services by editing `Bl_Dcm_Lcfg.c` + the sub-service table in `Bl_UdsService_Lcfg.c` only — no Core changes needed.**
- **NRC gating order**: SID not found → 0x11 serviceNotSupported; request shorter than min → 0x13 incorrectMessageLength; sub-function violations → 0x12 subFunctionNotSupported (the sub-function is looked up in the service's sub-service id table from `Bl_UdsService_Lcfg`; a 0x80 suppress bit on a service without `u8_SuppressBit` is also rejected); session not allowed → 0x7F serviceNotSupportedInActiveSession; security needed and not unlocked → 0x33 securityAccessDenied. Pass → call the `Bl_Uds` handler.
- **Service table** (7 entries, all → `Bl_Uds_*`): 0x10 (sub 01/02/03, suppress not allowed), 0x11 (sub 01/02/03/04), 0x27 (sub 01/02), 0x3E (sub 00, suppress allowed) in any session (mask 0x07); 0x34/0x36/0x37 (no sub-function) only in programming session (mask 0x02) and require security unlocked.
- **Download buffer**: `BL_DCM_BUFFER_SIZE[BL_DCM_BUFFER_LEN]` — `BL_DCM_BUFFER_LEN = BL_UDS_TRANSFER_BLOCK_SIZE (2048) + BL_DCM_TRANSFER_OVERHEAD (2)` = 2050. Reused as the CanTp reassembly buffer (single RX session, zero-copy).
- **Init**: `Bl_Dcm_Init()` (from RTE) clears session/security/download state; no deinit needed.

### Bl_Uds — UDS Service Implementations (Core)

AUTOSAR UDS service layer — the *handlers* behind the Dcm discriminator. `Bl_Uds_ServiceFunc_t = void(*)(const bl_uint8_t*, bl_uint32_t)`.

- **Files**: `Core/Bl_Uds.c/.h` (services), `Config/Bl_Uds_Cfg.h` (SID/NRC/session/security/P2/P2*, `BL_UDS_TRANSFER_BLOCK_SIZE 2048`, `BL_UDS_RESPONSE_BUFFER_LEN 64`, `BL_UDS_APP_FLASH_BASE_ADDR/MAX_SIZE`, download-state macros).
- **Implemented services**: 0x10 DiagnosticSessionControl — **sub-service dispatcher**: `Bl_Uds_DiagSessionControl` matches the sub-function against the **centralized UDS sub-service table** (`Config/Bl_UdsService_Lcfg.h/.c` via `Core/Bl_UdsService.c`: `Bl_UdsService_Find(sid,sub)` / `Bl_UdsService_Dispatch`, entry: sid/subFunc/name/resetSecurity/resetDownload/respDataLen/p_Func) and jumps to the per-sub-service response function (`Bl_Uds_DiagSessionDefaultResp/ProgrammingResp/ExtendedResp`, shared `s_Bl_Uds_SendDiagSessionResp` builds 0x50+sub+P2+P2*); 0x11 ECUReset — 4 sub-services (hard/keyOffOn/soft/fastSoft) with a **real reset**: the response functions queue 0x51, synchronously pump the send pipeline (`Bl_Can_MainFunctionWrite` + `Bl_CanTp_MainFunction`, bounded by `BL_UDS_ECURESET_TX_TIMEOUT_MS`) until the response is confirmed on the bus, then reset via `Bl_Rte_SystemReset` (respond-then-reset, hardware-verified — the chip restarts and the session is cleared); 0x27 SecurityAccess (fixed seed 0x5A / key 0xA5), 0x34 RequestDownload (parse address+length format, validate against app flash range, reply 0x74 with maxBlockLength=0x0800, arm download state), 0x36 TransferData (block-sequence + length checks, dest-address range guard, data write to flash is TODO — `Bl_FlashIf` is ready and hardware-verified), 0x37 RequestTransferExit, 0x3E TesterPresent (suppress-bit aware).
- **NRC replies**: `Bl_Uds_SendNrc(sid, nrc)` builds `0x7F SID NRC`; static response buffer `s_Bl_Uds_Resp[64]`.
- **Response TX queue** (back-to-back requests): CanTp TX is single-session (`Bl_CanTp_Transmit` returns `BL_E_NOT_OK` while a previous response is still in flight), so Uds queues responses in a ring (`BL_UDS_RESPONSE_QUEUE_DEPTH 4`, entries hold a copy of the response). `s_Bl_Uds_SendResponse` pushes + `Bl_Uds_ProcessResponseQueue()`; the override `Bl_CanTp_UpperTxConfirmation` pops the in-flight head (`s_Bl_Uds_TxInFlight`) and re-processes the queue. The queue is ALSO driven periodically by **`Bl_Dcm_MainFunction()`** (called every main-loop iteration after CanTp) — AUTOSAR Dcm_MainFunction style bounded retry, which self-heals the "CanTp busy without a pending confirmation" window. Preserves order for back-to-back requests (e.g. `10 01`+`10 02`+`27 01` → `50 01 → 50 02 → 67 01 5A`). Without this, the middle response was silently dropped (old `(void)Bl_CanTp_Transmit(...)`), appearing last after the tester's timeout retry. Depth 4 suffices for SF responses; raise it if multi-frame (0x62 etc.) responses are added.
- **Hooks**: overrides weak `Bl_CanTp_UpperTxConfirmation` (pop in-flight + reprocess queue) and `Bl_CanTp_UpperRxErrorIndication` (RX session aborted → reset download state to IDLE, block counter cleared).
- **Init**: `Bl_Uds_Init()` from RTE (`Bl_Rte_ProcessInit`), after `Bl_CanTp_Init()`.
- **Dependency direction**: `Bl_Dcm.c` includes `Bl_CanTp.h` + `Bl_Uds.h`; `Bl_Uds.c` includes `Bl_CanTp.h` + `Bl_Uds.h`; `Bl_Dcm_Cfg.h` includes `Bl_Uds_Cfg.h` — acyclic.

### Bl_TimingManager — Centralized Protocol Timing (Core)

Single source of truth for all protocol timeouts, plus a generic software timer service. Named by user decision; scope covers UDS timing (S3/P2/P2*) and CanTp timing (N_As/N_Bs/N_Cr).

- **Files**: `Core/Bl_TimingManager.c/.h` (timer service), `Config/Bl_TimingManager_Cfg.h` (timeout values, single source).
- **Timeout values** (`Bl_TimingManager_Cfg.h`): `BL_TIMINGMANAGER_S3_TIMEOUT_MS 5000`, `P2 50`, `P2STAR 5000`, `N_AS 1000`, `N_BS 1000`, `N_CR 1000`. Consumed via macro aliases by `Bl_CanTp_Cfg.h` (`BL_CANTP_N_*_TIMEOUT_MS → BL_TIMINGMANAGER_N_*_TIMEOUT_MS`) and `Bl_Uds_Cfg.h` (`BL_UDS_P2/P2STAR_DEFAULT_MS → BL_TIMINGMANAGER_P2/P2STAR_TIMEOUT_MS`) — change the value in one place only.
- **Runtime consumers**: the **S3 timer** is started in `Bl_Uds_Init()`, refreshed on every accepted request (`Bl_Dcm` restarts it in `Bl_CanTp_UpperRxIndication`), and checked by `Bl_Dcm_MainFunction()` — expiry calls `Bl_Uds_ResetToDefaultSession()`. The **P2/P2* values** are read at runtime by the 0x10 handler via `Bl_TimingManager_GetTimeoutMs()` (compile-time macros are kept as aliases only). N_* values remain compile-time aliases (CanTp keeps its own frame timers).
- **Timer ids**: `Bl_TimingManager_TimerId_t` enum — `S3 / P2 / P2STAR / N_AS / N_BS / N_CR` (+ `BL_TIMINGMANAGER_TIMER_CNT` sentinel).
- **API**: `Bl_TimingManager_Init()` (stop-all), `Start(id)` (restart from now), `Stop(id)`, `IsRunning(id)`, `IsExpired(id)` (wrap-safe subtraction vs `Bl_TaskSchedule_GetTickMs`), `GetRemainingMs(id)`, `GetTimeoutMs(id)`.
- **Tick source**: `Bl_TaskSchedule_GetTickMs()` (Core, no HAL dependency). 32-bit subtraction handles tick wrap-around.
- **Usage note**: CanTp keeps its own state-machine timers internally but reads the timeout *values* from this module's Cfg (single source). Dcm/Uds can use the timer service for S3 session timeout / P2 response windows.
- **Init**: from RTE (`s_Bl_Rte_ProcessInit`), right after `Bl_TaskSchedule_Init()`.

### Bl_Fls — Internal Flash Driver (Apdapter, AUTOSAR Fls style)

AUTOSAR Fls-style internal flash driver on the STM32F1 HAL. Only the app flash area is reachable — the bootloader code region is excluded by the sector table, so an Erase/Write can never destroy the running image.

- **Files**: `Apdapter/Bl_Fls.c/.h` (driver), `Apdapter/Bl_Fls_Cfg.h` (chip-specific config: `BL_FLS_PAGE_SIZE 2` halfword write granularity, `BL_FLS_SECTOR_SIZE 2048`, operating range `BL_FLS_BASE_ADDR 0x08008000` .. `BL_FLS_END_ADDR 0x08080000`).
- **Async job model** (AUTOSAR Fls semantics): `Bl_Fls_Erase/Write/Read` validate (alignment + sector-table range) and start a job, returning immediately; `Bl_Fls_MainFunction()` (called cyclically) executes the job against the hardware and returns the driver to IDLE; `Bl_Fls_GetStatus()` returns UNINIT/IDLE/BUSY (MemIf semantics), `Bl_Fls_GetJobResult()` returns JOB_OK/FAILED/PENDING/CANCELED.
- **Notifications**: weak `Bl_Fls_JobEndNotification()` / `Bl_Fls_JobErrorNotification()` (AUTOSAR Fls_JobEnd/JobError), overridable.
- **Alignment rules**: Erase requires sector alignment (2KB multiple); Write requires page alignment (2-byte/halfword multiple, even length). The write destination must be erased first (F1 flash only programs 1→0).
- **HW access**: `HAL_FLASHEx_Erase` (page erase) / `HAL_FLASH_Program` (halfword). The CPU stalls during a program/erase cycle (interrupts deferred, not lost).
- **Init**: `Bl_Fls_Init()` from `Bl_DriverAdapter_Init()` (SysInit), UNINIT→IDLE.

### Bl_FlashIf — Synchronous Flash Wrapper (Core)

Synchronous convenience layer over the async Fls driver for Core callers (0x36 transfer data, 0x31 erase routines, bootloader jump logic).

- **Files**: `Core/Bl_FlashIf.c/.h`.
- **API**: `Bl_FlashIf_ErasePage(addr)` (one sector), `Bl_FlashIf_Write(addr, data, len)`, `Bl_FlashIf_Read(addr, data, len)` — each starts the Fls job, pumps `Bl_Fls_MainFunction()` until IDLE, then maps the job result to `BL_E_OK / BL_E_NOT_OK`.
- **Layering**: Core → Apdapter stable interface (`Bl_Fls.h`), same pattern as `Bl_TaskSchedule` → `Bl_Can`.
- Hardware-verified: erase → write → read-back compare of an 8-byte pattern at 0x08008000 passed on the bench.

### Bl_DriverAdapter — Driver Initialization Hub (Apdapter)

- `Bl_DriverAdapter_Init()` runs four parallel static init functions (single return via `e_Ret |=`):
  - `s_Bl_DriverAdapter_CddInit()` — CDD modules (currently `OLED_Init()`)
  - `s_Bl_DriverAdapter_TimerInit()` — starts TIM1 in interrupt mode (`HAL_TIM_Base_Start_IT(&htim1)`)
  - `s_Bl_DriverAdapter_CanInit()` — `Bl_Can_Init()`
  - `s_Bl_DriverAdapter_FlsInit()` — `Bl_Fls_Init()`
- `Bl_DriverAdapter_Deinit()` mirrors with `s_Bl_DriverAdapter_CddDeinit()` + `s_Bl_DriverAdapter_TimerDeinit()` + `s_Bl_DriverAdapter_CanDeinit()`
- Init/deinit functions use `/* User Add Begin/End */` markers

### Bl_Isr — Interrupt Callbacks (Apdapter)

- Holds chip-specific interrupt callbacks. Migrated out of `Bl_DriverAdapter` so driver init and interrupt handling stay separate.
- Callbacks:
  - `HAL_TIM_PeriodElapsedCallback` — TIM1 1kHz → `Bl_TaskSchedule_TickInc()`
  - `HAL_CAN_RxFifo0MsgPendingCallback` / `HAL_CAN_RxFifo1MsgPendingCallback` — CAN RX → `Bl_Can_RxIsr(0/1)`
- NVIC vectors `CAN1_RX0_IRQHandler` / `CAN1_RX1_IRQHandler` live in `Core/Src/stm32f1xx_it.c` (USER CODE 1), each calling `HAL_CAN_IRQHandler(&hcan)`.

### Bl_TaskUserdef — User Application (Apdapter)

- `Bl_TaskUserdef_Init(void)` — registers user application tasks (returns `void`; registration result isn't a simple OK/NOT_OK)
- `Bl_TaskUserdef_Deinit(void)` — unregisters all tasks it registered, tracking task IDs in a static array
- Tasks:
  - `s_Bl_TaskUserdef_CounterTask` (1000ms) — 1s uptime counter (increments only; no OLED I/O in the loop)
- No CAN test tasks and no app-level CAN code — the CAN RX/TX path goes driver → CanIf (PDU dispatch) → CanTp; only the 1s uptime counter task is registered.

## Coding Standards

From `01_InputFiles/AI编码规则/文件格式规范.md`:

- **Indentation**: 4 spaces, no tabs
- **File encoding**: UTF-8, CRLF
- **Module naming**: PascalCase (`LedControl`, `Bl_Can`)
- **Function naming**: `<ModuleName>_<Action>(<params>)` — e.g., `Bl_Can_Init()`, `OLED_ShowString()`
- **Variable prefixes**: `g_` global, `s_` static, `p_` pointer
- **Info/Cfg/Config naming**: always subject-qualified — `p_PduInfo`, `g_Bl_CanIf_PduConfig`, `Bl_CanIf_PduConfigType`; bare `p_Info`/`p_Cfg`/`p_Config` forbidden (see `01_InputFiles/AI编码规则/文件格式规范.md` §1.4)
- **Type naming**: `<ModuleName>_<Desc>_t` — e.g., `Bl_Can_ConfigType`, `LedControl_Status_t`
- **Macros**: `UPPER_SNAKE_CASE`
- **Section separators**: 65-char `/*...***/` blocks (e.g., `/**** Includes ****/`)
- **Function docs**: `@brief`, `@param`, `@retval` tags
- **Change log**: Version/Date/Description table in file header, tags: `[New]`, `[Modify]`, `[Fix]`, `[Remove]`, `[Optimize]`
- **EOF marker**: `/******************************* EOF (End of File) ***************************/` on last line

## Project Progress & Current State (2026-08-13)

### Completed

- **Bl_Can CAN driver** (Apdapter): AUTOSAR-style. `Bl_Can_Write` enqueues only; `Bl_Can_MainFunctionWrite` (TX) / `Bl_Can_MainFunctionRead` (RX); driver calls CanIf callbacks (`CanIf_RxIndication/TxConfirmation`); `BusOff` (implemented, software recovery) / `Mode/Wakeup` stubs.
- **Interrupt-driven RX**: CAN RX FIFO0/FIFO1 → `CAN1_RX0/1_IRQHandler` → `HAL_CAN_RxFifo0/1MsgPendingCallback` (Bl_Isr) → `Bl_Can_RxIsr` → software RX ring queue (128 slots) → `MainFunctionRead` → `CanIf_RxIndication`.
- **CubeMX integration (方案 A)**: CAN hardware/timing/MspInit owned by CubeMX (`hcan`, `MX_CAN_Init`, `HAL_CAN_MspInit` in msp.c). `Bl_Can_Init` reuses `hcan` + `HAL_CAN_Init` + filter + start + RX interrupt. main's `MX_CAN_Init` kept (user reconciles later).
- **Bl_Can_Cfg.h moved to Apdapter** (chip-specific config). Removed `BL_CAN_RX_BURST_LIMIT`.
- **Dcm download buffer reserved**: `Bl_Dcm_Cfg.h` + `Bl_Dcm.h/.c` with `BL_DCM_BUFFER_SIZE[2050]` (2048 block + 2 overhead).
- **Bl_CanIf CAN Interface layer** (Core): `Bl_CanIf_Transmit` for upper-layer TX (PDU id from Lcfg table), driver-facing callbacks `CanIf_RxIndication/TxConfirmation` (exact AUTOSAR MCAL symbol names — works with both `Bl_Can` and a real MCAL Can driver) dispatching by PDU to `Bl_CanTp`. Application no longer calls the driver directly.
- **Bl_CanTp transport layer** (Core, Phase 1): ISO 15765-2 SF/FF/CF/FC with flow control and N_As/N_Bs/N_Cr timeouts; diagnostic channel 0x7E0 (RX) / 0x7E8 (TX); reassembly into `BL_DCM_BUFFER_SIZE`; weak upper hooks for the UDS layer.
- **Bl_Dcm service discriminator + Bl_Uds service implementations** (Core): user decision — Dcm only discriminates (SID lookup in const service table, NRC checks 0x11/0x13/0x7F/0x33, then dispatches), Uds implements the actual services. `Bl_Dcm` overrides `Bl_CanTp_UpperRxIndication`; `Bl_Uds` overrides `Bl_CanTp_UpperRxErrorIndication` (download state reset). Service table (type `Bl_Dcm_Service_t` + data `g_Bl_Dcm_ServiceConfig`) lives in the **Config layer** (`Bl_Dcm_Lcfg.h/.c`) — add/remove/re-configure services without touching Core.
- **Bl_Uds services implemented**: 0x10 session (0x50+sub+P2+P2*), 0x11 reset (4 sub-services + **real reset**, respond-then-reset via `Bl_Rte_SystemReset`), 0x27 security (fixed seed/key), 0x34 request download (range check + 0x74 maxBlockLength), 0x36 transfer data (block sequence + length + dest-range checks; flash write TODO — `Bl_FlashIf` ready), 0x37 transfer exit, 0x3E tester present (suppress-bit). NRC helper `Bl_Uds_SendNrc`.
- **Bl_Fls / Bl_FlashIf flash stack** (Apdapter + Core): AUTOSAR Fls-style async internal flash driver (`Bl_Fls`: Erase/Write/Read jobs + MainFunction + GetStatus/GetJobResult + weak job notifications, sector/page alignment + app-area-only sector table) and a synchronous Core wrapper (`Bl_FlashIf`: ErasePage/Write/Read with in-line MainFunction pump). **Hardware-verified**: erase → write → read-back compare of an 8-byte pattern at 0x08008000 passed (result reported over CAN 0x7E9). The one-shot bring-up test task was removed afterwards.
- **Bl_TimingManager centralized timing** (Core, user-named): all protocol timeouts (S3/P2/P2*/N_As/N_Bs/N_Cr) defined once in `Bl_TimingManager_Cfg.h`, aliased by `Bl_CanTp_Cfg.h` and `Bl_Uds_Cfg.h`; generic Start/Stop/IsExpired/GetRemaining timer service over `Bl_TaskSchedule_GetTickMs` (wrap-safe). Init from RTE.
- **Back-to-back response ordering fix** (Bl_Uds): CanTp TX is single-session, so Uds queues responses (`BL_UDS_RESPONSE_QUEUE_DEPTH 4` ring) and flushes them from `Bl_CanTp_UpperTxConfirmation` — verified on hardware with 10/10 back-to-back stress rounds (`10 01`+`10 02`+`27 01` → ordered `50 01 → 50 02 → 67 01 5A`).
- **Test tasks**: removed — the KEY0 send / KEY1 statistics tasks and the CanIf upper-layer hooks are gone; `CanIf_RxIndication/TxConfirmation` are guarded no-ops awaiting the CanTp layer. Only the 1s uptime counter task remains.

### Current BL_Platform file map

- Common: `Bl_Types.h`
- Core: `Bl_TaskSchedule.c/.h`, `Bl_Rte.c/.h`, `Bl_TimingManager.c/.h`, `Bl_Dcm.c/.h`, `Bl_Uds.c/.h`, `Bl_UdsService.c/.h`, `Bl_CanIf.c/.h`, `Bl_CanTp.c/.h`, `Bl_FlashIf.c/.h`
- Config: `Bl_TaskSchedule_Cfg.h`, `Bl_TaskSchedule_Lcfg.c/.h`, `Bl_TimingManager_Cfg.h`, `Bl_Dcm_Cfg.h`, `Bl_Dcm_Lcfg.c/.h`, `Bl_Uds_Cfg.h`, `Bl_UdsService_Lcfg.c/.h`, `Bl_CanIf_Cfg.h`, `Bl_CanIf_Lcfg.c/.h`, `Bl_CanTp_Cfg.h`
- Apdapter: `Bl_DriverAdapter.c/.h`, `Bl_Isr.c/.h`, `Bl_TaskUserdef.c/.h`, `Bl_Can.c/.h`, `Bl_Can_Cfg.h`, `Bl_Fls.c/.h`, `Bl_Fls_Cfg.h`

### Key facts / gotchas

- CAN clock macros `__HAL_RCC_CAN1_CLK_ENABLE/DISABLE` live in `stm32f1xx_hal_rcc_ex.h` (NOT the main `_rcc.h`), guarded by `#if defined(STM32F103xE)`. STM32F103xE is defined in the Keil project → macro is available.
- CAN pins are PB8(RX)/PB9(TX) via AFIO remap — `__HAL_AFIO_REMAP_CAN1_2()` lives in the `HAL_CAN_MspInit` USER CODE block (survives CubeMX regeneration). `test.ioc` carries `PB8.Signal=CAN_RX` / `PB9.Signal=CAN_TX`, and CubeMX regeneration emits the same remap call in the generated zone (verified against the workspace-generated copy). 500 kbps = APB1 8 MHz / (Prescaler 1 × 16 TQ), BS1=11, BS2=4, SJW=1.
- `Bl_TaskSchedule_MainFunction()` (Core) directly calls `Bl_Can_MainFunctionWrite/Read()` (Apdapter) — intentional interface dependency.
- Software RX ring queue is SPSC with one empty slot → 127 usable of 128 (`BL_CAN_RX_QUEUE_DEPTH`). Drops are counted in `s_Bl_Can_RxOverflow`, readable via `Bl_Can_GetRxOverflow()`. Main loop must stay free of blocking I2C (OLED) writes.
- `BL_DCM_BUFFER_SIZE` (2050 = `BL_UDS_TRANSFER_BLOCK_SIZE` 2048 + `BL_DCM_TRANSFER_OVERHEAD` 2) is the CanTp RX reassembly buffer AND the Dcm download buffer (zero-copy). Referenced since CanTp reassembles into it.
- `BL_UDS_TRANSFER_BLOCK_SIZE`/`BL_UDS_RESPONSE_BUFFER_LEN`/`BL_UDS_APP_FLASH_BASE_ADDR`/`BL_UDS_APP_FLASH_MAX_SIZE` live in `Bl_Uds_Cfg.h`; `Bl_Dcm_Cfg.h` includes it for `BL_DCM_BUFFER_LEN`.
- Protocol timeout values live ONLY in `Bl_TimingManager_Cfg.h` — `Bl_CanTp_Cfg.h` (`BL_CANTP_N_*_TIMEOUT_MS`) and `Bl_Uds_Cfg.h` (`BL_UDS_P2/P2STAR_DEFAULT_MS`) alias them. Never redefine raw values in those files.
- Flash: STM32F103VE sectors are 2KB; write granularity is halfword (2B). `Bl_Fls`'s sector table only allows the app area (0x08008000..0x08080000) — the bootloader code region is unreachable, so an Erase/Write can never brick the running image. **Always erase before write** (F1 flash only programs 1→0); a write to a non-erased location fails the job (result FAILED).
- Hardware test tooling: repo root `uds_tool.py` drives ZLG USBCAN via `usbcan.dll` (see **CAN Test Bench** section above). ZCANPRO GUI must be closed (it holds the USB device); a stuck adapter (TX blocks ~1.7–5 s, ErrCode=0x2) is fixed only by physically replugging the USB. Always send ISO-TP-framed requests (`[02 10 01]`), never raw `[10 01]` — CanTp parses `data[0]` as PCI.
- `s_Bl_TaskSchedule_TickMs` (32-bit, wraps every ~49.7 days) is safe to use ONLY with unsigned differential timing: `(now - start) >= timeout`, or the wrap-safe deadline idiom `(now - deadline) < 0x80000000UL`. NEVER use absolute comparisons (`now >= start + timeout`), never store the tick in a 16/8-bit variable, never cast to signed. All existing code (scheduler, CanTp, TimingManager) already follows this — keep it that way in new code.
- UV4.exe `-b` writes `build.log` asynchronously; read it after the build finishes (stale partial logs can show misleading errors).

### Next steps (TODO)

1. ISO-TP / DoCAN layer: **Phase 1 done (Bl_CanTp: SF/FF/CF/FC, FC, timeouts, 0x7E0/0x7E8)**; Phase 2 = 29-bit/extended addressing, WAIT/OVF polish, multi-connection, `Bl_CanTp_GetTxBuffer`.
2. UDS services: 0x10/0x11/0x27/0x34/0x36/0x37/0x3E **done in Bl_Uds** (discrimination in Bl_Dcm; 0x11 resets for real); remaining: wire 0x36 data write into `Bl_FlashIf` (erase-before-write download flow), 0x37 verify/finalize + jump-to-app, real seed/key algorithm, 0x22/0x2E/0x31, 0x78 pending path.
3. Flash download flow: `Bl_Fls`/`Bl_FlashIf` **done and hardware-verified**; remaining: 0x36 → `Bl_FlashIf_Write` (with sector erase strategy + partial last block), 0x37 checksum/verify, `JumpToApp` (bootloader → app handover).
4. `Bl_Can_MainFunctionMode/Wakeup` implementations (AUTOSAR stubs).
5. Verify CAN pins (PB8/PB9, AFIO remap) against the board schematic before hardware bring-up.
6. Reconcile CubeMX `MX_CAN_Init` double-init with `Bl_Can_Init` (user will handle).

# AUTOSAR 标准的 UDS 实现架构（本工程适配版）

> 适用工程：STM32F103VET6 Bootloader（`BL_Platform` 分层框架）
> 相关标准：ISO 14229-1（UDS 应用层）、ISO 15765-2（CAN 传输层 CanTp）、ISO 11898（CAN 数据链路）
> 目标：按 AUTOSAR 的诊断分层思想，实现 **UDS 刷写（Bootloader）** 完整链路

---

## 1. 引言

### 1.1 目的

本文档描述在本工程中按 AUTOSAR 标准架构实现 UDS（统一诊断服务）的**分层结构、模块职责、服务框架、刷写流程**以及与现有代码的对接方式，作为 `Bl_Dcm` 层及后续 Flash 驱动开发的实现依据。

### 1.2 标准参考

| 标准 | 内容 | 本工程对应 |
|---|---|---|
| ISO 14229-1 | UDS 应用层（服务定义、NRC、会话） | `Bl_Dcm`（判别）+ `Bl_Uds`（服务实现） |
| ISO 15765-2 | CAN 传输层（SF/FF/CF/FC、流控） | `Bl_CanTp`（已建，Phase 1） |
| ISO 11898-1/2 | CAN 数据链路/物理层 | `Bl_Can` 驱动（已建） |

### 1.3 诊断通信模型

UDS 是**请求-响应**模型：测试仪（Tester/上位机）在 CAN 总线上发诊断请求，ECU 收到后处理并回响应。本工程 ECU 为**服务端（Server）**。

---

## 2. AUTOSAR 诊断架构（标准视角）

AUTOSAR 中诊断功能由一组 BSW 模块协作完成：

```
┌──────────────────────────────────────────────────────────────┐
│  Application / SWC   （诊断应用，如 Bootloader 应用逻辑）      │
├──────────────────────────────────────────────────────────────┤
│  Dcm（Diagnostic Communication Manager）诊断会话/服务分发     │
│    · 会话管理（默认/编程/扩展 + S3 超时）                     │
│    · 服务分发（SID 查表、NRC 检查、响应组装）                 │
│    · 安全访问（Seed/Key 校验）                                │
├──────────────────────────────────────────────────────────────┤
│  PduR（PDU Router）SDU 级路由（Dcm ↔ CanTp ↔ 其他）           │
├──────────────────────────────────────────────────────────────┤
│  CanTp（ISO 15765-2 传输层）分段/重组/流控/超时               │
├──────────────────────────────────────────────────────────────┤
│  CanIf（CAN 接口层）PDU 路由、收发                             │
├──────────────────────────────────────────────────────────────┤
│  Can（CAN 驱动，MCAL）队列/中断/寄存器                        │
├──────────────────────────────────────────────────────────────┤
│  CanSM / ComM / EcuM 状态与模式管理                           │
└──────────────────────────────────────────────────────────────┘
```

**AUTOSAR 数据流（请求 RX / 响应 TX）**：

```
测试仪 ──0x7E0──▶ Can ──▶ CanIf ──▶ CanTp(RX) ──▶ PduR ──▶ Dcm ──▶ 应用
测试仪 ◀─0x7E8── Can ◀─ CanIf ◀─ CanTp(TX) ◀─ PduR ◀─ Dcm ◀─ 应用
```

**关键点**：下层只传数据不解析内容——Can/CanIf 认 L-PDU，CanTp 认 SDU（完整消息），Dcm 认诊断服务（SID + 参数）。每一层只关心自己那一级的"数据单元"。

---

## 3. 本工程的简化架构映射

本工程无完整 BSW，按 AUTOSAR 分层思想做了等价映射，**删去了 PduR**（SDU 路由由层间直连承担）：

| AUTOSAR 模块 | 本工程模块 | 层 | 状态 | 职责 |
|---|---|---|---|---|
| Dcm | `Bl_Dcm` | Core | ✅ 判别器 | 服务判别（SID 查表、NRC 检查 0x11/0x13/0x7F/0x33）、分发到 Bl_Uds |
| Uds | `Bl_Uds` | Core | ✅ 服务实现 | 会话/安全/下载等各服务函数、NRC 应答、下载状态机 |
| Timing | `Bl_TimingManager` | Core | ✅ | 协议时限集中定义（S3/P2/P2*/N_*）+ 通用软件定时服务 |
| PduR | （无，直连） | — | — | CanTp 与 UDS 直接调用 |
| CanTp | `Bl_CanTp` | Core | ✅ Phase 1 | SF/FF/CF/FC、流控、N_As/N_Bs/N_Cr 超时 |
| CanIf | `Bl_CanIf` | Core | ✅ | PDU 路由（HOH+ID 掩码）、收发、直连 CanTp |
| Can | `Bl_Can` | Apdapter | ✅ | 软件队列、中断 RX、Bus-Off 恢复 |
| — | `Bl_Can`（HAL） | Apdapter | ✅ | bxCAN 寄存器/GPIO/中断（CubeMX） |

**本工程完整目标链路**：

```
上位机 ──0x7E0/0x7DF──▶ Bl_Can ──▶ Bl_CanIf(PDU 路由) ──▶ Bl_CanTp(重组) ──▶ Bl_Dcm(服务处理)
上位机 ◀─0x7E8─────── Bl_Can ◀─ Bl_CanIf ◀─ Bl_CanTp(分段) ◀─ Bl_Dcm(响应)
```

**与标准的主要取舍**：

| 项 | 标准 AUTOSAR | 本工程（简化） |
|---|---|---|
| SDU 路由 | PduR 模块 | CanTp → UDS 直连（弱钩子） |
| 寻址 | 物理 + 功能 + 29 位 | Phase 1 仅 11 位物理（0x7E0/0x7E8），0x7DF 功能寻址后续 |
| 连接 | 多通道多连接 | 单通道，单 RX + 单 TX 会话 |
| 传输超时 | 全部 N_* 定时器 | Phase 1：N_As / N_Bs / N_Cr |
| 会话/安全 | Dcm 完整实现 | Bl_Dcm 判别 + Bl_Uds 实现（会话 + 安全访问） |

---

## 4. UDS 协议基础（ISO 14229）

### 4.1 报文格式

```
请求:      [SID] [Sub-function/参数...]
正响应:    [SID+0x40] [Sub-function(如有)/数据...]
负响应:    [0x7F] [SID] [NRC]
```

### 4.2 刷写相关服务（本工程重点）

| SID | 服务 | 子功能/参数 | 正响应 | 说明 |
|---|---|---|---|---|
| 0x10 | DiagnosticSessionControl | 01=默认 02=编程 03=扩展 | 0x50 | 回 P2/P2* |
| 0x11 | ECUReset | 01=硬复位 03=软复位 | 0x51 | 刷写结束复位跳转 |
| 0x27 | SecurityAccess | 01=请求Seed 02=发送Key | 0x67 | 刷写前解锁 |
| 0x34 | RequestDownload | 数据格式+地址+长度 | 0x74 | 回最大块长度 |
| 0x36 | TransferData | 块序号+数据 | 0x76 | 逐块下发 |
| 0x37 | RequestTransferExit | 无 | 0x77 | 结束传输 |
| 0x3E | TesterPresent | 00（80=抑制响应） | 0x7E | 保活会话 |
| 0x22 | ReadDataByIdentifier | DID | 0x62 | 读标识（版本/SN） |
| 0x2E | WriteDataByIdentifier | DID+数据 | 0x6E | 写标识（可后置） |
| 0x31 | RoutineControl | 01/02/03+RID | 0x71 | 例程（校验/擦除，可后置） |
| 0x28 | CommunicationControl | — | 0x68 | 通信控制（可后置） |

### 4.3 常用 NRC（负响应码）

| NRC | 含义 | 典型触发 |
|---|---|---|
| 0x10 | generalReject | 条件不允许 |
| 0x11 | serviceNotSupported | SID 未实现 |
| 0x12 | subFunctionNotSupported | 子功能不支持 |
| 0x13 | incorrectMessageLengthOrInvalidFormat | 长度/格式错误 |
| 0x22 | conditionsNotCorrect | 前置条件不满足（如未进编程会话） |
| 0x24 | requestSequenceError | 顺序错误（如未 0x34 就 0x36） |
| 0x31 | requestOutOfRange | 参数越界（地址/长度非法） |
| 0x33 | securityAccessDenied | 未解锁 |
| 0x35 | invalidKey | Key 错误 |
| 0x36 | exceededNumberOfAttempts | 尝试次数超限 |
| 0x37 | requiredTimeDelayNotExpired | 延时未到（防暴力破解） |
| 0x78 | requestCorrectlyReceived-ResponsePending | 中间响应（处理时间长时） |
| 0x7E / 0x7F | subFunction/serviceNotSupportedInActiveSession | 当前会话不支持 |

### 4.4 寻址

| 类型 | CAN ID | 方向 | 说明 |
|---|---|---|---|
| 物理请求 | 0x7E0 | RX | 发给本 ECU（CanTp RX 连接） |
| 物理响应 | 0x7E8 | TX | 本 ECU 回给测试仪（CanTp TX 连接） |
| 功能请求 | 0x7DF | RX | 广播给所有 ECU（Phase 2） |

### 4.5 会话状态机

```
默认会话(0x01) ──0x10 02──▶ 编程会话(0x02) ──0x10 01──▶ 默认
     ▲                      │  ▲                        │
     └── S3 超时(5s) ◀──────┘  └── 0x10 03 ──▶ 扩展会话(0x03)
     （任何会话 S3 超时都回默认）
```

- **S3 超时**：5s 内无任何诊断请求 → 回默认会话（影响服务可用性）。
- **P2 / P2***：服务端响应时限。P2 默认 50ms（快速响应），P2* 默认 5000ms（慢服务如擦除 Flash 用，期间先回 0x78）。

---

## 5. 诊断服务处理框架设计（Bl_Dcm 判别 + Bl_Uds 实现）

> **职责拆分（用户决策）**：`Bl_Dcm` 只负责"判别"——收到的数据属于哪个服务、当前会话/安全是否允许、长度是否合法；判别通过后调用 `Bl_Uds` 中实现的具体服务函数。服务怎么处理、回什么响应，全在 `Bl_Uds`。

### 5.1 模块结构

```
BL_Platform/
├── Core/Bl_Dcm.c/.h          判别器：服务表查找 + NRC 判别（0x11/0x13/0x12/0x7F/0x33）+ 分发
├── Core/Bl_Uds.c/.h          服务实现：0x10/0x11/0x27/0x34/0x36/0x37/0x3E + NRC 应答 + 下载状态机
├── Config/Bl_Dcm_Cfg.h       下载缓冲配置（BL_DCM_BUFFER_LEN = 块 2048 + 开销 2，include Bl_Uds_Cfg.h）
├── Config/Bl_Dcm_Lcfg.h/.c   **诊断服务表**（类型 Bl_Dcm_Service_t + 数据 g_Bl_Dcm_ServiceConfig）
│                             —— 增删/改配服务只动这里，Core 判别逻辑不用改
├── Config/Bl_Uds_Cfg.h       UDS 配置（SID/NRC/会话/安全/P2/P2*/块大小/应用 Flash 范围）
└── Apdapter/（后续）          Flash 驱动（擦除/写入/校验，芯片相关）
```

### 5.2 请求处理主流程（Bl_Dcm 判别 → Bl_Uds 实现）

```
Bl_CanTp_UpperRxIndication(pduId, sdu, len)      ← Bl_Dcm 覆盖 CanTp 弱钩子
  │
  ├─ 1. 解析 SID；查服务表 g_Bl_Dcm_ServiceConfig[]
  │      └─ 未找到 → 回 NRC 0x11（serviceNotSupported）
  ├─ 2. NRC 判别（按序，全部通过才调用实现）：
  │      · 请求长度 < 表内 minLen → 0x13（incorrectMessageLength）
  │      · 当前会话不在 sessionMask → 0x7F（serviceNotSupportedInActiveSession）
  │      · 需要安全访问且未解锁 → 0x33（securityAccessDenied）
  ├─ 3. 调用 Bl_Uds 服务实现函数（表内 p_Func）
  │      └─ 各服务在 Bl_Uds.c 内实现（可能回中间响应 0x78，如 Flash 擦除）
  ├─ 4. 服务实现组装正响应（SID+0x40 [+子功能] [+数据]）
  │      · 若请求子功能最高位=1（抑制正响应）→ 不回
  └─ 5. Bl_CanTp_Transmit(DIAG_TX_PDU, 响应, len)
        └─ 发送结果经 Bl_CanTp_UpperTxConfirmation 通知（超时/失败处理）
```

### 5.3 服务表设计（Config 层 Bl_Dcm_Lcfg.h/.c）

> **配置下沉**：服务表类型与数据位于 **Config 层**（`Bl_Dcm_Lcfg.h` 定义类型，`Bl_Dcm_Lcfg.c` 实例化 `g_Bl_Dcm_ServiceConfig[]`），后续增删/改配服务只需编辑 `Bl_Dcm_Lcfg.c`，Core 判别逻辑无需改动。

```c
typedef void (*Bl_Uds_ServiceFunc_t)(const bl_uint8_t *p_Req, bl_uint32_t u32_ReqLen);

typedef struct {
    bl_uint8_t           u8_Sid;              /* 主服务字节（如 0x10）            */
    bl_uint8_t           u8_SubFuncLen;       /* 子服务字节长度（0x10 01 → 1；无子服务 → 0）；
                                                 为未来按服务单独配置子服务预留    */
    bl_uint8_t           u8_SubFuncSupported; /* 子功能支持位图（bit N = 子功能 0xN，
                                                 覆盖 0x00..0x07；0xFF = 全部接受）  */
    bl_uint8_t           u8_SuppressBit;      /* 1 = 允许子功能 0x80 抑制正响应
                                                 （如 0x3E；0x10 不允许）            */
    bl_uint8_t           u8_MinLen;           /* 最小请求长度                      */
    bl_uint8_t           u8_SessionMask;      /* 允许的会话位掩码（bit0 默认/bit1 编程/bit2 扩展） */
    bl_uint8_t           u8_SecurityNeeded;   /* 是否需要安全访问                  */
    bl_uint8_t           u8_P2Override;       /* P2  覆盖（0xFF = 用 Bl_TimingManager 默认；
                                                 慢服务可单独配置，Dcm 消费）       */
    bl_uint8_t           u8_P2StarOverride;   /* P2* 覆盖（0xFF = 默认）           */
    bl_uint8_t           u8_RespDataLen;      /* 正响应数据长度（不含 SID+子服务；
                                                 定长服务为固定值，变长服务为上界）   */
    Bl_Uds_ServiceFunc_t p_Func;              /* 处理函数（Bl_Uds.c 中的实现）     */
} Bl_Dcm_Service_t;
```

已注册 7 条服务（`g_Bl_Dcm_ServiceConfig[]`）：0x10（子功能 01/02/03，禁抑制位）/0x11（子功能 01）/0x27（子功能 01/02）/0x3E（子功能 00，允许抑制位）全会话（mask 0x07）；0x34/0x36/0x37（无子功能）仅编程会话（mask 0x02）且需已解锁（security=1）。

**判别流程**（Bl_Dcm）：查表（未找到→0x11）→ 长度（<minLen→0x13）→ 子功能（长度不足/位图不支持/禁抑制位却带 0x80 → 0x12）→ 会话（0x7F）→ 安全（0x33）→ 调 handler。

**响应缓冲策略**：正响应统一使用 `Bl_Uds` 的 `s_Bl_Uds_Resp[64]`（**不**复用 `BL_DCM_BUFFER_SIZE` 大缓冲，后者只作接收重组/下载缓冲）；`u8_RespDataLen` 用于按服务校验/计算总长（总长 = 1 + 子服务长度 + RespDataLen），未来如需每服务独立响应数组，从该字段即可拆。

### 5.4 定时器（Bl_TimingManager 集中管理）

所有协议时限统一在 `Config/Bl_TimingManager_Cfg.h` 定义（**单一来源**），各模块通过宏别名引用；`Bl_TimingManager`（Core）提供通用软件定时服务（`Start/Stop/IsExpired/GetRemaining`，基于 `Bl_TaskSchedule_GetTickMs()` 32 位差分校时，回绕安全）：

| 定时器 | TimerId | 默认 | 消费方 |
|---|---|---|---|
| S3 | `BL_TIMINGMANAGER_TIMER_S3` | 5000ms | Dcm 会话超时（回到默认会话） |
| P2 | `BL_TIMINGMANAGER_TIMER_P2` | 50ms | Uds 响应时限（0x10 响应中通告） |
| P2* | `BL_TIMINGMANAGER_TIMER_P2STAR` | 5000ms | Uds 慢服务响应时限（0x78 pending） |
| N_As | `BL_TIMINGMANAGER_TIMER_N_AS` | 1000ms | CanTp TX 帧确认（宏别名） |
| N_Bs | `BL_TIMINGMANAGER_TIMER_N_BS` | 1000ms | CanTp 等 FC（宏别名） |
| N_Cr | `BL_TIMINGMANAGER_TIMER_N_CR` | 1000ms | CanTp 等下一 CF（宏别名） |

CanTp 状态机保留自有帧定时器，但超时**值**来自本模块 Cfg（别名）；S3/P2/P2* 由 Dcm/Uds 通过定时服务使用。

---

## 6. 刷写（下载）流程设计（核心目标）

### 6.1 完整刷写时序

```
测试仪                                      ECU（Bootloader）
  │  0x10 03（进入编程会话）                     │
  ├───────────────────────────────────────────▶ 0x50 03 P2 P2*
  │  0x27 01（请求 Seed）                       │
  ├───────────────────────────────────────────▶ 0x67 Seed
  │  0x27 02（发送 Key）                        │
  ├───────────────────────────────────────────▶ 0x67 02
  │  0x34（请求下载：地址 0x08008000, 长度 N）    │
  ├───────────────────────────────────────────▶ 0x74 最大块长=0x800(2048)
  │  0x36 01 [2KB 数据]                         │
  ├───────────────────────────────────────────▶ 0x76 01
  │  0x36 02 [2KB 数据]                         │
  ├───────────────────────────────────────────▶ 0x76 02
  │  ...（0x36 循环直到传完）                    │
  │  0x37（请求传输退出）                        │
  ├───────────────────────────────────────────▶ 0x77
  │  0x11 01（复位到新程序）                     │
  ├───────────────────────────────────────────▶ 0x51 01
```

### 6.2 各服务实现要点（Bootloader 版）

| 服务 | 处理逻辑 |
|---|---|
| **0x10 03** | 允许任何会话进入；回 0x50 + 03 + P2(0x0032) + P2*(0x1388) |
| **0x27** | 01：生成 Seed（可基于随机数/时间）；02：校验 Key（固定算法，可配置加解锁延时 + 尝试次数上限） |
| **0x34** | 校验地址+长度在合法应用区；回最大块长度 = 0x800（2KB，与 Flash 页对齐）；置"下载就绪"状态 |
| **0x36** | 校验"已 0x34"且块序号递增；数据拷入 `BL_DCM_BUFFER_SIZE`；整块收齐后调 Flash 驱动写入；回块号 |
| **0x37** | 校验"已 0x34"；结束传输状态；回 0x77 |
| **0x3E** | 刷新 S3 计时；回 0x7E（或抑制） |
| **0x11 01** | 复位（软件复位 → 跳转应用） |

### 6.3 块大小与 Flash 页对齐（关键设计）

STM32F103VE 高密度器件 **Flash 页 = 2KB**，因此：

- `BL_UDS_TRANSFER_BLOCK_SIZE = 2048`（单块 0x36 正好写一页，省去跨页拼接）
- 缓冲数组 `BL_DCM_BUFFER_SIZE[2050]`（= 块 2048 + 0x36 协议开销 2；CanTp 重组缓冲 == 下载块缓冲，**零拷贝复用**）
- 响应 `0x74` 中的 maxNumberOfBlockLength = 0x0800

### 6.4 Flash 驱动接口（Apdapter，待建）

```c
bl_ret_t Bl_Flash_ErasePage(bl_uint32_t u32_Addr);           /* 擦一页 2KB */
bl_ret_t Bl_Flash_Write(bl_uint32_t u32_Addr, const bl_uint8_t *p_Data, bl_uint32_t u32_Len);
bl_ret_t Bl_Flash_Verify(bl_uint32_t u32_Addr, const bl_uint8_t *p_Data, bl_uint32_t u32_Len);
bl_ret_t Bl_Flash_JumpToApp(bl_uint32_t u32_AppAddr);        /* 校验向量表后跳转 */
```

注意：擦除/写入期间需关闭全局中断或妥善处理中断（Flash 操作时总线等待），长操作配合 0x78 中间响应。

---

## 7. 与现有代码的对接

### 7.1 覆盖 CanTp 上层钩子（Bl_Dcm.c + Bl_Uds.c）

```c
/* Bl_Dcm.c 覆盖：收到完整诊断请求 → 判别（查表 + NRC）→ 调 Bl_Uds 服务实现 */
void Bl_CanTp_UpperRxIndication(Bl_CanIf_PduIdType u16_PduId,
                                const bl_uint8_t *p_Sdu, bl_uint32_t u32_SduLen);

/* （当前无覆盖，weak no-op）响应发送完成/失败 → 更新诊断会话状态 */
void Bl_CanTp_UpperTxConfirmation(Bl_CanIf_PduIdType u16_PduId, bl_uint8_t u8_Result);

/* Bl_Uds.c 覆盖：接收会话被掐断（SN 错/N_Cr 超时）→ 复位下载状态（块号、就绪标志） */
void Bl_CanTp_UpperRxErrorIndication(Bl_CanIf_PduIdType u16_PduId);
```

### 7.2 发送响应

```c
/* 组好响应 buffer 后： */
(void)Bl_CanTp_Transmit(BL_CANTP_CANIF_TX_PDU_ID /* = DIAG_TX 0x7E8 */,
                        p_Resp, u8_RespLen);
```

### 7.3 周期任务接入

- UDS 会话定时器（S3 超时检测）可用 `Bl_TimingManager`（`BL_TIMINGMANAGER_TIMER_S3`，`IsExpired()` 检测，超时回默认会话）；
- 建议注册一个周期任务（如 100ms）或在主循环统一处理会话超时与 Flash 状态机。

### 7.4 配置点（Bl_Uds_Cfg.h 现状 + 扩展）

| 配置 | 值 | 说明 |
|---|---|---|
| `BL_UDS_TRANSFER_BLOCK_SIZE` | 2048 | 0x36 单块字节数（= Flash 页），位于 Bl_Uds_Cfg.h |
| `BL_DCM_BUFFER_LEN` | `BL_UDS_TRANSFER_BLOCK_SIZE + BL_DCM_TRANSFER_OVERHEAD(2)` = 2050 | 下载/重组缓冲长度，位于 Bl_Dcm_Cfg.h（include Bl_Uds_Cfg.h） |
| `BL_DCM_BUFFER_SIZE`（数组） | 2050 | 下载/重组缓冲，长度取 `sizeof()` |
| `BL_UDS_APP_FLASH_BASE_ADDR` / `MAX_SIZE` | 0x08008000 / 0x000F8000 | 应用区范围（0x34 校验用，按实际 layout 确认） |
| （扩展）`BL_DCM_DIAG_RX_CAN_ID` / `TX` | 0x7E0 / 0x7E8 | 物理寻址（对应 CanIf PDU 表） |
| （扩展）`BL_DCM_FUNC_RX_CAN_ID` | 0x7DF | 功能寻址（Phase 2） |
| （扩展）`BL_DCM_S3_TIMEOUT_MS` / `P2` / `P2*` | 5000 / 50 / 5000 | 会话/响应时限（实际值在 Bl_TimingManager_Cfg.h） |
| （扩展）安全 Key 算法 | 固定 seed 0x5A / key 0xA5 | 刷写安全（当前为占位实现） |

> **时限配置单一来源**：S3/P2/P2*/N_As/N_Bs/N_Cr 的实际值只定义在 `Config/Bl_TimingManager_Cfg.h`；`Bl_Uds_Cfg.h` 与 `Bl_CanTp_Cfg.h` 通过宏别名引用，勿在别处重复定义原始值。

---

## 8. 安全性与健壮性

1. **地址范围校验**：0x34 的地址+长度必须落在**应用区**（如 0x08008000 ~ 0x0807FFFF），严禁覆盖 Bootloader 区 → 回 0x31。
2. **NRC 检查顺序固定**：长度 → 会话 → 安全 → 顺序 → 范围，避免信息泄露（如未解锁时统一回 0x33 而不是 0x35）。
3. **安全访问防暴力**：Key 错误次数上限（0x36 exceededNumberOfAttempts）+ 延时（0x37）。
4. **下载中断恢复**：0x36 中途失败/超时（`UpperRxErrorIndication`）→ 复位块号与就绪标志，防止旧数据污染新会话。
5. **0x78 中间响应**：Flash 擦除等长操作先回 0x78，测试仪超时用 P2* 处理。
6. **响应抑制位**：子功能最高位为 1 时不回正响应（减少总线占用）。
7. **跳转前校验**：0x11 复位前确认下载已完成且校验通过（可加 CRC/比较校验）。

---

## 9. 实施阶段规划

| 阶段 | 内容 | 依赖 |
|---|---|---|
| **Phase 1（当前）** | Bl_Dcm 判别器 + Bl_Uds 服务实现：0x10/0x3E/0x11/0x27/0x34/0x36/0x37、服务表、NRC 判别、响应组装；跑通 0x7E0/0x7E8 请求-响应往返（0x36 的 Flash 写入为 TODO） | Bl_CanTp ✅ |
| **Phase 2** | Flash 驱动（擦/写/校验）+ 0x36 实际写入 + 0x11 复位触发 + 真实 seed/key 算法 | Phase 1 |
| **Phase 3** | 0x22/0x2E/0x31、功能寻址 0x7DF、29 位寻址、0x78 中间响应完善、多块/多连接 | Phase 2 |

---

## 10. 附录：关键常量速查

| 常量 | 值 | 说明 |
|---|---|---|
| 物理请求 ID | 0x7E0 | CanIf PDU `DIAG_RX` |
| 物理响应 ID | 0x7E8 | CanIf PDU `DIAG_TX` |
| 功能请求 ID | 0x7DF | Phase 2 |
| SF 最大数据 | 7 字节 | CanTp |
| FF/CF 数据 | 6 / 7 字节 | CanTp |
| 下载块大小 | 2048（0x800） | = Flash 页；重组缓冲 = 块 + 2 开销 = 2050 |
| 编程会话 | 0x02 | Bootloader 起始会话可用 0x02 |
| P2 / P2* / S3 | 50ms / 5000ms / 5000ms | 会话与响应时限（Bl_TimingManager_Cfg.h） |
| N_As / N_Bs / N_Cr | 1000ms | CanTp 传输超时（Bl_TimingManager_Cfg.h 别名，已实现） |

---

*文档版本 V1.0.0（2026-08-16）—— 基于当前工程分层（Bl_Can/Bl_CanIf/Bl_CanTp/Bl_TimingManager/Bl_Dcm/Bl_Uds）编写，随 Bl_Dcm/Bl_Uds 实现迭代更新。*

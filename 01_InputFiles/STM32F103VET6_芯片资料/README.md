# STM32F103VET6 芯片资料

## 芯片概述

STM32F103VET6 是意法半导体（STMicroelectronics）推出的基于 **ARM Cortex-M3** 内核的 **32 位高密度性能微控制器**。属于 STM32F103 系列中存储容量最大的 LQFP-100 封装型号。

## 型号解析

| 字段 | 含义 |
|------|------|
| STM32 | ARM 32 位 MCU |
| F | 通用型 |
| 103 | 性能线 |
| V | 100 引脚 |
| E | 512 KB Flash |
| T | LQFP 封装 |
| 6 | 工业温度范围（-40°C 至 +85°C） |

## 核心规格速览

| 参数 | 规格 |
|------|------|
| 内核 | ARM Cortex-M3 32 位 RISC |
| 最大主频 | 72 MHz（1.25 DMIPS/MHz） |
| Flash | 512 KB |
| SRAM | 64 KB |
| 工作电压 | 2.0V – 3.6V |
| 工作温度 | -40°C 至 +85°C |
| 封装 | LQFP-100（14×14 mm，0.5 mm 间距） |
| I/O 引脚 | 80 个 |

## 外设一览

| 类别 | 外设 |
|------|------|
| ADC | 3× 12 位（1 Msps，最多 21 通道） |
| DAC | 2× 12 位 |
| 高级定时器 | 2× 16 位（TIM1、TIM8，电机控制 PWM） |
| 通用定时器 | 4× 16 位（IC/OC/PWM/编码器） |
| 基本定时器 | 2× 16 位 |
| 通信接口 | 5× USART、3× SPI（2 个可复用 I²S）、2× I²C、USB 2.0 FS、CAN 2.0B、SDIO |
| DMA | 12 通道 |
| 存储控制 | FSMC（支持 SRAM/NOR/NAND/PSRAM/LCD） |
| 调试 | SWD + JTAG |
| 其他 | CRC 单元、96 位唯一 ID、RTC、2× 看门狗 |

## 本目录文件说明

- `01_规格参数.md` — 详细规格参数表
- `02_引脚参考.md` — 引脚分布与功能说明
- `03_参考资料与链接.md` — 官方手册下载链接与学习资源
- `docs/` — 原始官方手册 PDF

### docs/ 目录（已下载官方原始资料）

| 文件 | 大小 | 说明 |
|------|------|------|
| `STM32F103VET6_Datasheet_ST.pdf` | 3.2 MB | STM32F103xC/D/E 数据手册（Doc ID 14611），含电气特性、引脚定义、封装 |
| `RM0008_STM32F10xxx_Reference_Manual.pdf` | 13 MB | STM32F10xxx 参考手册，外设寄存器、时钟、中断详解（~1136 页） |
| `PM0056_Cortex_M3_Programming_Manual.pdf` | 2.0 MB | Cortex-M3 编程手册，内核指令集与系统控制 |
| `PM0075_Flash_Programming_Manual.pdf` | 296 KB | STM32F10xxx Flash 编程手册 |

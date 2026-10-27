# MiSTer 中文 OSD：256×256/224p 高度实验（未完成 FPGA 固件）

本实验目标为完整 **256×256 像素 OSD**，总高度包含独立顶部状态栏。224p 为兼容布局。已更新 `test_memory_map.py` 检查地址与高度预算。

## 布局预算

| 输出模式 | 顶部状态栏 | 16px 中文正文 | 底部留白 | 总高度 | 可见中文行 |
|---|---:|---:|---:|---:|---:|
| 256p 目标 | 24px | 224px | 8px | 256px | 14 行 |
| 224p 兼容布局 | 24px | 192px | 8px | 224px | 12 行 |
| 原版兼容 | 按现有 menu.rbf | 128px OSD 高度 | 硬件既定 | 保持原版 | 8 行 |

单个中文菜单行占 16px。256p 总高度提供 14 个完整正文行，并为状态栏和底部安全留白保留空间。

## 地址规划

- 256p 的 14 行中文字需要 28 个物理 8px 行，即 `28×256=7168` 字节位图正文。
- 224p 的 12 行中文字需要 24 个物理 8px 行，即 `24×256=6144` 字节位图正文。
- 顶部 24px 状态栏须占独立 768B 数据，预留 1024B 对齐区。状态栏不能再与扩大的正文混用旧的第 16–18 行。
- 原版 FPGA OSD `OSD_HEIGHT=64`，highres 翻倍至 128，正常位图 SRAM 为 4096B；`MENU_CORE` 另有顶部状态栏存储（见官方 `MiSTer-devel/Menu_MiSTer/sys/osd.v`）。
- 需要配套修改 FPGA OSD 读写地址/高度和新的状态栏命令；仅修改 ARM 主程序的物理行数**不会**扩大实际 OSD。
- 对游戏核心维持 128px 兼容模式，扩展只考虑具备新握手的实验主菜单 FPGA。

## CRT 可见区的重要限制

这里的 256/224 指 **目标像素坐标预算**，不是每台 CRT 实际都能完整显示的屏幕边界。过扫描会裁掉上/下缘；因此 8px 留白只是最低设计预算，不代表对所有 CRT 足够安全。未来需支持安全区偏移或按真机校正缩小菜单高度。

如果用户的视频模式是 384×224 或其他核心模式，需要选择匹配的布局/回退；256p 布局并不适合 224p 核心。

## 已完成与未完成

已完成：纯 Python 离线地址非重叠测试、256p 和 224p 尺寸预算；保留固定的 ZLabs 字体与原本可用中文版分支。

未完成：ARM 正式菜单 UI 的协议发送集成、ARM 与 FPGA capability 协商、Quartus 综合、真机菜单测试，**没有可以安装的新版 menu.rbf**。本 PR 继续保持草稿，不应安装或合并为正式版本。

## 2026-10-26：新增扫描路径原型（当时的 240p 设计）

独立模块 `cn_osd240_ram.v` 已实现 26 条物理行的正文 RAM 和 3 行的状态栏 RAM；`cn_osd240_pixel.v` 新增了同步 RAM 读取流水线，将当时的 240p 像素坐标映射为 24px 状态栏、208px 正文和 8px 底部留白；由 `tb_cn_osd240_pixel.v` 验证边界像素，`tb_cn_osd240_ram.v` 检查两区不互相覆盖。

- GitHub Actions： https://github.com/treeslins/Main_MiSTer/actions/runs/40317406954
- `iverilog -g2012` 和 `vvp` 的硬件逻辑仿真 **通过**。
- 当时尚未接入官方 `sys/osd.v` 及 `sys/sys_top.v`，也没有 Quartus 合成结果、`menu.rbf` 或 FPGA 与 ARM 新协议握手。离线像素映射通过不代表真实视频同步、任意视频模式或者 CRT 扫描一定正确；后续 source-level OSD 集成进展见本文末尾。
- 接入时应修改上游 Menu_MiSTer 工程，而不能把新 Verilog 文件误当成可以单独加载的 MiSTer 核心。

## 真实官方 SPI 协议接入前必须解决的差异

2026-10-26 复查官方 `MiSTer-devel/Menu_MiSTer/sys/osd.v`：真实 OSD 命令在 `io_osd` 片选有效后，将第一个 `io_strobe` 识别为 8 位命令，后续选通为同一事务的载荷；而目前 `cn_osd240_bridge.v` 实验模块使用独立 `command_valid`/`data_valid` 信号，并采用五个“命令”握手。这是抽象协议，**不能直接将其接在 `io_din` 上**，否则会破坏旧版命令解析。必须设计真实 SPI 事务适配器并在 HDL 仿真中交错测试原生命令、新命令、重启和中断事务。

另一个关键问题：官方 OSD 被 `sys/sys_top.v` 分别例化为 `hdmi_osd` 与 `vga_osd`，使用共同 `io_strobe`/`io_din` 但不同片选；扩展能力和模式状态必须与两路输出分别匹配。对于 VGA/CRT 还须验证 OSD 的缩放/扫描参数和 224p 安全区。

这一轮为实验桥增加了“命令有效时禁止误写入 RAM”的保护及测试；**尚未将其接入上游 OSD**。下一阶段先完成事务级指令设计及独立仿真，再进行视频顶层集成和 Quartus 综合。

## 真实 OSD SPI 事务适配原型（第六轮）

已增加 `cn_osd240_spi_adapter.v` 和 `tb_cn_osd240_spi_adapter.v`，仿真严格按照官方 `sys/osd.v` 的 `io_osd` 片选、`io_strobe` 上升沿、首字节指令及后续字节载荷来发包。扩展在默认情况下关闭，需要实验性 F0 + CN24 魔数字节序列；F1 关闭；F2 选择正文行，F3 选择状态栏行。仿真覆盖错误握手、不支持行号和关闭后的禁止写入。

**协议仍是私有实验设计。F0–F4 尚未从官方 OSD 协议中协商保留，不能与既有硬件命令直接混接；`legacy_passthrough` 目前只是状态提示，不是对上游命令解析的完整多路选择。新模块尚未接入 HPS ARM 正式菜单 UI 或验证 VGA/HDMI 顶层；也没有 FPGA capability discovery、CDC 或 Quartus inference 检查。**

最新成功仿真：[Actions #40319597561](https://github.com/treeslins/Main_MiSTer/actions/runs/40319597561)。

## 本分支 PR #4：256×256 隔离原型状态

本分支现在以 **256×256 可见画面预算**为目标：24px 独立顶部状态栏、224px 正文和 8px 底边；224p 兼容模式将正文缩为 192px。`test_memory_map.py` 检查 256p/224p 布局与银行地址预算。

当前最小隔离 HDL 由 `cn_osd240_spi_adapter.v`、`cn_osd240_dualclock_ram.v`、`cn_osd240_pixel.v` 和 `cn_osd240_vscan.v` 表达。扩展正文有独立 28×256B（7,168B）存储，状态栏有独立 3×256B（768B）存储；行号分别界限检查。SPI 扩展默认关闭，只有实验握手才启用；`F1` 关闭后恢复 `legacy_passthrough` 提示。现有测试覆盖银行隔离、非法行拒写和兼容回退。

在本节编写时，这些模块尚未连入官方 Menu_MiSTer `sys/osd.v` 的真实上游输出路径；后续 2026-10-27 集成进展见下文。`legacy_passthrough` 是回退状态标记，不是旧解析器的完整数据多路器；视频输出 mux 仅在扩展握手开启、官方 OSD enable 有效且 256×256 扩展 active 区内选择新像素。`F0`–`F4` 是未经官方保留/协商的实验命令，不能发送给未经修改的 OSD 固件。`apply_upstream_write_integration.py` 会对固定上游版本应用实验性视频读出和输出补丁。

独立 HDL 仿真与上游 `osd.v` Icarus elaboration 已在 2026-10-27 本地执行并通过。Quartus 综合、真实 sys_top 的双 HDMI/VGA 综合行为、时钟域可靠性、ARM 协议协商、视频波形/CRT 和硬件仍未验证。没有可安装固件，也不应把这个实验称为可安装版本。

## 2026-10-27：扩展视频输出接入及扫描事件修正

`apply_upstream_write_integration.py` 现在会把扩展 pixel/active 送进实际 `sys/osd.v` 输出混合器；只有扩展握手成功、官方 OSD enable 有效且坐标落在扩展 256×256 active 区内时选择扩展画面，其余区域继续使用原 OSD pixel 和 active gate。扫描计数器及 `ce_pix` / `DE` 事件已接到扩展读地址。此前行/帧事件在 `ce_pix` 之间可能保持为高并重复推进 Y；新增 `cn_osd240_scan_pulses.v` 把每个事件约束为一个 `clk_video` 周期，并新增仿真覆盖此点。

本轮在 `Menu_MiSTer` 上游提交 `fa83dc15b9545b58021f62738f0d69bba71ab187` 上实际生成改写源码；Icarus `-Wall` elaboration 通过（仅上游原有静态初始化警告）。本分支 Icarus 仿真覆盖 256p/224p 内存图、银行隔离和边界。Quartus workflow 已固定该上游提交，并在成功综合后保存 RBF、哈希及来源元数据；该 workflow 仍需要带 Quartus Standard 17 / Cyclone V 支持的自托管 runner，本环境没有 Quartus，因此**尚无实际 RBF 构建证据**。ARM 主程序发送器也仍未接入正式菜单 UI，不能称为可安装或完整可用固件。

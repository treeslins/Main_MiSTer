# MiSTer FPGA 中文 OSD · ZLabs 固定字库集成（开发预览）

本分支融合旧版 `chinese-osd-prototype` 的菜单中文化、滚动、选中视口和翻译资源与固定的 ZLabs RoundPix 16px 字库。当前约 602 条菜单翻译（以 localization/zh_CN.tsv 为准），不依赖 Oldman。

## 字库（不会在 CI 重新生成）

- **唯一运行时文件：** `cn/assets/misterfpga.cn`，314,376 字节，M16CN001。
- SHA-256：`cb8afcf32aabfc566ee0e39cd282c5d5a2c20049f2252dd26258696481c1a517`。
- 安装位置：MiSTer SD 卡根目录，对应 Linux 路径 `/media/fat/misterfpga.cn`。
- 引擎由 `zh_font.cpp` 读取原生 16x16 (中)、8x16 (ASCII) 列式点阵，转换位序。字库缺失/不合法时回退英文界面。
- 字型遵循 SIL OFL-1.1；生成的二进制随包带 `LICENSE-OFL`。除非明确升级字体，不会重新拉取或转换字库。

## 自动更新

`.github/workflows/mister-cn-integrated.yml` 每天北京时间 11:27 检查官方 `MiSTer-devel/Main_MiSTer/master`，在 **CI 临时工作树** 中尝试 Git merge，测试词库、字体、菜单，然后 ARM 10.2 交叉编译；冲突或测试失败就停止，旧成功产物不被覆盖。

每次成功的正式触发仅产生独立的 GitHub 预发布版；PR 验证不发布。不会推送合并到 master，也不会自动修改真实 MiSTer 的系统。遇到上游改动覆盖同一功能时仍需人工适配——并非保证所有未来更新都能零冲突自动合并。

## 菜单状态

- 保留旧项目译词、普通/反白/图标/滚动、8 行菜单视口、标题侧栏等。
- `MiSTer.ini` 的 `osd_language=1` 启用中文（当前分支默认）；`osd_language=0` 强制英文。
- 物理 CRT、HDMI、不同菜单核心、加载时机仍未完成真机验收；这是开发预览，不要覆盖唯一一份可启动的 MiSTer 程序。

建议先安装 `misterfpga.cn`，确认散列，再备份 `MiSTer` 主程序后测试。请不要把 `misterfpga.cn` 改名为 `oldmanfont.ex`。

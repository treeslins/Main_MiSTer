# MiSTerFPGA.cn 中文版

本项目基于 [MiSTer-devel/Main_MiSTer](https://github.com/MiSTer-devel/Main_MiSTer)，为 MiSTer 主程序加入简体中文 OSD、中文文件浏览和固定的 ZLabs RoundPix 字库。它仍使用原有 MiSTer 硬件与核心；中文化主要发生在主程序的菜单显示、文件列表和输入处理层。

**下载：[最新正式版](https://github.com/treeslins/Main_MiSTer/releases/latest)** · [构建记录](https://github.com/treeslins/Main_MiSTer/actions/workflows/mister-cn-integrated.yml) · [实现细节](cn/README.md) · [翻译与字库说明](localization/README.zh-CN.md)

## 与上游相比，改了什么

| 范围 | 中文版的改动 |
| --- | --- |
| 菜单文字 | 为主菜单、系统设置、其他选项及部分核心菜单提供约 602 条简体中文译词。翻译只改变显示文字，不改变核心配置值。 |
| 字体与排版 | 使用固定的 ZLabs RoundPix 12px 字库，在 16 像素菜单行内显示 12 像素宽中文和 6 像素宽西文；调整中英文、数字和标点的基线与间距。菜单侧栏标题按字竖排。 |
| 文件浏览 | 中文界面把上级目录和真实目录放在文件前；文件依次按数字、英文字母、汉字及其他符号开头分组。汉字按无声调拼音排序，L/R 快跳也支持拼音首字母。ZIP 虚拟目录仍按文件处理。 |
| 菜单操作 | 调整 8 行可视区域的选中项跟随、翻页、长文件名滚动、底部“退出”项，以及长路径和短核心名等边界情况。 |
| 信息显示 | 将网络地址、核心分辨率、菜单分辨率等信息改为更清楚的中文标签；中文界面的版本抬头显示 `MiSTerFPGA.cn`。 |
| 英文回退 | 可在 `MiSTer.ini` 中切回原英文界面；字库缺失或格式无效时，程序也会回退英文显示。 |

排序和界面调整只在中文模式启用，英文模式沿用原有文件排序。拼音采用单字默认读音，多音字不会按词组自动判音；字库未覆盖的汉字也可能无法正常显示。

## 下载与安装

1. 从[最新正式版](https://github.com/treeslins/Main_MiSTer/releases/latest)下载 **`MiSTer`** 主程序和 **`misterfpga.cn`** 字库。两个文件必须来自同一版发布。
2. 先备份 SD 卡根目录现有的 `MiSTer`，以便需要时恢复。
3. 将下载的 `MiSTer` 和 `misterfpga.cn` 放到 SD 卡根目录（MiSTer 系统中的 `/media/fat/`），然后重启。
4. 在 `MiSTer.ini` 的 `[MiSTer]` 段设置语言。发布包中的 `manifest.txt` 可用于核对文件 SHA-256。

```ini
[MiSTer]
osd_language=1
```

`1` 是简体中文（本项目默认值）；改为 `0` 并重启，即可使用英文界面。字库文件名必须保持为 `misterfpga.cn`，不要改名为旧字库文件名。若新程序出现启动或菜单问题，请恢复备份的 `MiSTer`。

本项目**不会自动更新 SD 卡上的程序**。GitHub 自动生成新版后，仍需要你自行下载和替换。每次新构建会经过主机测试和 ARM 交叉编译，但不能代替你在实际 MiSTer、显示设备和核心上的验证。

## 版本号与上游自动更新

仓库的 [GitHub Actions 工作流](.github/workflows/mister-cn-integrated.yml) 每天北京时间约 **11:27** 检查一次上游 `master`。它在临时工作区合入上游，校验固定字库、译词与菜单行为，再交叉编译 ARM 主程序。上游或中文版的程序产物有变化且全部检查通过时，才创建新的正式版；合并冲突或检查失败时，不会发布有问题的构建。

新发布的版本号使用**上游最新提交的 UTC 日期**，例如 `v2026.10.27`；同一天多次发布会增加 `.2`、`.3`。所以版本日期可能早于中文版的构建日期。每个发布包附有 `manifest.txt`（上游和中文源码提交号、散列）及 `upstream-merge.patch`（本次构建合入上游的改动）。自动构建不会把上游合并结果写回仓库分支，也不会更新实体设备。

## 来源、许可与开发资料

- MiSTer 主程序来自 [MiSTer-devel/Main_MiSTer](https://github.com/MiSTer-devel/Main_MiSTer)，项目许可见 [LICENSE](LICENSE)。本仓库保留了原有 MiSTer Floppy、SCP 和 IPF 支持的相关修改，详情见 [MiSTer Floppy](https://mister.robsmithdev.co.uk)。
- 中文字形由 [ZLabs RoundPix 12px Build_20260813](https://github.com/Astro-2539/ZLabs-RoundPix-12px/releases/tag/Build_20260813) 转换并固定在仓库中。字库许可为 SIL OFL 1.1，见 [LICENSE-OFL](cn/assets/LICENSE-OFL)。自动构建只校验并复制该字库，不会重新下载或生成它。
- 上游 [MiSTer Wiki](https://github.com/MiSTer-devel/Wiki_MiSTer/wiki) 与[交叉编译指南](https://mister-devel.github.io/MkDocs_MiSTer/developer/mistercompile/#general-prerequisites-for-arm-cross-compiling)适用于一般 MiSTer 使用与开发；中文版的具体实现和测试方法见 [cn/README.md](cn/README.md) 与 [localization/README.zh-CN.md](localization/README.zh-CN.md)。

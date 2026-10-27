# MiSTer FPGA 中文 OSD · ZLabs 固定字库集成（开发预览）

本分支融合旧版 `chinese-osd-prototype` 的菜单中文化、滚动、选中视口和翻译资源与固定的 ZLabs RoundPix 12px 字库。当前约 602 条菜单翻译（以 localization/zh_CN.tsv 为准），不依赖 Oldman。

## 字库（不会在 CI 重新生成）

- **唯一运行时文件：** `cn/assets/misterfpga.cn`，531,856 字节，M16CN004，13,296 个字形。
- SHA-256：`0f2857e3f2c559db4efdad381a0eb8384aa05810971ba9ab963535aa548b4ec9`。
- 安装位置：MiSTer SD 卡根目录，对应 Linux 路径 `/media/fat/misterfpga.cn`。
- 引擎由 `zh_font.cpp` 读取 12×16（中）及 6×16（ASCII）列式点阵。中文在 16 像素行内居中，英文、数字及短全角标点按基线与中文底部对齐。字库缺失/不合法时回退英文界面。
- 字库由 [ZLabs RoundPix 12px 的 Build_20260813 发布字体](https://github.com/Astro-2539/ZLabs-RoundPix-12px/releases/tag/Build_20260813)转换，原始 `ZLabsRoundPix_12px_M_CN.ttf` 的 SHA-256 为 `5adc3a83d11edc2e6f5bb3bc009dd9028dc055cbb3fc5f23715fb4de3c4e3eb0`。开发时可用 `cn/build_font12.py` 重现；CI 只校验并复制已提交的二进制。
- 字型遵循 SIL OFL-1.1；生成的二进制随包带 `LICENSE-OFL`。除非明确升级字体，不会重新拉取或转换字库。

## 自动更新

`.github/workflows/mister-cn-integrated.yml` 每天北京时间 11:27 检查官方 `MiSTer-devel/Main_MiSTer/master`，在 **CI 临时工作树** 中尝试 Git merge，测试词库、字体、菜单，然后 ARM 10.2 交叉编译；冲突或测试失败就停止，旧成功产物不被覆盖。

每次成功的正式触发仅产生独立的 GitHub 预发布版；PR 验证不发布。不会推送合并到 master，也不会自动修改真实 MiSTer 的系统。遇到上游改动覆盖同一功能时仍需人工适配——并非保证所有未来更新都能零冲突自动合并。

## 菜单状态

- 保留旧项目译词、普通/反白/图标/滚动、8 行菜单视口、标题侧栏等。
- 中文文件浏览器先列上级目录和真实目录；ZIP 虚拟目录按文件排序。文件依次按数字开头、英文字母开头、汉字开头排列。数字和英文按原名排序，汉字按无声调拼音排序；其他符号开头的文件排在其后。L/R 快跳也按目录或文件类别与首字母（汉字使用拼音首字母）分组。英文界面沿用原排序。拼音表由 `pypinyin==0.55.0`（MIT）在开发时生成，运行时无额外依赖。多音字采用单字默认读音，暂不按词组判音；字库以外的汉字按原 UTF-8 字节排序。
- `MiSTer.ini` 的 `osd_language=1` 启用中文（当前分支默认）；`osd_language=0` 强制英文。
- 物理 CRT、HDMI、不同菜单核心、加载时机仍未完成真机验收；这是开发预览，不要覆盖唯一一份可启动的 MiSTer 程序。

建议先安装 `misterfpga.cn`，确认散列，再备份 `MiSTer` 主程序后测试。请不要把 `misterfpga.cn` 改名为 `oldmanfont.ex`。

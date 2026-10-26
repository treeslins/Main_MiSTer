# misterfpga.cn · 自动跟随 MiSTer 上游（实验版）

本仓库由 treeslins/Main_MiSTer fork 承载；目标是让 ARM 主程序跟随官方上游更新，但始终使用固定且经过校验的中文字体，不再手工重复修改整份 osd.cpp。

## GitHub Actions 自动流程

- 每天 03:27 UTC 定时运行，也支持 Actions 页面手动触发。
- 检出 MiSTer-devel/Main_MiSTer 的最新 master。
- **不拉取、也不重新生成字体**：直接从本仓库 `cn/assets/misterfpga.cn` 复制固定字库；以 SHA-256、Unicode 数量与 GB2312 覆盖率检查其完整性。
- 由 cn/apply_overlay.py 对 osd.cpp 做锚点保护的源码改动，追加 cn_font.cpp/h。
- 使用官方文档指定的 ARM GNU 10.2 编译器交叉编译。
- 构建成功后，发布独立版本号的 GitHub Release 预发布版；附 MiSTer、misterfpga.cn、覆盖率、源码差异、校验表和字体许可证。
- 任何固定字体完整性检测、锚点检测、交叉编译失败，都会阻止自动发布，不会覆盖旧的成功产物。

## 重要：这还不是可投入日常使用的中文 MiSTer 主程序

目前已包含一个完整的 M16CN001 稀疏 Unicode 字库加载器和**实验性**双行 OSD 数据写入钩子。默认关闭，只有进程环境设置 MISTER_CN_EXPERIMENTAL=1 时才会尝试显示 UTF-8。实验路径仍存在菜单行号映射和普通行混排问题；尚未完成所有菜单翻译、CRT/HDMI 验证，不建议在真机开启。

日常默认编译行为尽量与上游保持一致。即使 GitHub 预发布构建成功，也不等于真机中文版已经验收通过。请不要覆盖 SD 卡原来的 MiSTer 而不做备份。

文件 misterfpga.cn 是独立的 M16CN001 格式，**不能直接改名为 oldmanfont.ex 替换**。没有复制 Oldman 管家、激活和字体文件。

## 授权与版本追踪

Main_MiSTer 依其原许可证；ZLabs RoundPix 字型依 SIL OFL 1.1，生成的字体数据继续遵守 OFL，分发时带 LICENSE-OFL；字体构建代码为本仓库补丁代码，不代替原作者版权声明。修改或再分发衍生字体时注意保留字体名称的限制。

每份发布都在 manifest.txt 记录官方主程序 SHA、**固定字体 SHA-256**、补丁 SHA，避免不知道打在了什么版本上。

## 启用

这个配置需要先合并到 fork 的默认 master，GitHub Actions 才会按计划运行。打开本仓库 Pull Request 审核合并，再到 Actions 页面选择 MiSTer CN - upstream auto patch and build → Run workflow 手动跑第一次。

发布需要允许 Actions 使用 contents:write 权限；若你的仓库限制更严格，在 Settings → Actions → General 的 Workflow permissions 中检查。此工作流**不**自动合并官方源码到你的 master、不自动改变本地 MiSTer 系统、不生成 Pull Request；它只在工作区应用补丁并发布成功构建的 GitHub 预览版。

## 本地复现

~~~bash
sha256sum -c cn/assets/SHA256SUMS
python3 cn/verify_font.py --input cn/assets/misterfpga.cn --report dist/coverage.json
cp cn/assets/misterfpga.cn dist/misterfpga.cn
python3 cn/apply_overlay.py Main_MiSTer
make -C Main_MiSTer
~~~

最后一步要求符合上游要求的 ARM 交叉工具链。补丁脚本针对一份未修改的官方源码树；不得在同一个工作树上重复执行。

## 固定字体更新政策

`cn/assets/misterfpga.cn` 是从已生成成品原样固定的二进制字库，大小 **314,376 字节**，SHA-256 为 `cb8afcf32aabfc566ee0e39cd282c5d5a2c20049f2252dd26258696481c1a517`。GitHub 每日构建**不会**访问 ZLabs 字体仓库，也不会运行 `cn/build_font.py`。后者仅为将来手动决定升级字库时保留的独立工具，不参与 CI。若固定文件被改动，构建会停止，不会悄悄用新版本覆盖。

# MiSTer FPGA 简体中文界面（固定 ZLabs 字库版）

## 当前实现

菜单中文化来自原 `chinese-osd-prototype` 分支，包含 602 条翻译、12×16 中文和 6×16 西文混排、8 行可视 OSD 菜单、选中项跟随、翻页标记和底部退出项，以及支持 UTF-8 的文件名滚动显示。中文翻译显示层不更改游戏核心的配置值。

本版本已经不使用旧 GNU Unifont 字库，也不会自动生成新字体。只读取固定的 ZLabs RoundPix 字库：

- GitHub 存放：`cn/assets/misterfpga.cn`
- 实机存放：`/media/fat/misterfpga.cn`（SD 卡根目录）
- 规格：M16CN004，13,296 个字形，531,856 字节
- SHA-256：`0f2857e3f2c559db4efdad381a0eb8384aa05810971ba9ab963535aa548b4ec9`
- 字型许可：SIL OFL 1.1（`cn/assets/LICENSE-OFL`）

加载器 `zh_font.cpp` 自动验证头、数据长度、Unicode 递增索引、有效码位、保留字段与字形宽度，并将列式位图从 MSB-first 转换为 MiSTer OSD 的 LSB-first。若字体缺失或格式无效，`OsdChinese()` 退回英文显示。运行时不校验整文件 SHA-256；固定字库的 SHA-256 与译词覆盖由构建检查验证。首次加载失败后，补回字库需要重启程序。

## 中英文切换

在 `MiSTer.ini` 中设置：

~~~ini
[MiSTer]
osd_language=1
~~~

`1` 启用中文，`0` 使用英文。当前开发分支的缺省值为 1。修改配置后重启。

## 自动跟随官方上游

`.github/workflows/mister-cn-integrated.yml` 配置了每天 03:27 UTC 的定时任务；该文件进入默认分支后，GitHub 才会启用定时运行。PR 阶段由匹配的改动或手动触发构建。它会检出当前中文工程，获取 `MiSTer-devel/Main_MiSTer/master` 并在 CI 临时工作区进行 Git 合并，再依次执行：

1. SHA-256 校验固定字体，**原样复制**至构建包，不下载也不重新生成；
2. 验证 602 条中文翻译词条的一致性；
3. 编译并运行主机端中文菜单渲染、UTF-8、滚动、选中项和位图位序测试；
4. 用官方 ARM GCC 10.2 交叉编译 MiSTer；
5. 只在全部通过后上传构建产物；定时或手动构建仅在上游提交或中文源码变化时建立 GitHub 正式版，版本号从上游最新提交的 UTC 日期生成。同日多次发布增加序号。

如果上游合并冲突、字体缺失或者任何测试失败，整个任务失败，不会覆盖已有的成功产物；也不会向默认分支自动写回合并结果或自动更新实体设备。

## 本地和真机验证

主机测试（在仓库根目录，需提供固定字体路径）：

~~~bash
export MISTER_CN_FONT_PATH=cn/assets/misterfpga.cn
g++ -std=gnu++14 -I. tests/zh_render_test.cpp zh_ui.cpp zh_font.cpp -o /tmp/zh-test
/tmp/zh-test /tmp/zh-preview.pgm
g++ -std=gnu++14 -I. tests/zlabs_loader_test.cpp zh_font.cpp -o /tmp/font-test
/tmp/font-test
~~~

ARM 交叉编译使用 `make`，需要 ARM GNU 10.2 工具链。构建结果能通过编译，并不等于已经完成 CRT/HDMI 真机验收。真实 MiSTer 测试前务必备份 SD 卡根目录的 `MiSTer`，把新编译的 `MiSTer` 和 `misterfpga.cn` 都放在根目录。如果出现启动、显示或菜单异常，立即恢复原版主程序。

中文菜单在 CRT 低分辨率、特殊核心 OSD、旋转以及不同视频模式下仍需确认。若 `menu.rbf` 的行为与旧版预期不同，还需要独立兼容性修正。

更多集成细节见 `cn/README.md`。

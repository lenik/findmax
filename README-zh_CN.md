# findmax

`findmax` 是一个面向“按指定条件找最大值文件”的快速查找工具，针对 O(1) 查询做了优化，并提供共享库 `libfindmax`。

## 仓库结构

- `src/` - 命令行工具、库源码与 bas 日志辅助
- `tests/` - 单元/集成测试源码
- `docs/` - AsciiDoc man 页源文件（`docs/*.adoc`）
- `debian/` - Debian 打包元数据
- `packaging/` - RPM 等其他打包文件
- `meson.build` - 顶层构建定义

## 功能

- 使用堆结构做快速最值查询
- 可按修改时间、访问时间、创建时间、大小或名称排序
- 支持递归目录遍历与深度限制
- 支持类似 `stat(1)` 的自定义输出格式
- 支持仅文件 / 仅目录过滤，以及符号链接解引用
- 提供 Bash 补全

## 构建与安装

```bash
sudo apt install meson ninja-build gcc pkg-config asciidoctor libbas-c-dev
meson setup build
ninja -C build
sudo ninja -C build install
```

## 用法

```bash
findmax [选项] [文件...]
```

常用选项：

- `-R`, `--recursive` — 递归目录
- `-r`, `--reverse` — 反向排序
- `-t` / `-u` / `-c` / `-S` / `-n` — 按 mtime / atime / ctime / 大小 / 名称排序
- `-f`, `--file-only` / `-d`, `--dir-only` — 仅文件或仅目录
- `-l NUM` — 返回前 NUM 个结果
- `-printf FORMAT` — 自定义输出格式

详见 `man findmax`。

## 许可证

AGPL-3.0-or-later，见 `LICENSE`。

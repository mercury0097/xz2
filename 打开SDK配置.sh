#!/bin/bash
# 命令行打开 SDK Configuration (menuconfig)

cd /Users/machenyang/Desktop/xz1-main

echo "🔧 正在打开 ESP-IDF SDK 配置工具..."
echo ""
echo "提示:"
echo "  - 使用 ↑↓←→ 方向键导航"
echo "  - 使用 Enter 键选择/进入菜单"
echo "  - 使用 Space 键启用/禁用选项"
echo "  - 使用 / 键搜索配置项"
echo "  - 使用 S 键保存并退出"
echo "  - 使用 Q 键退出(会询问是否保存)"
echo ""
echo "按 Enter 继续..."
read

source $HOME/Desktop/esp-5.51/v5.5.1/esp-idf/export.sh
idf.py menuconfig


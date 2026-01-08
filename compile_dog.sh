#!/bin/bash

# Dog板子编译脚本
# 使用方法: ./compile_dog.sh

set -e

echo "🐕 开始编译桌面小狗机器人..."

# 尝试不同的 ESP-IDF 路径
if [ -f "$HOME/.espressif/frameworks/esp-idf-v5.5/export.sh" ]; then
    echo "📦 找到 ESP-IDF v5.5"
    source "$HOME/.espressif/frameworks/esp-idf-v5.5/export.sh"
elif [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    echo "📦 找到 ESP-IDF (默认位置)"
    source "$HOME/esp/esp-idf/export.sh"
elif [ -f "/opt/esp-idf/export.sh" ]; then
    echo "📦 找到 ESP-IDF (/opt)"
    source "/opt/esp-idf/export.sh"
else
    echo "❌ 未找到 ESP-IDF，请手动设置环境"
    echo "   提示：运行 'source ~/esp/esp-idf/export.sh' 或类似命令"
    exit 1
fi

# 设置dog板子
echo "🔧 配置Dog板子..."
idf.py set-target esp32s3

# 注意：需要通过menuconfig手动选择板子类型为"Dog"
echo ""
echo "⚠️  重要：请在menuconfig中选择板子类型："
echo "   Board Configuration -> Board Type -> Dog (Desktop Quadruped Robot)"
echo ""
echo "是否要打开menuconfig配置界面？(y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    idf.py menuconfig
fi

# 编译
echo "🔨 开始编译..."
idf.py build

echo "✅ 编译完成！"
echo ""
echo "📝 下一步："
echo "   1. 烧录: idf.py flash"
echo "   2. 监控: idf.py monitor"
echo "   3. 或者: idf.py flash monitor"
echo ""
echo "🐕 MCP控制接口："
echo "   - self.dog.walk_forward    前进"
echo "   - self.dog.walk_backward   后退"
echo "   - self.dog.stop            停止"
echo "   - self.dog.home            回到休息姿态"
echo "   - self.dog.set_trim        设置舵机微调"
echo "   - self.dog.get_trims       获取舵机微调值"
echo "   - self.dog.get_status      获取状态"
echo ""





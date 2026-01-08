#!/bin/bash
# VSCode ESP-IDF 配置诊断脚本

echo "========================================"
echo "VSCode ESP-IDF 配置诊断"
echo "========================================"
echo ""

cd /Users/machenyang/Desktop/xz1-main

# 1. 检查ESP-IDF路径
echo "1️⃣ 检查 ESP-IDF 路径..."
IDF_PATH="$HOME/Desktop/esp-5.51/v5.5.1/esp-idf"
if [ -d "$IDF_PATH" ]; then
    echo "   ✅ ESP-IDF 路径存在: $IDF_PATH"
else
    echo "   ❌ ESP-IDF 路径不存在: $IDF_PATH"
fi
echo ""

# 2. 检查Python环境
echo "2️⃣ 检查 Python 环境..."
PYTHON_PATH="$HOME/.espressif/python_env/idf5.5_py3.12_env/bin/python"
if [ -f "$PYTHON_PATH" ]; then
    echo "   ✅ Python 环境存在: $PYTHON_PATH"
    $PYTHON_PATH --version
else
    echo "   ❌ Python 环境不存在: $PYTHON_PATH"
    echo "   💡 可用的Python环境:"
    ls -1 "$HOME/.espressif/python_env/" 2>/dev/null
fi
echo ""

# 3. 检查工具链
echo "3️⃣ 检查 ESP-IDF 工具链..."
if [ -d "$HOME/.espressif/tools" ]; then
    echo "   ✅ 工具链目录存在"
else
    echo "   ❌ 工具链目录不存在"
fi
echo ""

# 4. 检查sdkconfig文件
echo "4️⃣ 检查 sdkconfig 文件..."
if [ -f "sdkconfig" ]; then
    echo "   ✅ sdkconfig 文件存在"
    echo "   📊 文件大小: $(ls -lh sdkconfig | awk '{print $5}')"
    echo "   📅 最后修改: $(ls -l sdkconfig | awk '{print $6, $7, $8}')"
else
    echo "   ❌ sdkconfig 文件不存在"
fi
echo ""

# 5. 检查build目录
echo "5️⃣ 检查 build 目录..."
if [ -d "build" ]; then
    echo "   ✅ build 目录存在"
    if [ -f "build/project_description.json" ]; then
        echo "   ✅ 项目已配置"
    else
        echo "   ⚠️  项目可能需要重新配置"
    fi
else
    echo "   ⚠️  build 目录不存在(首次编译会自动创建)"
fi
echo ""

# 6. 测试menuconfig
echo "6️⃣ 测试 menuconfig 命令..."
source "$IDF_PATH/export.sh" > /dev/null 2>&1
if idf.py menuconfig --help > /dev/null 2>&1; then
    echo "   ✅ menuconfig 命令可用"
else
    echo "   ❌ menuconfig 命令不可用"
fi
echo ""

# 7. 检查VSCode配置
echo "7️⃣ 检查 VSCode 配置..."
if [ -f ".vscode/settings.json" ]; then
    echo "   ✅ VSCode settings.json 存在"
    echo "   📝 当前配置:"
    echo ""
    cat .vscode/settings.json
    echo ""
else
    echo "   ❌ VSCode settings.json 不存在"
fi

echo "========================================"
echo "诊断完成!"
echo "========================================"
echo ""
echo "💡 如果 SDK 配置还是打不开，请尝试:"
echo ""
echo "方案1: 在 VSCode 命令面板中 (Cmd+Shift+P):"
echo "   输入: ESP-IDF: SDK Configuration Editor"
echo "   或者: ESP-IDF: Launch GUI Configuration Tool"
echo ""
echo "方案2: 使用命令行打开 menuconfig:"
echo "   cd /Users/machenyang/Desktop/xz1-main"
echo "   source $HOME/Desktop/esp-5.51/v5.5.1/esp-idf/export.sh"
echo "   idf.py menuconfig"
echo ""
echo "方案3: 重新配置 ESP-IDF 扩展:"
echo "   1. 在 VSCode 中打开命令面板 (Cmd+Shift+P)"
echo "   2. 输入: ESP-IDF: Configure ESP-IDF Extension"
echo "   3. 选择 'Use Existing Setup'"
echo "   4. 选择 ESP-IDF 路径: $HOME/Desktop/esp-5.51/v5.5.1/esp-idf"
echo "   5. 选择 Tools 路径: $HOME/.espressif"
echo ""


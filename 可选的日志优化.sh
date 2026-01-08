#!/bin/bash
# 可选的日志级别优化(生产环境用)
# 只减少日志输出,对功能无影响
# 预计减少: ~100KB 左右

cd /Users/machenyang/Desktop/xz1-main

echo "=== 当前日志配置 ==="
grep "CONFIG_LOG_DEFAULT_LEVEL" sdkconfig | grep -v "^#"

echo ""
echo "=== 是否要将日志级别从 INFO 降低到 WARN? ==="
echo "影响: 减少约100KB大小,但INFO级别的日志(如'WiFi连接成功')将不再输出"
echo "建议: 开发时保持INFO,生产环境改为WARN"
echo ""
read -p "确认修改? (y/n): " confirm

if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
    # 备份当前配置
    cp sdkconfig sdkconfig.backup
    
    # 修改日志级别
    sed -i '' 's/CONFIG_LOG_DEFAULT_LEVEL_INFO=y/# CONFIG_LOG_DEFAULT_LEVEL_INFO is not set/' sdkconfig
    sed -i '' 's/# CONFIG_LOG_DEFAULT_LEVEL_WARN is not set/CONFIG_LOG_DEFAULT_LEVEL_WARN=y/' sdkconfig
    sed -i '' 's/CONFIG_LOG_DEFAULT_LEVEL=3/CONFIG_LOG_DEFAULT_LEVEL=2/' sdkconfig
    
    echo "✅ 已修改配置,备份保存在 sdkconfig.backup"
    echo "📝 现在运行 ./compile.sh 重新编译"
else
    echo "❌ 已取消修改"
fi


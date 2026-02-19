#!/bin/bash
PROJECT_NAME="tcp-turbo"
BUILD_DIR="build"
INSTALL_PATH="/usr/local/bin"
SERVICE_NAME="tcp_turbo.service"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
echo -e "${YELLOW}=== $PROJECT_NAME 自动化构建脚本 ===${NC}"
echo -e "\n${GREEN}[1/5] 检查依赖环境...${NC}"
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}错误: 未安装 cmake${NC}"
    exit 1
fi
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}错误: 未安装 g++ (支持 C++17)${NC}"
    exit 1
fi
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}清理旧的构建目录...${NC}"
    rm -rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"
echo -e "\n${GREEN}[2/5] 开始编译 (Release 模式)...${NC}"
cd "$BUILD_DIR" || exit
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake 配置失败${NC}"
    exit 1
fi
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo -e "${RED}编译出错${NC}"
    exit 1
fi
cd ..
echo -e "\n${GREEN}[3/5] 生成安装脚本...${NC}"
cat <<EOF > install.sh
#!/bin/bash
if [ "\$EUID" -ne 0 ]; then
  echo "请以 root 权限运行安装脚本 (sudo ./install.sh)"
  exit 1
fi
echo "正在安装 $PROJECT_NAME 到 $INSTALL_PATH..."
cp "$BUILD_DIR/$PROJECT_NAME" "$INSTALL_PATH/"
chmod +x "$INSTALL_PATH/$PROJECT_NAME"
if [ -f "$SERVICE_NAME" ]; then
    echo "正在注册 Systemd 服务..."
    cp "$SERVICE_NAME" /etc/systemd/system/
    systemctl daemon-reload
    echo "安装完成。使用 'systemctl start $PROJECT_NAME' 启动，或直接运行 '$PROJECT_NAME start'"
else
    echo "警告: 未找到服务文件 $SERVICE_NAME"
fi
EOF
chmod +x install.sh
echo -e "\n${GREEN}[4/5] 编译成功！${NC}"
echo -e "可执行文件位于: ${YELLOW}$BUILD_DIR/$PROJECT_NAME${NC}"
echo -e "你可以通过执行 ${YELLOW}sudo ./install.sh${NC} 将其安装到系统路径。"
echo -e "\n${GREEN}[5/5] 快速测试 (检查版本)...${NC}"
./$BUILD_DIR/$PROJECT_NAME | head -n 3
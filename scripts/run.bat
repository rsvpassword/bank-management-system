@echo off
setlocal

set PORT=%1
if "%PORT%"=="" set PORT=8888

echo [1/2] 正在编译...
g++ -O2 -std=c++17 bank_server.cpp -o bank_server.exe -lws2_32
if errorlevel 1 (
    echo 编译失败，请检查 g++ 是否已安装并加入 PATH
    pause
    exit /b 1
)

echo [2/2] 启动服务器，端口 %PORT% ...
bank_server.exe %PORT%

endlocal
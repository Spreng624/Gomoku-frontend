# 1. 设置 Qt 环境变量 (根据你的路径)
$QtPath = "D:/Dev/QT/6.10.1/mingw_64/bin;D:/Dev/QT/Tools/mingw1310_64/bin"
$env:PATH = "$QtPath;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "D:/Dev/QT/6.10.1/mingw_64/plugins/platforms"

# 2. 启动两个客户端 (& 表示后台运行，不阻塞终端)
Write-Host "正在启动双客户端..." -ForegroundColor Green
start-process "./build/debug/Gomoku-frontend.exe"
start-process "./build/debug/Gomoku-frontend.exe"
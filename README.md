# 银行管理系统（Bank Management System）

一个用 C++ 实现的银行账户管理系统，包含**单机控制台版**和**多线程 TCP 服务器版**，附带 Python 客户端。

## 功能特性

- 管理员注册 / 登录
- 银行卡开户 / 销户 / 激活
- 存款、取款、汇款
- 借记卡 / 信用卡（含授信额度、欠款、还款）
- 卡冻结 / 解冻
- 密码修改 / 忘记密码重置
- 卡号符合 Luhn 校验（银联 / VISA / MasterCard 号段）
- 数据持久化到 `bank.in`
- 操作日志写入 `log.txt`（UTC 时间）
- 多客户端并发访问（互斥锁保护共享数据）

## 目录结构

```
├── src/
│   ├── bank.cpp            # 原始单机控制台版
│   └── bank_server.cpp     # 多线程 TCP 服务器版
├── client/
│   └── bank_climb.py       # Python 客户端
├── scripts/
│   ├── run.bat
│   └── run.sh
└── docs/
    └── protocol.md
```

## 快速开始

### 编译并启动服务器

**Windows（MinGW）：**
```bat
g++ -O2 -std=c++17 src/bank_server.cpp -o bank_server.exe -lws2_32
bank_server.exe 8888
```

**Linux / macOS：**
```bash
g++ -O2 -std=c++17 -pthread src/bank_server.cpp -o bank_server
./bank_server 8888
```

服务器默认监听 `8888` 端口，数据保存在 `bank.in`，日志写入 `log.txt`。

### 运行客户端

```bash
python client/bank_climb.py
```

然后按提示输入命令即可。

## 通信协议

详见 [`docs/protocol.md`](docs/protocol.md)。

简要说明：
- 请求：一行文本，字段用 `|` 分隔，以 `\n` 结束
- 响应：多行文本，以单独一行 `###END###` 结束
- 首行 `OK|...` 表示成功，`ERR|...` 表示失败

## 常用命令示例

```
PING
REGADMIN|测试银行|admin|123456
LOGIN|1|123456
OPEN|1|1|110101199001011234|ZhangSan|123456
INFO|卡号|密码
DEPOSIT|卡号|密码|1000
TRANSFER|付款卡号|付款密码|收款卡号|收款密码|500
QUIT
```

## 构建脚本

- Windows：双击 `scripts/run.bat`
- Linux / macOS：`chmod +x scripts/run.sh && ./scripts/run.sh`

## 开源协议

本项目采用 [MIT License](LICENSE)。

## 免责声明

本项目仅供学习和研究使用，**不可用于真实金融场景**。代码中的银行卡号、CVV、账户等均为模拟数据，没有任何实际金融效力。
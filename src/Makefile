# SRC=./src
# TARGET=.
# GCC = g++
# CFLAGS = -o $(TARGET)/PiRecorder.out
# ARGS = 101 -lpthread -lbcm2835 -lSSD1306_OLED_RPI -lasound
# RM = rm -rf

# all: clean default test

# default: main.cpp

# CoinFlipper.cpp:
# 	$(GCC) $(CFLAGS) $(SRC)/main.cpp

# test: main.cpp
# 	$(TARGET)/PiRecorder.out $(ARGS)

# clean:
# 	$(RM) $(TARGET)/*.out

CC := g++
CFLAGS := -Wall -g -lpthread -lbcm2835 -lSSD1306_OLED_RPI -lasound -lstdc++fs
TARGET := PiRecorder
BUILD_FOLDER := ./build

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	cd $(BUILD_FOLDER) && \
	 $(CC) -o $@ $^ $(CFLAGS)
%.o: %.cpp
	@mkdir -p $(BUILD_FOLDER)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_FOLDER)/$@
clean:
	rm -rf $(TARGET) $(BUILD_FOLDER)/*.o
	
.PHONY: all clean
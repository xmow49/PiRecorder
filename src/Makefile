# CC := g++
# CFLAGS := -Wall -g -lpthread -lbcm2835 -lSSD1306_OLED_RPI -lasound -lstdc++fs
# TARGET := PiRecorder
# BUILD_FOLDER := ./build

# # $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
# SRCS := $(wildcard *.cpp)
# # $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
# OBJS := $(patsubst %.cpp,%.o,$(SRCS))

# all: $(TARGET)
# $(TARGET): $(OBJS)
# 	cd $(BUILD_FOLDER) && \
# 	 $(CC) -o $@ $^ $(CFLAGS)
# %.o: %.cpp
# 	@mkdir -p $(BUILD_FOLDER)
# 	$(CC) $(CFLAGS) -c $< -o $(BUILD_FOLDER)/$@
# clean:
# 	rm -rf $(TARGET) $(BUILD_FOLDER)/*.o
	
# .PHONY: all clean



# -----------------------------------------
CC := g++
CFLAGS := -Wall -g -lpthread -lbcm2835 -lSSD1306_OLED_RPI -lasound -lstdc++fs
TARGET := PiRecorder

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<
clean:
	rm -rf $(TARGET) *.o
	
.PHONY: all clean
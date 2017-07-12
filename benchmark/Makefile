SHELL        = /bin/sh

SRC_FILE_DIR = src
OBJ_DIR      = obj
BIN_DIR      = .

SRC_DIRS     = $(shell find $(SRC_FILE_DIR) -depth -type d)
SOURCES      = $(foreach d, $(SRC_DIRS), $(wildcard $(d)/*.cpp) )
OBJS         = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

CC           = cc
CXX          = g++

CPPFLAGS  = -std=c++11 -O2 -c -Wall -fmessage-length=0
CFLAGS    = -std=c++11

LIBS      = -lpthread ../libeasynet.a
INCLUDE   = -Ipthread -I../
LIBPATH   =

SERVER    = $(BIN_DIR)/server
CLIENT    = $(BIN_DIR)/client

TARGET    = $(SERVER) $(CLIENT)

DEPENDENCY  = $(OBJS:%.o=%.d)

all: $(TARGET)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCY)
endif

$(DEPENDENCY):$(OBJ_DIR)/%.d:%.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo 'Creating dependence: $<'
	@set -e; rm -f $@;\
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($(basename $(notdir $@))\)\.o[:]*,$(addsuffix .o, $(basename $@)) $@ :,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

$(OBJS):$(OBJ_DIR)/%.o:%.cpp 
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CXX) $(INCLUDE) $(CPPFLAGS) $< -o $@
	@echo 'Finished building: $<'
	@echo ' '

$(SERVER):$(OBJ_DIR)/$(SRC_FILE_DIR)/server.o
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(CLIENT):$(OBJ_DIR)/$(SRC_FILE_DIR)/client.o
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(TARGET)

.PHONY: all clean

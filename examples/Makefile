SHELL        = /bin/sh

SRC_FILE_DIR = .
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

ECHO_SERVER    = $(BIN_DIR)/echo-server
ECHO_CLIENT    = $(BIN_DIR)/echo-client

FILE_SERVER    = $(BIN_DIR)/file-server
FILE_CLIENT    = $(BIN_DIR)/file-client

UDP_SERVER    = $(BIN_DIR)/udp-server
UDP_CLIENT    = $(BIN_DIR)/udp-client

TARGET    = $(ECHO_SERVER) $(ECHO_CLIENT) $(FILE_SERVER) $(FILE_CLIENT) $(UDP_SERVER) $(UDP_CLIENT) 

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
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CXX) $(INCLUDE) $(CPPFLAGS) $< -o $@
	@echo 'Finished building: $<'
	@echo ' '

$(ECHO_SERVER):$(OBJ_DIR)/$(SRC_FILE_DIR)/echo-server.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(ECHO_CLIENT):$(OBJ_DIR)/$(SRC_FILE_DIR)/echo-client.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(FILE_SERVER):$(OBJ_DIR)/$(SRC_FILE_DIR)/file-server.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(FILE_CLIENT):$(OBJ_DIR)/$(SRC_FILE_DIR)/file-client.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(UDP_SERVER):$(OBJ_DIR)/$(SRC_FILE_DIR)/udp-server.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

$(UDP_CLIENT):$(OBJ_DIR)/$(SRC_FILE_DIR)/udp-client.o $(LIBS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $< $(LIBS) -o $@
	@echo 'Finished building target: $@'
	@echo ' '

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(TARGET)

.PHONY: all clean

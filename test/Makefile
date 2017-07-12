SHELL        = /bin/sh

# src dir we want to test...
TO_BE_TEST_SRC_DIR      = ../easynet
  
# find dir and sub-dir in @TO_BE_TEST_SRC_DIR
SRC_DIR_ALL  = $(shell find $(TO_BE_TEST_SRC_DIR) -depth -type d)

# test harness file dir
TEST_SRC_DIR = .

OBJ_DIR      = obj
BIN_DIR      = .

SOURCES      = $(foreach d, $(SRC_DIR_ALL), $(wildcard $(d)/*.cpp) )
TEST_SOURCES = $(foreach d, $(TEST_SRC_DIR), $(wildcard $(d)/*.cpp) )

OBJS_ALL = $(patsubst ../%.cpp, $(OBJ_DIR)/%.o, $(filter ../%.cpp, $(SOURCES)))
SRC_MAIN = $(OBJ_DIR)/easynet/main.o
OBJS  = $(filter-out $(SRC_MAIN), $(OBJS_ALL))

TEST_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(TEST_SOURCES))

CC           = cc
CXX          = g++

CPPFLAGS  = -std=c++11 -O2 -c -Wall -fmessage-length=0
CFLAGS    = -std=c++11

LIBS      = -lpthread
INCLUDE   = -Ipthread -I$(TO_BE_TEST_SRC_DIR) -I$(TEST_SRC_DIR)
LIBPATH   = 
TARGET    = $(BIN_DIR)/easynet-unitest

DEPENDENCY   = $(OBJS:%.o=%.d)
TEST_DEP     = $(TEST_OBJS:%.o=%.d)
 
all: $(TARGET)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCY)
-include $(TEST_DEP)
endif

$(DEPENDENCY):$(OBJ_DIR)/%.d : ../%.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo 'Creating dependence: $< to $@'
	@set -e; rm -f $@;\
	$(CC) -MM $(INCLUDE) $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($(basename $(notdir $@))\)\.o[:]*,$(addsuffix .o, $(basename $@)) $@ :,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

$(TEST_DEP):$(OBJ_DIR)/%.d : %.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo 'Creating dependence: $< $@'
	@set -e; rm -f $@;\
	$(CC) -MM $(INCLUDE) $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($(basename $(notdir $@))\)\.o[:]*,$(addsuffix .o, $(basename $@)) $@ :,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

$(OBJS):$(OBJ_DIR)/%.o:../%.cpp 
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CXX) $(INCLUDE) $(CPPFLAGS) $< -o $@
	@echo 'Finished building: $<'
	@echo ' '

$(TEST_OBJS):$(OBJ_DIR)/%.o:%.cpp 
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(CXX) $(INCLUDE) $(CPPFLAGS) $< -o $@
	@echo 'Finished building: $<'
	@echo ' '

$(TARGET):$(OBJS) $(TEST_OBJS)
	@echo 'Building target:$@'
	@echo 'Invoking: GCC C++ Linker'
	$(CXX) $(LIBPATH) $(OBJS) $(TEST_OBJS) $(LIBS) -o $(TARGET)
	@echo 'Finished building target: $@'
	@echo ' '

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(TARGET)

.PHONY: all clean

SHELL        = /bin/sh

SRC_FILE_DIR = easynet
OBJ_DIR      = obj
BIN_DIR      = bin

SRC_DIRS     = $(shell find $(SRC_FILE_DIR) -depth -type d)
SOURCES      = $(foreach d, $(SRC_DIRS), $(wildcard $(d)/*.cpp) )
OBJS         = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

INSTALL_INCLUDE_PATH = /usr/local/include
INSTALL_LIB_PATH = /usr/local/lib

CC           = cc
CXX          = g++

CPPFLAGS  = -std=c++11 -O2 -c -Wall -fmessage-length=0
CFLAGS    = -std=c++11

LIBS      = -lpthread
INCLUDE   = -Ipthread

LIBRARY = libeasynet.a  
DEPENDENCY  = $(OBJS:%.o=%.d)

all: $(LIBRARY)

ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDENCY)
endif

ifeq ($(MAKECMDGOALS),)
-include $(DEPENDENCY)
endif

ifdef prefix
INSTALL_INCLUDE_PATH=$(prefix)
INSTALL_LIB_PATH=$(prefix)
endif

$(LIBRARY):$(OBJS) 
	-rm -f $@
	$(AR) -rs $@  $(OBJS)

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

install:makedir
	$(foreach a, $(SRC_DIRS), $(shell cp -fp $(a)/*.h $(INSTALL_INCLUDE_PATH)/$(a)/))
	cp -fp $(LIBRARY) $(INSTALL_LIB_PATH)

uninstall:
	-rm -rf $(INSTALL_INCLUDE_PATH)/easynet
	-rm -rf $(INSTALL_LIB_PATH)/$(LIBRARY)

makedir:
	mkdir -p $(INSTALL_INCLUDE_PATH)/$(SRC_DIRS)

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(LIBRARY)

.PHONY: all clean install uninstall
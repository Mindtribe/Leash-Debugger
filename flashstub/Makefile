#SDK stuff
SDK_DIR=./../cc3200-sdk

#output folder
EXE := ./exe

#project parameters
program_NAME := testapp
program_C_SRCS := $(wildcard ./src/*.c) $(wildcard ./src/vendor/*.c)
program_CXX_SRCS := $(wildcard ./src/*.cpp) $(wildcard ./src/vendor/*.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
program_INCLUDE_DIRS := ./src ./src/vendor $(SDK_DIR)/inc $(SDK_DIR)/driverlib $(SDK_DIR)
program_LIBRARY_DIRS := ./lib ./lib/vendor $(SDK_DIR)/driverlib/gcc/exe
program_LIBRARIES :=
program_STATIC_LIBS := $(SDK_DIR)/driverlib/gcc/exe/libdriver.a

#map file
MAPFILE := ./$(program_NAME).map

#compiler and linker
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

#flags
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir)) \
	-mthumb -mcpu=cortex-m4 -ffunction-sections -fdata-sections -MD -std=gnu99 -g -Dgcc -O0 -c -Wall -Werror -Wextra -pedantic-errors
#LDFLAGS += --specs=nosys.specs
LDFLAGS += --gc-sections
LDFLAGS += -Map=$(MAPFILE)
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
LDFLAGS += $(program_STATIC_LIBS)

#linker script
LDSCRIPT=linker.ld

#base directory
BASE_DIR = $(shell pwd)

#include dependencies
-include $(program_OBJS:.o=.d)

#rules
.PHONY: all clean distclean dirmake driverlib

all: dirmake driverlib $(program_NAME)

dirmake: 
	mkdir -p exe

driverlib:
	cd $(SDK_DIR)/driverlib/gcc && make

$(program_NAME): $(program_OBJS)
	$(LD) $(program_OBJS) $(LDFLAGS) -T $(LDSCRIPT) -o $(EXE)/$(program_NAME).axf
	$(OBJCOPY) -O binary $(EXE)/$(program_NAME).axf $(EXE)/$(program_NAME).bin

clean:
	@- $(RM) $(EXE)/$(program_NAME).axf
	@- $(RM) $(EXE)/$(program_NAME).bin
	@- $(RM) -rf $(EXE)
	@- $(RM) $(MAPFILE)
	@- $(RM) $(program_OBJS)

distclean: clean

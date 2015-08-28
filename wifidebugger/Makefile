#SDK stuff
SDK_DIR=./../cc3200-sdk

#output folder
EXE := ./exe

#src folder
SRC := ./src

#inc folder
INC := ./inc

#object folder
OBJDIR := ./obj

#project parameters
program_NAME := wifidebugger
program_SOURCE_DIRS := $(SRC) $(SRC)/vendor $(SRC)/target $(SRC)/target/cc3200 $(SRC)/interface/jtag $(SRC)/gdbserver $(SRC)/thirdparty $(SRC)/common
program_INCLUDE_DIRS := $(program_SOURCE_DIRS) $(SDK_DIR)/inc $(SDK_DIR)/driverlib $(SDK_DIR)
program_LIBRARY_DIRS := ./lib ./lib/vendor $(SDK_DIR)/driverlib/gcc/exe
program_C_SRCS := $(foreach dir, $(program_SOURCE_DIRS), $(wildcard $(dir)/*.c))
program_CXX_SRCS := $(foreach dir, $(program_SOURCE_DIRS), $(wildcard $(dir)/*.cpp))
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(addprefix $(OBJDIR)/, $(notdir $(program_C_OBJS) $(program_CXX_OBJS)))
program_LIBRARIES :=
program_STATIC_LIBS := $(SDK_DIR)/driverlib/gcc/exe/libdriver.a

#VPATH
VPATH := $(program_SOURCE_DIRS)

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

#rules
.PHONY: all clean distclean dirmake driverlib

all: dirmake driverlib $(program_NAME)

dirmake: 
	mkdir -p $(EXE)
	mkdir -p $(OBJDIR)

driverlib:
	cd $(SDK_DIR)/driverlib/gcc && make

$(OBJDIR)/%.o: %.c
	$(CC) $(CPPFLAGS) -c $< -o $(addprefix $(OBJDIR)/, $(notdir $@))

$(program_NAME): $(program_OBJS)
	$(LD) $(program_OBJS) $(LDFLAGS) -T $(LDSCRIPT) -o $(EXE)/$(program_NAME).axf
	$(OBJCOPY) -O binary $(EXE)/$(program_NAME).axf $(EXE)/$(program_NAME).bin

clean:
	@- $(RM) $(EXE)/$(program_NAME).axf
	@- $(RM) $(EXE)/$(program_NAME).bin
	@- $(RM) -rf $(EXE)
	@- $(RM) $(MAPFILE)
	@- $(RM) $(addprefix $(OBJDIR)/, $(notdir $(program_OBJS)))
	@- $(RM) $(addprefix $(OBJDIR)/, $(notdir $(program_OBJS:.o=.d)))

distclean: clean

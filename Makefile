#SDK stuff
SDK_DIR=/home/parallels/CC3200_SDK/cc3200-sdk

#output folder
EXE := ./exe

#project parameters
program_NAME := WirelessDebugger
program_C_SRCS := $(wildcard ./src/*.c)
program_CXX_SRCS := $(wildcard ./src/*.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
program_INCLUDE_DIRS := ./inc $(SDK_DIR)/inc $(SDK_DIR)/driverlib $(SDK_DIR)
program_LIBRARY_DIRS := ./lib $(SDK_DIR)/driverlib/gcc/exe
program_LIBRARIES :=
program_STATIC_LIBS := $(SDK_DIR)/driverlib/gcc/exe/libdriver.a

#compiler and linker
CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld

#flags
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir)) \
	-mthumb -mcpu=cortex-m4 -ffunction-sections -fdata-sections -MD -std=c99 -g -Dgcc -O0 -c
#LDFLAGS += --specs=nosys.specs
LDFLAGS += --gc-sections
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
LDFLAGS += $(program_STATIC_LIBS)

#linker script
LDSCRIPT=linker.ld

#rules
.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(LD) $(program_OBJS) $(LDFLAGS) -T $(LDSCRIPT) -o $(EXE)/$(program_NAME).axf

clean:
	@- $(RM) $(EXE)/$(program_NAME).axf
	@- $(RM) $(program_OBJS)

distclean: clean

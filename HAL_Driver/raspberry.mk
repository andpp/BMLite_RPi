# Binary sources

NHAL = $(DEPTH)../HAL_Driver

VPATH += $(NHAL)

C_INC += -I$(NHAL)/inc

LDFLAGS += -lwiringPi -L$(NHAL)/lib/ 

# Source Folders
VPATH += $(NHAL)/src/

# C Sources
C_SRCS += $(notdir $(wildcard $(NHAL)/src/*.c))
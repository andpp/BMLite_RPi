# Binary sources
VPATH += $(DEPTH)../HAL_Driver

C_INC += -I$(DEPTH)../HAL_Driver/inc

LDFLAGS += -lwiringPi -L$(DEPTH)../HAL_Driver/lib/ 

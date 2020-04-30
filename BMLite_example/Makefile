# Copyright (c) 2020 Fingerprint Cards AB
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#   https://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Make sure that 'all' target become default target
.DEFAULT_GOAL := all

PRODUCT := bmlite_demo

PATH := /work/devtools/gcc-arm-hf/bin:$(PATH)

CC := arm-linux-gnueabihf-gcc

# Setup paths
OUT := out
DEPTH := 
HCP_PATH := ../hcp
RPIHAL_PATH := ../HAL_Driver

# Main target
TARGET := $(OUT)/$(PRODUCT)

# Common flags
CFLAGS +=\
	-std=c99\
	-D_DEFAULT_SOURCE \
	-g3\
	-Og\
	-Wall\
	-Werror\
	-fdata-sections\
	-ffunction-sections\
	-MMD\
	-MP\
	-Wno-unused-result

# C source files
C_SRCS = $(wildcard src/*.c)

# Include directories
PATH_INC += inc

C_INC = $(addprefix -I,$(PATH_INC))

# Include HAL driver
include $(RPIHAL_PATH)/raspberry.mk

# Object files and search paths
VPATH += $(sort $(dir $(C_SRCS)))
OBJECTS = $(patsubst %.c,$(OUT)/obj/%.o,$(notdir $(C_SRCS)))

# Dependency files
DEP := $(OBJECTS:.o=.d)
DEP_CFLAGS=$(OUT)/dep_cflags.txt

all: $(TARGET)

# Create binary from object files and external libraries
$(TARGET): $(OBJECTS) $(DEP_CFLAGS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(C_INC) $(OBJECTS) $(LDFLAGS) -o $@

# Compile source files
$(OUT)/obj/%.o: %.c $(DEP_CFLAGS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_INC) -o $@ -c $<

# Detect changes in CFLAGS
$(DEP_CFLAGS): force
	@mkdir -p $(dir $@)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@

-include $(DEP)

# Empty rule for dep files, they will be created when compiling
%.d: ;

clean:
	rm -rf $(OUT)

.PHONY: clean force

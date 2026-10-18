CUDA_HOME ?= /usr/local/cuda
NVCC      := $(CUDA_HOME)/bin/nvcc
ARCH      := -arch=sm_75
STD       := -std=c++17
INCLUDES  := -I$(CUDA_HOME)/include -Iinclude

BUILD_DIR := build
SRCS      := $(wildcard src/*.cu)
TARGET    := $(BUILD_DIR)/hello

.PHONY: all debug run clean list compdb

all: compdb $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(NVCC) $(STD) $(ARCH) -O2 $(INCLUDES) $(SRCS) -o $@

debug: $(SRCS) | $(BUILD_DIR)
	$(NVCC) $(STD) $(ARCH) -g -G -O0 $(INCLUDES) $(SRCS) -o $(TARGET)

run: all
	LD_LIBRARY_PATH=$(CUDA_HOME)/lib64:$$LD_LIBRARY_PATH ./$(TARGET)

compdb:
	python3 scripts/gen_clangd_cdb.py

list:
	@ls -1 src/*.cu 2>/dev/null || echo "src/ 下没有 .cu 文件"

clean:
	rm -rf $(BUILD_DIR)

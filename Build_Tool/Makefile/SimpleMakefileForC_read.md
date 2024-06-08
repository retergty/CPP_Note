# 简单的Makefile阅读文档，可以用于C语言的编译

本文讲述了一个简单的Makefile的工程实例，可以用于编译C语言。

```Makefile
#### PROJECT SETTINGS ####
# The name of the executable to be created
BIN_NAME := hello
# Compiler used
CC ?= gcc
# Extension of source files used in the project
SRC_EXT = c
# Path to the source directory, relative to the makefile
SRC_PATH = .
# Remove / in dierectory
SRC_PATH := $(patsubst %/,%,$(SRC_PATH))
# Space-separated pkg-config libraries used by this project
LIBS =
# General compiler flags
COMPILE_FLAGS = -std=c99 -Wall -Wextra -g
# Additional release-specific flags
RCOMPILE_FLAGS = -D NDEBUG
# Additional debug-specific flags
DCOMPILE_FLAGS = -D DEBUG
# Add additional include paths
INCLUDES = -I $(SRC_PATH)
# General linker settings
LINK_FLAGS =
# Additional release-specific linker settings
RLINK_FLAGS =
# Additional debug-specific linker settings
DLINK_FLAGS =
# Destination directory, like a jail or mounted system
DESTDIR = /
# Install path (bin/ is appended automatically)
INSTALL_PREFIX = usr/local
#### END PROJECT SETTINGS ####

# Optionally you may move the section above to a separate config.mk file, and
# uncomment the line below
# include config.mk

# Generally should not need to edit below this line

# Obtains the OS type, either 'Darwin' (OS X) or 'Linux'
UNAME_S:=$(shell uname -s)

# Function used to check variables. Use on the command line:
# make print-VARNAME
# Useful for debugging and adding features
print-%: ; @echo $*=$($*)

# Shell used in this makefile
# bash is used for 'echo -en'
SHELL = /bin/bash
# Clear built-in rules
.SUFFIXES:
# Programs for installation
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

# Append pkg-config specific libraries if need be
ifneq ($(LIBS),)
	COMPILE_FLAGS += $(shell pkg-config --cflags $(LIBS))
	LINK_FLAGS += $(shell pkg-config --libs $(LIBS))
endif

# Verbose option, to output compile and link commands
export V := false
export CMD_PREFIX := @
ifeq ($(V),true)
	CMD_PREFIX :=
endif

# Combine compiler and linker flags
release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

# Build and output paths
release: export BUILD_PATH := build/release
release: export BIN_PATH := bin/release
debug: export BUILD_PATH := build/debug
debug: export BIN_PATH := bin/debug
install: export BIN_PATH := bin/release

# Find all source files in the source directory, sorted by most
# recently modified
ifeq ($(UNAME_S),Darwin)
	SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
else
	SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' -printf '%T@\t%p\n' \
						| sort -k 1nr | cut -f2-)
endif

# fallback in case the above fails
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $(subst *,%,$2), $d))
ifeq ($(SOURCES),)
	SRC_PATH := $(patsubst %,%/,$(SRC_PATH))
	SOURCES := $(call rwildcard, $(SRC_PATH), *.$(SRC_EXT))
endif

# Set the object file names, with the source directory stripped
# from the path, and the build path prepended in its place
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
# Set the dependency files that will be used to add header dependencies
DEPS = $(OBJECTS:.o=.d)

# Macros for timing compilation
ifeq ($(UNAME_S),Darwin)
	CUR_TIME = awk 'BEGIN{srand(); print srand()}'
	TIME_FILE = $(dir $@).$(notdir $@)_time
	START_TIME = $(CUR_TIME) > $(TIME_FILE)
	END_TIME = read st < $(TIME_FILE) ; \
		$(RM) $(TIME_FILE) ; \
		st=$$((`$(CUR_TIME)` - $$st)) ; \
		echo $$st
else
	TIME_FILE = $(dir $@).$(notdir $@)_time
	START_TIME = date '+%s' > $(TIME_FILE)
	END_TIME = read st < $(TIME_FILE) ; \
		$(RM) $(TIME_FILE) ; \
		st=$$((`date '+%s'` - $$st - 86400)) ; \
		echo `date -u -d @$$st '+%H:%M:%S'`
endif

# Version macros
# Comment/remove this section to remove versioning
USE_VERSION := false
# If this isn't a git repo or the repo has no tags, git describe will return non-zero
ifeq ($(shell git describe > /dev/null 2>&1 ; echo $$?), 0)
	USE_VERSION := true
	VERSION := $(shell git describe --tags --long --dirty --always | \
		sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)-\?.*-\([0-9]*\)-\(.*\)/\1 \2 \3 \4 \5/g')
	VERSION_MAJOR := $(word 1, $(VERSION))
	VERSION_MINOR := $(word 2, $(VERSION))
	VERSION_PATCH := $(word 3, $(VERSION))
	VERSION_REVISION := $(word 4, $(VERSION))
	VERSION_HASH := $(word 5, $(VERSION))
	VERSION_STRING := \
		"$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_REVISION)-$(VERSION_HASH)"
	override CFLAGS := $(CFLAGS) \
		-D VERSION_MAJOR=$(VERSION_MAJOR) \
		-D VERSION_MINOR=$(VERSION_MINOR) \
		-D VERSION_PATCH=$(VERSION_PATCH) \
		-D VERSION_REVISION=$(VERSION_REVISION) \
		-D VERSION_HASH=\"$(VERSION_HASH)\"
endif

# Standard, non-optimized release build
.PHONY: release
release: dirs
ifeq ($(USE_VERSION), true)
	@echo "Beginning release build v$(VERSION_STRING)"
else
	@echo "Beginning release build"
endif
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

# Debug build for gdb debugging
.PHONY: debug
debug: dirs
ifeq ($(USE_VERSION), true)
	@echo "Beginning debug build v$(VERSION_STRING)"
else
	@echo "Beginning debug build"
endif
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

# Create the directories used in the build
.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS))
	@mkdir -p $(BIN_PATH)

# Installs to the set path
.PHONY: install
install:
	@echo "Installing to $(DESTDIR)$(INSTALL_PREFIX)/bin"
	@$(INSTALL_PROGRAM) $(BIN_PATH)/$(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

# Uninstalls the program
.PHONY: uninstall
uninstall:
	@echo "Removing $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)"
	@$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)

# Removes all build files
.PHONY: clean
clean:
	@echo "Deleting $(BIN_NAME) symlink"
	@$(RM) $(BIN_NAME)
	@echo "Deleting directories"
	@$(RM) -r build
	@$(RM) -r bin

# Main rule, checks the executable and symlinks to the output
all: $(BIN_PATH)/$(BIN_NAME)
	@echo "Making symlink: $(BIN_NAME) -> $<"
	@$(RM) $(BIN_NAME)
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

# Link the executable
$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "Linking: $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo -en "\t Link time: "
	@$(END_TIME)

# Add dependency files, if they exist
-include $(DEPS)

# Source file rules
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CC) $(CFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
	@echo -en "\t Compile time: "
	@$(END_TIME)

```

定义了变量`BIN_NAME`，这个变量定义了最后生成的可执行文件名字为`hello`.
定义了变量`CC`,这个变量代表了要使用的编译器，使用`gcc`编译器.
定义了变量`SRC_EXT`,这个变量代表了源文件的后缀名，使用`.c`的源文件.
定义了变量`SRC_PATH`,这个变量代表了源文件的目录，使用了`.`的目录，本Makefile会递归地查找所有子目录的`.c`文件，本文件只支持指定一个目录.
为了把所有目录的最后一个`/`去掉，若是由`/`后面就有bug.
定义了变量`LIBS`,这个变量代表了要使用的库文件目录.
定义了变量`COMPILE_FLAGS`，这个变量代表了通用的编译器选项.
定义了变量`RCOMPILE_FLAGS`,这个变量代表了专用于发行版的编译器选项.
定义了变量`DCOMPILE_FLAGS`,这个变量代表了专用于debug版的编译器选项.
定义了变量`INCLUDES`,这个变量代表了表示包括的路径，也就是头文件搜索的路径.
定义了变量`LINK_FLAGS`,这个变量代表了通用的链接器选项.
定义了变量`RLINK_FLAGS`,这个变量代表了专用于发行版的链接器选项.
定义了变量`DLIBK_FLAGS`,这个变量代表了专用于debug版的编译器选项.
定义了变量`DSETDIR`,这个变量代表了目标的文件夹，用于系统命令.
定义了变量`INSTALL_PREFIX`,这个变量代表了安装时的路径.

接着结束了用户定义的部分，到达Makefile内部定义部分了.

定义了变量`UNAME_S`,这个变量使用shell函数`uname`来得到当前系统类型，确定命令的写法.
定义了用于打印变量的模式规则`print-%: ; @echo $*=$($*)`，方便我们进行调试。
定义了`SHELL`特殊名称，这个名字会告诉make使用`shell`函数时应该在哪里查找命令.
使用`.SUFFIXED:`取消了所有的隐式规则。
定义了三个变量`INSTALL INSTALL_PROGRAM INSTALL_DATA`用于安装指令.
要是`LIBS`不为空，那么就在编译器选项`COMPILE_FLAGS`和链接器选项`LINK_FLAGS`加入与库文件相关的内容。

定义了两个变量`V`和`CMD_PREFIX`用于打印命令.

接下来就到了真正的规则定义了。

定义了两个版本,一个是`release`版本，一个是`debug`版本。同时按照不同的版本，给`CLAGS`和`LDFLAGS`赋上不同的值。按照不同的版本，定义不同的文件输出路径`BUILD_PATH`和`BIN_PATH`.

接下来，使用Linux`find`命令递归地查找指定的源文件目录的所有`.c`文件，并将他们按照最后修改时间降序排列，再只输出源文件名字，存储在变量`SOURCES`中。

之后定义了一个递归的查找方法，防止前面的命令失效后的备用方法。

这个方法就是递归的给所有路径加上`/`查找子目录，直到遇到了文件，判断文件是否是以`.c`结尾的，如果是，就保留，不是就去除。

得到了所有的源文件后，定义`OBJECTS`,它是把源文件的后缀改为`.o`以及把存储的路径改为了`BUILD_PATH`.

同时定义了每个`.o`文件的`.d`文件，这个文件里面有源文件的头文件依赖。

接着定义了三个变量`TIME_FILE`,`START_TIME`,`END_TIME`,这三个变量是用来测量编译时间的。

接着定义了版本控制有关的变量,`VERSION`,`VERSION_MAJOR`,`VERSION_MINOR`,`VERSION_PATCH`,`VERSION_REVISION`,`VERSION_HASH`,`VERSION_STRING`.

接下来是`release`和`debug`版本都依赖于`dirs`.注意这个配方`$(MAKE) all`这个配方运行`all`.

`dirs`目标是用来生成所有`.o`和二进制文件的文件夹的。`mkdir -p`就会自动创建多级目录。

接着定义了三个命令`install`,`uninstall`,`clean`.

着这到了最主要的规则，也就是编译源文件，生成目标文件的规则了。

`all`依赖于`$(BIN_PATH)/$(BIN_NAME)`,对于`release`版本，就是依赖`bin/release/hello`.

`$(BIN_PATH)/$(BIN_NAME)`依赖于所有的目标文件`$(OBJECTS)`.
这个的配方就是链接所有的`.o`文件最终生成可执行文件。

接着使用`-include`包含源文件的依赖文件，使用`-`忽略错误。

对于每个`.o`文件，定义了模式规则。对于`release`版本，就是`bin/release/%.o : ./%.c`

使用命令`$(CMD_PREFIX)$(CC) $(CFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@`编译，`-MMD`输出`.d`文件去除了标准库的头文件，`-MP`为每个头文件添加了空白的依赖，防止删除头文件后make找不到构建规则而出错。

生成的`.d`文件大致为

```Makefile
./hello/hello.c : ./hello/include/hello.h ./hello/include/hello2.h
./hello/include/hello.h :
./hello/include/hello2.h : 
```

现代编译器都会支持输出`.d`文件的功能，`.d`文件默认和`.o`文件放在一起，编译器会搜索`.c`文件所有的头文件，头文件里的头文件，全部加到依赖里去。

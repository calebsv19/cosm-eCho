HOST_CC ?= cc
FISICS_CC ?= /Users/calebsv/Desktop/CodeWork/fisiCs/fisics
FISICS_FLAGS ?=
BUILD_TOOLCHAIN ?= clang
PACKAGE_TOOLCHAIN ?= $(BUILD_TOOLCHAIN)
TEST_TOOLCHAIN ?= clang
RELEASE_TOOLCHAIN ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
PKG_CONFIG ?= pkg-config

#
# Copyright (c) 2019, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PROJECT		:= ecnt_efuse
V		?= 0
BUILD_INFO	?= 1
DEBUG		:= 0
BINARY		:= ${PROJECT}${BIN_EXT}
OPENSSL_DIR	:= /usr

OBJECTS := src/cmd_opt.o \
           src/main.o

HOSTCCFLAGS := -Wall -std=c99

MAKE_HELPERS_DIRECTORY := ../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}build_env.mk

ifeq (${DEBUG},1)
  HOSTCCFLAGS += -g -O0 -DDEBUG -DLOG_LEVEL=40
else
ifeq (${BUILD_INFO},1)
  HOSTCCFLAGS += -O2 -DLOG_LEVEL=20
else
  HOSTCCFLAGS += -O2 -DLOG_LEVEL=10
endif
endif
ifeq (${V},0)
  Q := @
else
  Q :=
endif

ifneq ($(strip $(TCSUPPORT_UBOOT)),)
  HOSTCCFLAGS += -DCONFIG_ECNT
endif

ifneq ($(strip $(TCSUPPORT_CPU_AN7583)),)
	HOSTCCFLAGS += -DTCSUPPORT_CPU_AN7583
endif

ifneq ($(strip $(TCSUPPORT_ARM_SECURE_BOOT_FLASH_KEY)),)
	HOSTCCFLAGS += -DTCSUPPORT_ARM_SECURE_BOOT_FLASH_KEY
endif

ifneq ($(strip $(TCSUPPORT_CPU_AN7552)),)
	HOSTCCFLAGS += -DTCSUPPORT_CPU_AN7552
endif

# Make soft links and include from local directory otherwise wrong headers
# could get pulled in from firmware tree.
INC_DIR := -I ./include -I ../../include/tools_share 


HOSTCC ?= gcc

.PHONY: all clean realclean

all: clean ${BINARY}

${BINARY}: ${OBJECTS} Makefile
	@echo "  HOSTLD  $@"
	@echo 'const char build_msg[] = "Built : "__TIME__", "__DATE__;' | \
                ${HOSTCC} -c ${HOSTCCFLAGS} -xc - -o src/build_msg.o
	${Q}${HOSTCC} src/build_msg.o ${OBJECTS} -o $@

%.o: %.c
	@echo "  HOSTCC  $<"
	${Q}${HOSTCC} -c ${HOSTCCFLAGS} ${INC_DIR} $< -o $@

clean:
	$(call SHELL_DELETE_ALL, src/build_msg.o ${OBJECTS})

realclean: clean
	$(call SHELL_DELETE,${BINARY})

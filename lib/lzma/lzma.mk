#
# Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LZMA_PATH	:=	lib/lzma

# Imported from zlib 1.2.11 (do not modify them)
LZMA_SOURCES	:=	$(addprefix $(LZMA_PATH)/,	\
					LzmaDec.c)

# Implemented for TF

INCLUDES	+=	-Iinclude/lib/lzma

# REVISIT: the following flags need not be given globally
LZMA_SOURCES	+=	$(addprefix $(LZMA_PATH)/,	\
					LzmaTools.c)

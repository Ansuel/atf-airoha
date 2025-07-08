/*
 * Usefuls routines based on the LzmaTest.c file from LZMA SDK 4.65
 *
 * Copyright (C) 2007-2008 Industrie Dial Face S.p.A.
 * Luigi 'Comio' Mantellini (luigi.mantellini@idf-hit.com)
 *
 * Copyright (C) 1999-2005 Igor Pavlov
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __LZMA_TOOL_H__
#define __LZMA_TOOL_H__

#include <Types.h>
#include <stdint.h>

extern int lzmaBuffToBuffDecompress(uintptr_t *inStream, size_t length, uintptr_t *outStream,
	   size_t uncompressedSize, uintptr_t work_buf, size_t work_len);
#endif

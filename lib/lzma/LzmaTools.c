
#include <LzmaTools.h>
#include <LzmaDec.h>

#include <string.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/utils.h>

#define LZMA_PROPERTIES_OFFSET		0
#define LZMA_SIZE_OFFSET		LZMA_PROPS_SIZE
#define LZMA_DATA_OFFSET		(LZMA_SIZE_OFFSET + sizeof(uint64_t))

#define MAX_SIZE			0x580000
#define ZALLOC_ALIGNMENT		sizeof(void *)

static uintptr_t zalloc_start;
static uintptr_t zalloc_end;
static uintptr_t zalloc_current;
static void * MyAlloc(size_t size)
{
	uintptr_t p, p_end;

	p = round_up(zalloc_current, ZALLOC_ALIGNMENT);
	p_end = p + size;

	if (p_end > zalloc_end)
		return NULL;

	memset((void *)p, 0, size);

	zalloc_current = p_end;

	return (void *)p;
}

static void MyFree(void *address)
{

}

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }

int lzmaBuffToBuffDecompress(uintptr_t *inStream, size_t length, uintptr_t *outStream,
	   size_t uncompressedSize, uintptr_t work_buf, size_t work_len)
{
	unsigned int* p = (unsigned int*)(*outStream);
	int res = SZ_ERROR_DATA;
	int i = 0;

	ISzAlloc g_Alloc;
	SizeT outSize = 0;
	SizeT outSizeHigh = 0;
	SizeT outProcessed = MAX_SIZE;
	ELzmaStatus state;
	SizeT compressedSize = (SizeT)(length - LZMA_PROPS_SIZE);

	zalloc_start = work_buf;
	zalloc_end = work_buf + work_len;
	zalloc_current = zalloc_start;
	INFO("LZMA: Image address............... 0x%lx\n", *inStream);
	INFO("LZMA: Properties address.......... 0x%lx\n", *inStream + LZMA_PROPERTIES_OFFSET);
	INFO("LZMA: Uncompressed size address... 0x%lx\n", *inStream + LZMA_SIZE_OFFSET);
	INFO("LZMA: Compressed data address..... 0x%lx\n", *inStream + LZMA_DATA_OFFSET);
	INFO("LZMA: Destination address......... 0x%lx\n", *outStream);
	memset(&state, 0, sizeof(state));

	INFO("LZMA: Uncompresed size............ 0x%zx\n", outProcessed);
	INFO("LZMA: Compresed size.............. 0x%zx\n", compressedSize);

	for (i = 0; i < 8; i++) {
		unsigned char b = ((unsigned char *) *inStream)[LZMA_SIZE_OFFSET + i];

		if (i < 4) {
			outSize     += (SizeT)(b) << (i * 8);
		} else {
			outSizeHigh += (SizeT)(b) << ((i - 4) * 8);
		}
	}

	if ((outSizeHigh != 0) || (outSize > outProcessed)) {
		return SZ_ERROR_DATA;
	}

	outProcessed = outSize;

	g_Alloc.Alloc = SzAlloc;
	g_Alloc.Free = SzFree;

	res = LzmaDecode((Byte *) *outStream, &outProcessed,
			 ((Byte *) *inStream) + LZMA_DATA_OFFSET, &compressedSize,
			 ((Byte *) *inStream) , LZMA_PROPS_SIZE, LZMA_FINISH_END, &state, &g_Alloc);
	INFO("LZMA: Uncompresed ................ 0x%zx\n", outProcessed);

	VERBOSE("\033[33;1m 	decompressed data=> 0x%x-0x%x-0x%x-0x%x    \n\033[0m",*p,*(p+1), *(p+2), *(p+3));
	if (res != SZ_OK) {
		ERROR("LZMA: res %d state %d\n", res, state);
	}

	*outStream = round_up(*outStream + outProcessed, sizeof(uintptr_t ));

	return res;
}

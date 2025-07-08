#ifndef __FLASHHAL_H__
#define __FLASHHAL_H__

#include <stdint.h>	
#include <plat_private.h>

typedef enum {
	FLASH_INIT_SUCCESS = 0,
	FLASH_INIT_FAIL
} FLASH_INIT_T;

typedef enum{
	FLASH_READ_STATUS_CORRECT = 0,
	FLASH_READ_STATUS_INCORRECT
} FLASH_READ_STATUS_T;

typedef enum{
	FLASH_READ_BYTE_SWAP = 0,
	FLASH_READ_NO_BYTE_SWAP
} FLASH_READ_T;

extern FLASH_INIT_T flash_init(hw_trap_t *hw_trap);
extern FLASH_READ_STATUS_T flash_read(uint32_t from, uint32_t len, uint8_t *p_buf);
extern int flash_erase(uint32_t addr, uint32_t size);
extern int flash_write(uint32_t to, uint32_t len, uint8_t *p_buf);

#endif /* __FLASHHAL_H__ */

#ifndef _TC3162_H_
#define _TC3162_H_

#include <common/debug.h>
#include <ecnt_scu.h>
#include <asm/io.h>
#include <ecnt_pkgid.h>
#include <linux/types.h>


#undef MIN
#define prom_printf				printf
#define mb						dmb

#define isEN7523				(GET_HIR() == EN7523_HIR)
#define isEN7581				(GET_HIR() == EN7581_HIR)
#define isAN7552				(GET_HIR() == AN7552_HIR)
#define isAN7583				(GET_HIR() == AN7583_HIR)
#define isEN7526c				(0)
#define isEN751627				(0)
#define isEN7580				(0)
#define isFPGA					(GET_IS_FPGA())

#endif

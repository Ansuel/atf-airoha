#!/bin/bash

BASE_DIR="$(pwd)"

CHIP="an7581"
OPTEE="no"

# Please check:
#  - OVERRIDE_UBI_START_ADDR value should be a multiplier of flash erase
#    block size. Minimal value is
#      - 0x20000 for flashes with 128K erase block
#      - 0x40000 for flashes with 256K erase block
#  - OVERRIDE_PLAT_ECNT_BL31_FIP_OFFSET value should be a multiplier of
#    eMMC I/O block (512 bytes). Minimal value is 0x20000
#  - TCSUPPORT_GPT_ATF_SUPPORT should not be defined/enabled if GPT
#    partitioning is not used for eMMC
export TCSUPPORT_BB_FIX_UNOPEN=1
export TCSUPPORT_SPI_NAND_FLASH_ECC_DMA=1
export TCSUPPORT_BL2_OPTIMIZATION=1
export TCSUPPORT_CPU_EN7581=1
export TCSUPPORT_EMMC=1
export TCSUPPORT_GPT_ATF_SUPPORT=1
export OVERRIDE_PLAT_ECNT_BL31_FIP_OFFSET=0x84000
export TCSUPPORT_UBI_SUPPORT=1
export OVERRIDE_UBI_START_ADDR=0x100000
export TCSUPPORT_UBOOT=1

BSP_CFLAGS="-fsigned-char \
	    -Wno-error=date-time \
	    -Wno-error=missing-include-dirs -Wno-error=redundant-decls \
	    -DTCSUPPORT_CPU_ARMV8_64 \
	"
LDFLAGS=""

if [ "${OPTEE}" = "yes" ]; then
	BSP_CFLAGS="${BSP_CFLAGS} -DTCSUPPORT_OPTEE"
	OPTEE_BL23_OPT="TCSUPPORT_OPTEE=1"
	OPTEE_BL31_OPT="TCSUPPORT_OPTEE=1 SPD=opteed"
else
	OPTEE_BL23_OPT=""
	OPTEE_BL31_OPT=""
fi

export BSP_CFLAGS

OUT_DIR="${BASE_DIR}/out/${CHIP}"
BUILD_ATF_DIR="${BASE_DIR}/arm-trusted-firmware-2.10.0"

ATF_CFLAGS_COMMON="${BSP_CFLAGS} -Wl,--no-warn-execstack \
	-Wno-error=missing-include-dirs -Wno-error=redundant-decls"

ATF_CROSS_STAGING_DIR_AARCH32="${BASE_DIR}/toolchain-arm"
ATF_CROSS_PATH_AARCH32="${ATF_CROSS_STAGING_DIR_AARCH32}/bin"
ATF_CROSS_COMPILE_AARCH32="${ATF_CROSS_PATH_AARCH32}/arm-openwrt-linux-"
ATF_LDFLAGS_AARCH32="${LDFLAGS} --no-warn-execstack"
ATF_CFLAGS_AARCH32="${ATF_CFLAGS_COMMON} --param=min-pagesize=0"

ATF_CROSS_STAGING_DIR_AARCH64="${BASE_DIR}/toolchain-aarch64"
ATF_CROSS_PATH_AARCH64="${ATF_CROSS_STAGING_DIR_AARCH64}/bin"
ATF_CROSS_COMPILE_AARCH64="${ATF_CROSS_PATH_AARCH64}/aarch64-openwrt-linux-"
ATF_LDFLAGS_AARCH64="${LDFLAGS} --no-warn-execstack --no-warn-rwx-segments"
ATF_CFLAGS_AARCH64="${ATF_CFLAGS_COMMON}"

aarch32_clean() {
	echo ">>> ATF: cleanup (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 distclean         \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl21() {
	echo ">>> ATF: building BL21 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL21=1  \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl22() {
	echo ">>> ATF: building BL22 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL22=1  \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl23() {
	echo ">>> ATF: building BL23 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL23=1  \
		${OPTEE_BL23_OPT}                                             \
		TCSUPPORT_BL2_OPTIMIZATION=1                                  \
		TCSUPPORT_UBI_SUPPORT=1                                       \
		TCSUPPORT_GPT_ATF_SUPPORT=1                                   \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch64_clean() {
	echo ">>> ATF: cleanup (aarch64) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch64 distclean         \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH64}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH64}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH64}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH64}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH64}"
}

aarch64_build_bl31() {
	echo ">>> ATF: building BL31 (aarch64) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch64 bl31              \
		${OPTEE_BL31_OPT}                                             \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH64}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH64}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH64}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH64}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH64}"
}

export PATH="${BASE_DIR}/bin:${PATH}"

(    aarch32_clean \
  && aarch32_build_bl21 \
  && echo ------------------------------------------------ \
  && aarch32_clean \
  && aarch32_build_bl22 \
  && echo ------------------------------------------------ \
  && aarch32_clean \
  && aarch32_build_bl23 \
  && echo ------------------------------------------------ \
  && aarch32_clean \
  && aarch64_clean \
  && aarch64_build_bl31 \
  && echo ------------------------------------------------ \
) || exit 1

if [ ! -e ../bin/trx-airoha ]; then
    echo "ERROR: ../bin/trx-airoha utility is missed, Can't continue"
    exit 1
fi

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

gcc -DFLASH_TABLE_OPEN -DTCSUPPORT_BL2_OPTIMIZATION \
	-I"${BUILD_ATF_DIR}/plat/ecnt/en7523/include" \
	-o "${OUT_DIR}/flash-table-airoha" \
	"${BASE_DIR}/arm-trusted-firmware-2.10.0/plat/ecnt/common/drivers/flash/spi_nand_flash_table.c"
"${OUT_DIR}/flash-table-airoha"
../bin/lzma e flash_table.bin flash_table.lzma
mv flash_table.lzma "${OUT_DIR}"
rm flash_table.bin

../bin/trx-airoha -z \
	"${BUILD_ATF_DIR}/bl21.bin"  \
	"${BUILD_ATF_DIR}/bl22.lzma" \
	"${BUILD_ATF_DIR}/bl23.lzma" \
	"${OUT_DIR}/flash_table.lzma"     \
	bl2.tmp
../bin/trx-airoha -x bl2.tmp bl2_crc.bin
mv bl2_crc.bin "${OUT_DIR}/${CHIP}-bl2.bin"
rm bl2.tmp

../bin/lzma e ${BUILD_ATF_DIR}/build/en7523/release/bl31.bin "${OUT_DIR}/${CHIP}-bl31.lzma"

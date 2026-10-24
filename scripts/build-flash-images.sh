#!/bin/bash

BASE_DIR="$(pwd)"
script_name=$(basename "$0")

function help(){
	echo "usage: $1 [options]"
	echo ""
	echo "Options:"
	echo "    --help, -h              get this help"
	echo "    --chip chip, -c chip    select an7581/an7583/en7523 chip"
	echo "    --optee path, -o path   path to lzma compressed optee image"
	echo "    --uboot path, -u path   path to 'u-boot.bin' binary for your chip"
}

CHIP=""
optee=""
uboot=""
while [ -n "$1" ]; do
	case "$1" in
		(--help|-h)
			help "${script_name}"
			exit
			;;
		(--chip|-c)
			if [ -z "$2" ]; then
				echo "Error: chip must be specified"
				help "${script_name}"
				exit 1
			fi
			CHIP="$2"
			shift
			shift
			;;
		(--optee|-o)
			if [ -z "$2" ]; then
				echo "Error: path to lzma compressed optee image must be specified"
				help "${script_name}"
				exit 1
			fi
			optee="$2"
			shift
			shift
			;;
		(--uboot|-u)
			if [ -z "$2" ]; then
				echo "Error: path to 'u-boot.bin' must be specified"
				help "${script_name}"
				exit 1
			fi
			uboot="$2"
			shift
			shift
			;;
		(*)
			echo "Error: unknown option $1"
			help "${script_name}"
			exit 1
			;;
	esac
done

if [ -z "${CHIP}" -o -z "${uboot}" ]; then
	echo "Error: mandatory options were missed"
	help "${script_name}"
	exit 1
fi

OUT_DIR="${BASE_DIR}/out/${CHIP}"

../bin/lzma e "${uboot}" ${OUT_DIR}/${CHIP}-u-boot-ram.lzma

../bin/fiptool create					\
	--align 1024					\
	--tb-fw ${OUT_DIR}/${CHIP}-bl2.bin		\
	${OUT_DIR}/${CHIP}-bl2.fip

[ -e ${OUT_DIR}/${CHIP}-bl1.bin ] || dd if=/dev/zero of=${OUT_DIR}/${CHIP}-bl1.bin bs=2048 count=1

cp ${OUT_DIR}/${CHIP}-bl1.bin ${OUT_DIR}/${CHIP}-bl2.img
truncate -s 2048 ${OUT_DIR}/${CHIP}-bl2.img
cat ${OUT_DIR}/${CHIP}-bl2.fip >>${OUT_DIR}/${CHIP}-bl2.img

if [ -z "${optee}" ]; then
	../bin/fiptool create					\
		--align 1024					\
		--soc-fw ${OUT_DIR}/${CHIP}-bl31.lzma		\
		--nt-fw ${OUT_DIR}/${CHIP}-u-boot-ram.lzma	\
		${OUT_DIR}/${CHIP}-bl31-uboot.fip
else
	cp "${optee}" ${OUT_DIR}/${CHIP}-bl32_optee.lzma
	../bin/fiptool create					\
		--align 1024					\
		--soc-fw ${OUT_DIR}/${CHIP}-bl31.lzma		\
		--tos-fw ${OUT_DIR}/${CHIP}-bl32_optee.lzma	\
		--nt-fw ${OUT_DIR}/${CHIP}-u-boot-ram.lzma	\
		${OUT_DIR}/${CHIP}-bl31-optee-uboot.fip
fi

echo "Final images:"
echo "----------------------------------------"
echo "* ${OUT_DIR}/${CHIP}-bl2.img"
echo "	BL2 bootloader image:"
echo "		- Write it to the flash/emmc starting from offset 0 to boot from"
echo "		  the device"
echo "* ${OUT_DIR}/${CHIP}-bl2.fip"
echo "	BL2 fip image:"
echo "		- Use it as first (bootext.ram) image for board recovery"
echo "		  using Xmodem protocol"
echo "		- Write it to the emmc starting from offset 2048 to boot from"
echo "		  the device. Space before and after this image can be used for"
echo "		  GPT partition table"
if [ -z "${optee}" ]; then
	echo "* ${OUT_DIR}/${CHIP}-bl31-uboot.fip"
else
	echo "* ${OUT_DIR}/${CHIP}-bl31-optee-uboot.fip"
fi
echo "	BL31/U-Boot fip image:"
echo "		- Use it as second (bl31/u-boot) image for board recovery"
echo "		  using Xmodem protocol"
echo "		- Write it to the UBI/GPT 'fip' volume to boot from the device"

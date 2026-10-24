#!/bin/bash

SCRIPT_DIR=$(dirname "$0")
ATF_AIROHA_DIR=${SCRIPT_DIR}/..
ATF_AIROHA_DIR=$(realpath "${ATF_AIROHA_DIR}")

mkdir atf-airoha-build
cd atf-airoha-build

mkdir downloads
pushd downloads >/dev/null
git clone --tags -b v2.10.0 https://github.com/TrustedFirmware-A/trusted-firmware-a.git
wget -O mbedtls-3.4.0.tar.gz https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v3.4.0.tar.gz
wget https://downloads.openwrt.org/releases/25.12.5/targets/mediatek/filogic/openwrt-toolchain-25.12.5-mediatek-filogic_gcc-14.3.0_musl.Linux-x86_64.tar.zst
wget https://downloads.openwrt.org/releases/25.12.5/targets/mediatek/mt7629/openwrt-toolchain-25.12.5-mediatek-mt7629_gcc-14.3.0_musl_eabi.Linux-x86_64.tar.zst
popd >/dev/null

tar xf downloads/mbedtls-3.4.0.tar.gz
tar xf downloads/openwrt-toolchain-25.12.5-mediatek-filogic_gcc-14.3.0_musl.Linux-x86_64.tar.zst
tar xf downloads/openwrt-toolchain-25.12.5-mediatek-mt7629_gcc-14.3.0_musl_eabi.Linux-x86_64.tar.zst

mkdir -p build
pushd build >/dev/null
cp -a ../downloads/trusted-firmware-a arm-trusted-firmware-2.10.0
cp -a "${ATF_AIROHA_DIR}"/* arm-trusted-firmware-2.10.0/
ln -s ../mbedtls-3.4.0 mbedtls-3.4.0
ln -s ../openwrt-toolchain-*-mediatek-filogic_*/toolchain-aarch64* toolchain-aarch64
ln -s ../openwrt-toolchain-*-mediatek-mt7629_*/toolchain-arm* toolchain-arm
cp "${ATF_AIROHA_DIR}"/scripts/build-[ae]n75[28][13].sh .
cp "${ATF_AIROHA_DIR}"/scripts/build-flash-images.sh .
popd >/dev/null

mkdir -p bin
if [ ! -e bin/trx-airoha ]; then
    echo
    echo "WARNING: 'trx-airoha' utility is missed. Please find it and put with proper"
    echo "  name to 'atf-airoha-build/bin' directory. The utility (and it sources) can"
    echo "  be found in your Airoha SDK build directory. Sources are placed in"
    echo
    echo "    \${SDK}/tclinux_phoenix/tools/trx"
    echo
    echo "  directory. Binary file placed in"
    echo
    echo "    \${SDK}/openwrt-21.02/openwrt-21.02.1_dev/build_dir/target-aarch64_cortex-a53_musl/linux-airoha_\${SOC}/bootloader/tools/trx/trx"
fi

if [ ! -e bin/lzma ]; then
    lzma=$(which lzma_alone)
    if [ -n "$lzma" ]; then
        cp "$lzma" bin/lzma
    else
        echo
        echo "WARNING: OpenWrt lzma (lzma_alone) utility is missed. Please find it and put"
        echo "  with proper name to 'atf-airoha-build/bin' directory. The utility can be "
        echo "  found in your OpenWrt build directory, see"
        echo
        echo "    \${openwrt}/staging_dir/host/bin/lzma"
    fi
fi

if [ ! -e bin/fiptool ]; then
    fiptool=$(which fiptool)
    if [ -n "$fiptool" ]; then
        cp "$fiptool" bin/fiptool
    else
        echo
        echo "WARNING: fiptool utility is missed. Please find it and put to 'atf-airoha-build/bin'"
        echo "  directory. The utility can be found in your OpenWrt build directory, see"
        echo
        echo "    \${openwrt}/staging_dir/host/bin/fiptool"
    fi
fi

if [ ! -e "${ATF_AIROHA_DIR}/plat/ecnt/blobs/en7523/bl1.bin" ]; then
    echo
    echo "WARNING: EN7523 SoC blobs are missed. Please find missed blobs"
    echo "  and put them to"
    echo
    echo "    ${ATF_AIROHA_DIR}/plat/ecnt/blobs/en7523/"
    echo
    echo "  subdirectory. The following blobs are required:"
    echo "    /"
    echo "    +-- bl1.bin"
    echo "    +-- bl22"
    echo "    |     +-- dramc.o"
    echo "    |     +-- dramc_pi_basic_api.o"
    echo "    |     +-- dramc_pi_calibration_api.o"
    echo "    |     +-- dramc_pi_main.o"
    echo "    |     +-- efuse_load.o"
    echo "    |     +-- efuse.o"
    echo "    |     +-- hal_io.o"
    echo "    +-- bl23"
    echo "    |     +-- cortex_a53.o"
    echo "    |     +-- ecnt_npu_img.o"
    echo "    |     +-- efuse_load.o"
    echo "    |     +-- efuse.o"
    echo "    +-- bl31.bin"
fi

echo
echo "=================================================================="
echo "Please enter 'atf-airoha-build/build' directory and run one of the"
echo "following scripts:"
echo " * build-an7581.sh"
echo " * build-an7583.sh"
echo " * build-en7523.sh"
echo "to build ATF blobs for the SoC of your choice. After the build you"
echo "may find builds artefacts in 'out/\${SOC}' subdirectory."
echo "=================================================================="

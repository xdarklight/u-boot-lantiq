#!/bin/bash

set -e

output_dir=/tmp
declare -a arch_list
arch_cnt=0
buildman_cfg=
toolchain_env=
paths=

toolchain_base_url=https://www.kernel.org/pub/tools/crosstool/files/bin/x86_64/4.9.0
toolchain_version=gcc-4.9.0-nolibc

declare -A toolchain_list
toolchain_list[aarch64]=x86_64-${toolchain_version}_aarch64-linux.tar.xz
toolchain_list[arm]=x86_64-${toolchain_version}_arm-unknown-linux-gnueabi.tar.xz
toolchain_list[mips]=x86_64-${toolchain_version}_mips-linux.tar.xz
toolchain_list[powerpc]=x86_64-${toolchain_version}_powerpc-linux.tar.xz

declare -A toolchain_prefix
toolchain_prefix[aarch64]=aarch64-linux
toolchain_prefix[arm]=arm-unknown-linux-gnueabi
toolchain_prefix[mips]=mips-linux
toolchain_prefix[powerpc]=powerpc-linux

usage_msg()
{
    echo "prepare-toolchain.sh [options]"
    echo " -o PATH  - output directory (default /tmp)"
    echo " -a ARCH  - architecture to download (can be repeated)"
    echo " -b FILE  - generate buildman config"
    echo " -t FILE  - path to generated toolchain env script"
}

die()
{
    echo $@ >2
    exit 1
}

parse_args()
{
    while getopts 'o:a:b:t:' opt; do
        case $opt in
        o)
            output_dir=$OPTARG
            ;;
        a)
            arch_list[$arch_cnt]=$OPTARG
            arch_cnt=$(($arch_cnt + 1))
            ;;
        b)
            buildman_cfg=$OPTARG
            ;;
        t)
            toolchain_env=$OPTARG
            ;;
        \?)
            usage_msg
            exit 1
            ;;
        esac
    done
}

check_arch()
{
    case $1 in
    aarch64|arm|mips|powerpc|sandbox)
        return 0
        ;;
    *)
        ;;
    esac

    return 1
}

install_toolchain()
{
    local arch=$1
    local url=${toolchain_base_url}/${toolchain_list[$arch]}
    local tarball=${output_dir}/${toolchain_list[$arch]}

    wget $url -O $tarball
    tar -xf $tarball -C $output_dir
}

create_buildman_cfg()
{
    [ $buildman_cfg ] || return 0
    echo "[toolchain]" > $buildman_cfg
    echo "root: /" >> $buildman_cfg
}

append_buildman_cfg()
{
    [ $buildman_cfg ] || return 0
    local arch=$1
    echo "$arch: ${output_dir}/${toolchain_version}/${toolchain_prefix[$arch]}" >> $buildman_cfg
}

finish_buildman_cfg()
{
    [ $buildman_cfg ] || return 0
    echo "[toolchain-alias]" >> $buildman_cfg
    echo "[make-flags]" >> $buildman_cfg
}

create_toolchain_env()
{
    [ $toolchain_env ] || return 0
    echo -n "" > $toolchain_env
}

append_toolchain_env()
{
    [ $toolchain_env ] || return 0
    local arch=$1
    echo "${arch}_path=${output_dir}/${toolchain_version}/${toolchain_prefix[$arch]}/bin" >> $toolchain_env
    paths="$paths:\${${arch}_path}"
}

finish_toolchain_env()
{
    [ $toolchain_env ] || return 0
    echo "PATH=\$PATH${paths}" >> $toolchain_env
}

parse_args $@ || usage_msg
create_buildman_cfg
create_toolchain_env

for arch in ${arch_list[*]}; do
    check_arch $arch || die "invalid arch"
    [ "$arch" != "sandbox" ] || continue
    install_toolchain $arch
    append_buildman_cfg $arch
    append_toolchain_env $arch
done

finish_buildman_cfg
finish_toolchain_env

#!/bin/bash

###############################################################################
#Variables
export PROJECT_LIST=$(find project | grep "GCC/Makefile$")
export BOARD_LIST="project/*"
export OUT="$PWD/out"
export FLASHGENERATOR="tools/flashgen/flashgen.pl"
#export CODING_STYLE="$PWD/tools/scripts/build/format_coding_style.py"
export DEPENDENCY_MSG="$PWD/tools/scripts/build/dep_chk.py" 
feature_mk=""
feature_mk_o=""
bl_feature_mk=""
bl_feature_mk_o=""
bl_feature_mk_o_flag=0
hal_feature_mk=""
flash_ld=""
mem_ld=""
mem_ld_path=""
rmode=""
default_rmode="debug"
no_mcfg="false"


platform=$(uname)
if [[ "$platform" =~ "MINGW" ]]; then
    export EXTRA_VAR=-j
else
    export EXTRA_VAR=-j`cat /proc/cpuinfo |grep ^processor|wc -l`
fi
###############################################################################
#Functions
show_usage () {
    echo "==============================================================="
    echo "Build Project"
    echo "==============================================================="
    echo "Usage: $0 <board> <project> [bl|clean] <argument>"
    echo ""
    echo "Example:"
    echo "       $0 mt7933_hdk iot_sdk_demo"
    echo "       $0 mt7933_hdk iot_sdk_demo bl      (build with bootloader)"
    echo "       $0 clean                      (clean folder: out)"
    echo "       $0 mt7933_hdk clean           (clean folder: out/mt7933_hdk)"
    echo "       $0 mt7933_hdk iot_sdk_demo clean   (clean folder: out/mt7933_hdk/iot_sdk_demo)"
    echo ""
    echo "Argument:"
    echo "       -f=<feature makefile> or --feature=<feature makefile>"
    echo "           Replace feature.mk with other makefile. For example, "
    echo "           the feature_example.mk is under project folder, -f=feature_example.mk"
    echo "           will replace feature.mk with feature_example.mk."
    echo ""
    echo "       -o=<make option> or --option=<make option>"
    echo "           Assign additional make option. For example, "
    echo "           to compile module sequentially, use -o=-j1."
    echo "           to turn on specific feature in feature makefile, use -o=<feature_name>=y"
    echo "           to assign more than one options, use -o=<option_1> -o=<option_2>."
    echo ""
    echo "==============================================================="
    echo "List Available Example Projects"
    echo "==============================================================="
    echo "Usage: $0 list"
    echo ""
}

show_available_proj () {
    echo "==============================================================="
    echo "Available Build Projects:"
    echo "==============================================================="
    for b in $BOARD_LIST
    do
        project_path=""
        project_name_list=""
        p=$(echo $PROJECT_LIST | tr " " "\n" | grep "$b")
        if [ ! -z "$p" ]; then
            echo  "  "`basename $b`
        fi
        for q in $p
        do
            if [ -e "$q" ]; then
                project_path=$(echo $q | sed 's/GCC\/Makefile//')
                project_name_list="${project_name_list} $(basename $project_path)"
            fi
        done
        for i in `echo $project_name_list | tr " " "\n" | sort`
        do
            echo "  ""  "$i
        done
    done
}

use_new_out_folder="false"
target_check () {
    for p in $PROJECT_LIST
    do
        q=$(echo $p | grep "project/$1/")
        if [ ! -z "$q" ]; then
            r=$(echo $q | sed 's/GCC\/Makefile//')
            s=`basename $r`
            if [ "$s" == "$2" ]; then
                echo "UE BUILD BOARD: $1"
                echo "UE BUILD PROJECT: $2"
                if [ $use_new_out_folder == "false" ]; then
                    OUT=$OUT/$1/$2
                fi
                BUILD=project/$1/$2
                export TARGET_PATH=$(dirname $q)
                return 0
            fi
        fi
    done
    return 1
}

# support MinGW
mingw_check () {
    echo "platform=$platform"
    if [[ "$platform" =~ "MINGW" ]]; then
        pwdpath=$(pwd)
        echo $pwdpath
        if [[ "$pwdpath" =~ "\[|\]| " ]]; then
            echo "Build.sh Exception: The codebase folder name should not have spacing, [ or ]."
            exit 1
        fi
    fi
}

clean_out () {
    rm -rf $1
    echo "rm -rf $1"
}

prebuilt_bt_wifi_fw_prefix_Basic="$(pwd)/prebuilt/driver/chip/mtk_internal"
prebuilt_bt_wifi_fw_prefix_QC="$(pwd)/prebuilt/driver/chip"
#$1: $where_to_find_feature_mk(dir : which contains feature.mk)
#$2: $feature_mk(file name of feature.mk, may be feature_example.mk)
copy_fw_from_prebuilt () {
    echo "copy fw from prebult repo"
    #if $2 comes from parameters -f -o ,$1 will be empty
    feature_mk_dir=$1
    if [ -z "$1" ]; then
        feature_mk_dir=$TARGET_PATH
    fi
    feature_set=$feature_mk_dir/$2
    IC=`grep "^IC_CONFIG\ *[?:]\{0,1\}=\ *" $feature_set | cut -d '=' -f2 | tr -d ' ' | tail -1`
    BT_FLAG=`grep "^MTK_MT7933_BT_ENABLE\ *[?:]\{0,1\}=\ *" $feature_set | cut -d '=' -f2 | tr -d ' ' | tail -1`
    DSP_FLAG=`grep "^MTK_HIFI4DSP_ENABLE\ *[?:]\{0,1\}=\ *" $feature_set | cut -d '=' -f2 | tr -d ' ' | tail -1`
    DSP_BT_AUDIO_FLAG=`grep "^MTK_HIFI4DSP_BT_AUDIO_ENABLE\ *[?:]\{0,1\}=\ *" $feature_set | cut -d '=' -f2 | tr -d ' ' | tail -1`
    WIFI_FLAG=`grep "^MTK_MT7933_CONSYS_WIFI_ENABLE\ *[?:]\{0,1\}=\ *" $feature_set | cut -d '=' -f2 | tr -d ' ' | tail -1`
    echo "feature set : $feature_set"

    #BT
    if [ "$BT_FLAG" == "Y" ] || [ "$BT_FLAG" == "y" ]; then
        echo " IC : $IC ,copying BT FW into out"
        BT_FW_QC_DIR=$prebuilt_bt_wifi_fw_prefix_QC/$IC
        BT_FW_Basic_DIR=$prebuilt_bt_wifi_fw_prefix_Basic/$IC/bt

        BT_Copy_DIR=""
        if [ -d $BT_FW_Basic_DIR ]; then
            BT_Copy_DIR=$BT_FW_Basic_DIR
        elif [ -d $BT_FW_QC_DIR ]; then
            BT_Copy_DIR=$BT_FW_QC_DIR
        fi
        echo " BT FW copy from: $BT_Copy_DIR"

        if [ -e $BT_Copy_DIR/BT_RAM_CODE_MT7933_1_1_hdr.bin ]; then
            cp $BT_Copy_DIR/BT_RAM_CODE_MT7933_1_1_hdr.bin $OUT
        fi
        if [ -e $BT_Copy_DIR/BT_RAM_CODE_MT7933_2_1_hdr.bin ]; then
            cp $BT_Copy_DIR/BT_RAM_CODE_MT7933_2_1_hdr.bin $OUT
        fi
    fi

    #DSP
    if [ "$DSP_FLAG" == "Y" ] || [ "$DSP_FLAG" == "y" ]; then
        echo " IC : $IC ,copying DSP FW into out"
        DSP_FW_DIR=$prebuilt_bt_wifi_fw_prefix_QC/$IC/adsp_prebuilt
        echo " DSP Bin copy from: $DSP_FW_DIR"
        if [ "$DSP_BT_AUDIO_FLAG" == "Y" ] || [ "$DSP_BT_AUDIO_FLAG" == "y" ]; then
            if [ -e $DSP_FW_DIR/bt_audio/hifi4dsp_load.bin ]; then
               cp $DSP_FW_DIR/bt_audio/hifi4dsp_load.bin $OUT
            fi
        else
            if [ -e $DSP_FW_DIR/iot7933bga-hadron/hifi4dsp_load.bin ]; then
                cp $DSP_FW_DIR/iot7933bga-hadron/hifi4dsp_load.bin $OUT
            fi
        fi
    fi

    #WIFI
    if [ "$WIFI_FLAG" == "Y" ] || [ "$WIFI_FLAG" == "y" ]; then
        echo " IC : $IC ,copying WIFI FW into out"
        WIFI_FW_QC_DIR=$prebuilt_bt_wifi_fw_prefix_QC/$IC
        WIFI_FW_Basic_DIR=$prebuilt_bt_wifi_fw_prefix_Basic/$IC/mobile

        WIFI_Copy_DIR=""
        if [ -d $WIFI_FW_Basic_DIR ]; then
            WIFI_Copy_DIR=$WIFI_FW_Basic_DIR
        elif [ -d $WIFI_FW_QC_DIR ]; then
            WIFI_Copy_DIR=$WIFI_FW_QC_DIR
        fi

        echo " WIFI FW copy from: $WIFI_Copy_DIR"

        if [ -e $WIFI_Copy_DIR/mt7933_patch_e1_hdr.bin ]; then
            cp $WIFI_Copy_DIR/mt7933_patch_e1_hdr.bin $OUT
        fi
        if [ -e $WIFI_Copy_DIR/WIFI_RAM_CODE_iemi.bin ]; then
            cp $WIFI_Copy_DIR/WIFI_RAM_CODE_iemi.bin $OUT
        fi
        if [ -e $WIFI_Copy_DIR/WIFI_RAM_CODE_log.bin ]; then
            cp $WIFI_Copy_DIR/WIFI_RAM_CODE_log.bin $OUT
        fi
        if [ -e $WIFI_Copy_DIR/WIFI_RAM_CODE_MT7933_APSOC.bin ]; then
            cp $WIFI_Copy_DIR/WIFI_RAM_CODE_MT7933_APSOC.bin $OUT
        fi
        if [ -e $WIFI_FW_QC_DIR/MT7933_BGA_TDD_EEPROM.bin ]; then
            cp $WIFI_FW_QC_DIR/MT7933_BGA_TDD_EEPROM.bin $OUT
        fi
        if [ -e $WIFI_FW_QC_DIR/MT7931_QFN_TDD_EEPROM.bin ]; then
            cp $WIFI_FW_QC_DIR/MT7931_QFN_TDD_EEPROM.bin $OUT
        fi
        if [ -e $WIFI_Copy_DIR/$WIFI_RAM_CODE_MT7933_ALL.bin ]; then
            cp $WIFI_Copy_DIR/$WIFI_RAM_CODE_MT7933_ALL.bin $OUT
        fi
    fi
}


feature_mk_check()
{
    #Set default feature_mk path
    where_to_find_feature_mk=$TARGET_PATH

    #Check if feature_mk is assigned by build command: -o=FEATURE=xxx.mk
    if [ ! -z $feature_mk_o ]; then
        if [ ! -z $feature_mk ]; then
            echo "Override -f=$feature_mk by -o=FEATURE=$feature_mk_o"
            echo "FEATURE=$feature_mk_o"
        fi
        feature_mk=$feature_mk_o
    fi

    #Check if feature_mk is assigned by build command: -o=FEATURE=xxx.mk or -f=xxx.mk
    if [ ! -z $feature_mk ]; then
        if [ ! -e "$TARGET_PATH/$feature_mk" ]; then
            echo "Error: cannot find $feature_mk under $TARGET_PATH."
            exit 1
        fi
        extra_opt+=" FEATURE=$feature_mk"
    else
        #Get the default config - feature_mk from Makefile
        where_to_find_feature_mk=`grep "^TARGET_PATH\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
        if [ -z $where_to_find_feature_mk ]; then
            where_to_find_feature_mk=$TARGET_PATH
        fi
        feature_mk=`grep "^FEATURE\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
    fi

    echo "Check: FEATURE=$feature_mk"
}


release_mode_check()
{
    #Check if release mode is assigned by build command: release/debug/mfg
    if [ -z $cfgmode ]; then
        #Check if release configure file exist - multi-release mode support
        releaseCfg=`grep "^RELEASE\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
        if [ -e "$TARGET_PATH/$releaseCfg" ]; then
            cfgmode=`grep "^MTK_RELEASE_MODE\ *[?:]\{0,1\}=\ *" $TARGET_PATH/$releaseCfg | cut -d '=' -f2 |  sed 's/^[ ]*//g' | tail -1`
        fi

        if [ -z "$cfgmode" ]; then
            cfgmode=`grep "^MTK_RELEASE_MODE\ *[?:]\{0,1\}=\ *" $where_to_find_feature_mk/$feature_mk | cut -d '=' -f2 | tr -d ' ' | tail -1`
            if [ -z $cfgmode ]; then
                cfgmode=$default_rmode
            fi
            no_mcfg="true"
        fi
    fi
}

release_mode_cfg()
{
    if [ "$#" -eq "0" ]; then
        echo "Check: No Mode Config Folder. Using Default."
        return 0
    fi

    mcfg="$TARGET_PATH/$1"
    if [ ! -e "$mcfg" ]; then
        echo "Check: Mode Config Folder ($mcfg) Not Exist. Using Default."
        return 0
    fi
    echo "Folder: $mcfg"

    #Get default configure from Makefile - HAL_FEATURE/FLASH_LD/MEM_LD/MEM_LD_PATH
    hal_feature_mk=`grep "^HAL_FEATURE\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
    flash_ld=`grep "^FLASH_LD\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
    mem_ld=`grep "^MEM_LD\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
    mem_ld_path=`grep "^MEM_LD_PATH\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`

    if [ -e "$mcfg/$feature_mk" ]; then
        feature_mk="$1/$feature_mk"
        EXTRA_VAR+=" FEATURE=$feature_mk"
    fi

    if [ -e "$mcfg/$hal_feature_mk" ]; then
        hal_feature_mk="$1/$hal_feature_mk"
        EXTRA_VAR+=" HAL_FEATURE=$hal_feature_mk"
    fi

    if [ -e "$mcfg/$flash_ld" ]; then
        flash_ld="$1/$flash_ld"
        EXTRA_VAR+=" FLASH_LD=$flash_ld"
    fi

    if [ -e "$mcfg/$mem_ld" ]; then
        mem_ld="$1/$mem_ld"
        mem_ld_path="$1/$mem_ld_path"
        EXTRA_VAR+=" MEM_LD=$mem_ld MEM_LD_PATH=$mem_ld_path"
    fi

    MAIN_PRJ_DIR="$PWD/$TARGET_PATH/$1"
    bl_extra_opt+=" MAIN_PRJ_DIR=$MAIN_PRJ_DIR"

    #echo "release_mode_cfg()"
    #echo "Release: Mode=[$1]"
    #echo "Define : FEATURE=[$feature_mk] HAL_FEATURE=[$hal_feature_mk] FLASH_LD=[$flash_ld] MEM_LD=[$mem_ld] MEM_LD_PATH=[$mem_ld_path]"
    #echo "         MAIN_PRJ_DIR=[$MAIN_PRJ_DIR]"
}
###############################################################################
#Begin here
#python $CODING_STYLE $1 $2 check_empty
if [ "$#" -eq "0" ]; then
    show_usage
    exit 1
fi

# parsing arguments
declare -a argv=($0)
ori_argv=$@
do_make_clean="none"
for i in $@
do
    case $i in
        -o=*|--option=*)
            opt=" ${i#*=}"
            echo "UE BUILD OPTION:$opt"
            echo "$opt" | grep -q -E " OUT="
            if [[ $? -eq 0 ]]; then
                OUT=`echo $opt | grep -o "OUT=[^ |^ ]*" | cut -d '=' -f2 | tr -d ' '`
                if [ -z "$OUT" ]; then
                    echo "Error: -o=OUT= cannot be empty!"
                    show_usage
                    exit 1
                fi
                OUT=$PWD/$OUT
                use_new_out_folder="true"
                echo "output folder change to: $OUT"
            fi
            if [[ "$opt" =~ " FEATURE=" ]]; then
                feature_mk_o="${opt#*=}"
                shift
                continue
            fi
            if [[ "$opt" =~ " BL_FEATURE=" ]]; then
                bl_feature_mk_o="${opt#*=}"
                bl_feature_mk_o_flag=1
            fi
            extra_opt+=$opt
            shift
            ;;
        -blo=*|--bloption=*)
            opt=" ${i#*=}"
            echo "$opt" | grep -q -E " OUT="
            if [[ $? -eq 0 ]]; then
                echo "Error: Unsupported -o=OUT= in [-blo|-bloption]."
                exit 1
            fi
            bl_extra_opt+=$opt
            do_make_clean="true"
            shift
            ;;
        -f=*|--feature=*)
            feature_mk="${i#*=}"
            shift
            ;;
        list)
            show_available_proj
            exit 0
            ;;
        -*)
            echo "Error: unknown parameter \"$i\""
            show_usage
            exit 1
            ;;
        *)
            argv+=($i)
            ;;
    esac
done


export PROJ_NAME=${argv[2]}
###############################################################################
if [ "${argv[3]}" == "bl" ]; then
    if [ "${argv[4]}" == "release" ]; then
        cfgmode="release"
    elif [ "${argv[4]}" == "mfg" ]; then
        cfgmode="mfg"
    elif [ "${argv[4]}" == "debug" ]; then
        cfgmode="debug"
    else
        if [ "${#argv[@]}" != "4" ]; then
           show_usage
           exit 1
        fi
    fi

    target_check ${argv[1]} ${argv[2]}
    if [ "$?" -ne "0" ]; then
        echo "Error: ${argv[1]} ${argv[2]} is not available board & project"
        show_usage
        exit 1
    fi
    if [ -f "project/${argv[1]}/apps/bootloader/build_lk.sh" ]; then
        echo "little kernel bootloader"
        sh project/${argv[1]}/apps/bootloader/build_lk.sh project/${argv[1]}/apps/bootloader/ ${argv[1]} ${argv[2]}
        OUT="$PWD/out"
        target_check ${argv[1]} ${argv[2]}
        if [ "$?" -ne "0" ]; then
            echo "Error: ${argv[1]} ${argv[2]} is not available board & project or module"
            show_usage
            exit 1
        fi

        mingw_check
        where_to_find_feature_mk=$TARGET_PATH
        if [ ! -z $feature_mk ]; then
            if [ ! -e "$TARGET_PATH/$feature_mk" ]; then
                echo "Error: cannot find $feature_mk under $TARGET_PATH."
                exit 1
            fi
            EXTRA_VAR+=" FEATURE=$feature_mk"
        else
            where_to_find_feature_mk=`grep "^TARGET_PATH\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
            if [ -z $where_to_find_feature_mk ]; then
                where_to_find_feature_mk=$TARGET_PATH
            fi
            feature_mk=`grep "^FEATURE\ *[?:]\{0,1\}=\ *" $TARGET_PATH/Makefile | cut -d '=' -f2 | tr -d ' ' | tail -1`
        fi
        echo "FEATURE = $feature_mk"

        if [ -e "$OUT/obj/$TARGET_PATH/tmp.mk" ]; then
            diff -q $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk
            if [ $? -ne 0 ]; then
                do_make_clean="true"
            fi
        fi
        if [ -e "$OUT/obj/$TARGET_PATH/extra_opts.lis" ]; then
            echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.current
            diff -q $OUT/obj/$TARGET_PATH/extra_opts.current $OUT/obj/$TARGET_PATH/extra_opts.lis
            if [ $? -ne 0 ]; then
                do_make_clean="true"
            else
                if [ $do_make_clean != "true" ]; then
                    do_make_clean="false"
                    rm -f $OUT/obj/$TARGET_PATH/extra_opts.current
                fi
            fi
        fi
        if [ $do_make_clean == "true" ]; then
            clean_out $OUT
        fi
        mkdir -p $OUT/obj/$TARGET_PATH
        echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.lis
        cp $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk
        mkdir -p $OUT/autogen
        mkdir -p $OUT/log
        echo "$0 $ori_argv" > $OUT/log/build_time.log
        echo "Start Build: "`date` >> $OUT/log/build_time.log
        copy_fw_from_prebuilt $where_to_find_feature_mk $feature_mk
        if [ ! -z $feature_mk ]; then
            EXTRA_VAR+=" FEATURE=$feature_mk"
        fi
        EXTRA_VAR+="$extra_opt"
        #echo "make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR"
        make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR 2>> $OUT/err.log
        BUILD_RESULT=$?
        mkdir -p $OUT/lib
        mv -f $OUT/*.a $OUT/lib/ 2> /dev/null
        mv -f $OUT/*.log $OUT/log/ 2> /dev/null
        echo "End Build: "`date` >> $OUT/log/build_time.log
        cat $OUT/log/build.log | grep "MODULE BUILD" >> $OUT/log/build_time.log
        if [ "$BUILD_RESULT" -eq "0" ]; then
            python $CODING_STYLE $1 $2 $3 $4
            python $CODING_STYLE $1 $2 check_build
            BUILD_RESULT=$?
            if [[ "$BUILD_RESULT" -eq "0" ]]; then
                echo "TOTAL BUILD: PASS" >> $OUT/log/build_time.log
            else
	        echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
            fi
        else
            python $DEPENDENCY_MSG $1 $2 $3  
            echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
        fi
        echo "=============================================================="
        cat $OUT/log/build_time.log
        exit $BUILD_RESULT
    else
        echo "normal bootloader"
        mingw_check

        #Keep OUTPUT Base
        OUT_Base="$OUT"

        #Check feature_mk
        feature_mk_check

        #Check release mode
        release_mode_check
        echo "Config : File=[$releaseCfg] MODE=[$cfgmode] OUT=[$OUT_Base]"

        #Keep Image/BL Extra Option
        feature_mk_base="$feature_mk"
        EXTRA_VAR_base="$EXTRA_VAR"
        extra_opt_base="$extra_opt"
        bl_extra_opt_base="$bl_extra_opt"

        for i in `echo $cfgmode | tr " " "\n"`
        do
            #Keep Original Extra Options
            feature_mk="$feature_mk_base"
            EXTRA_VAR="$EXTRA_VAR_base"
            extra_opt="$extra_opt_base"
            bl_extra_opt="$bl_extra_opt_base"

            #Release Mode Config
            if [ "$no_mcfg" != "true" ]; then
                #Release Mode Assign
                rmode=$i
                extra_opt+=" MTK_RELEASE_MODE=$rmode"
                bl_extra_opt+=" MTK_RELEASE_MODE=$rmode"
                #echo "Check: rmode = $rmode EXTRA_VAR=$EXTRA_VAR extra_opt=$extra_opt bl_extra_opt=$bl_extra_opt"

                release_mode_cfg $rmode
                OUT="$OUT_Base/$rmode"
                WIFI_RAM_CODE_MT7933_ALL="WIFI_RAM_CODE_MT7933_MSHRINK_ALL"
            else
                OUT="$OUT_Base"    #For backward compatible
                WIFI_RAM_CODE_MT7933_ALL="WIFI_RAM_CODE_MT7933_ALL"
            fi
            echo "Release: Mode=[$rmode]"
            echo "Define : FEATURE=[$feature_mk] HAL_FEATURE=[$hal_feature_mk] FLASH_LD=[$flash_ld] MEM_LD=[$mem_ld] MEM_LD_PATH=[$mem_ld_path]"
            echo "         MAIN_PRJ_DIR=[$MAIN_PRJ_DIR]"
            echo "OUT    : $OUT"

            if [ -e "$OUT/obj/$TARGET_PATH/tmp.mk" ]; then
                diff -q $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk
                if [ $? -ne 0 ]; then
                    do_make_clean="true"
                fi
            fi
            if [ -e "$OUT/obj/$TARGET_PATH/extra_opts.lis" ]; then
                echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.current
                diff -q $OUT/obj/$TARGET_PATH/extra_opts.current $OUT/obj/$TARGET_PATH/extra_opts.lis
                if [ $? -ne 0 ]; then
                    do_make_clean="true"
                else
                    if [ $do_make_clean != "true" ]; then
                        do_make_clean="false"
                        rm -f $OUT/obj/$TARGET_PATH/extra_opts.current
                    fi
                fi
            fi
            if [ $do_make_clean == "true" ]; then
                clean_out $OUT
            fi
            mkdir -p $OUT/obj/$TARGET_PATH
            echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.lis
            cp $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk

            RTOS_TARGET_PATH_BAK=$TARGET_PATH
            if [ "${argv[2]}" == "slt_rtos" ]; then
                TARGET_PATH="project/${argv[1]}/apps/slt_bootloader/GCC"
            else
                TARGET_PATH="project/${argv[1]}/apps/bootloader/GCC"
            fi

            # Check bootloader feature makefile
            if [ "$bl_feature_mk_o_flag" -eq "0" ]; then
                bl_feature_mk=`grep "^BL_FEATURE\ *[?:]\{0,1\}=\ *" $RTOS_TARGET_PATH_BAK/$feature_mk | cut -d '=' -f2 | tr -d ' ' | tail -1`
            else
                bl_feature_mk=$bl_feature_mk_o
            fi
            if [ ! -z $bl_feature_mk ]; then
                if [ ! -e "$TARGET_PATH/$bl_feature_mk" ]; then
                    echo "Error: cannot find $bl_feature_mk under $TARGET_PATH."
                    exit 1
                fi
                bl_extra_opt+=" FEATURE=$bl_feature_mk"
            fi
            echo "BL_FEATURE = $bl_feature_mk"

            # Check Project bootloader options
            bl_opt_project=`grep "^BL_OPT" $RTOS_TARGET_PATH_BAK/$feature_mk | cut -d '_' -f-2 --complement |  tr -d ' ' | tr -t '\n' ' '`
            if [ ! -z "$bl_opt_project" ]; then
                bl_extra_opt+=" $bl_opt_project"
                echo "bl_opt_project = [$bl_opt_project]"
            fi

            mkdir -p $OUT/log
            echo "$0 $ori_argv" > $OUT/log/build_time.log
            echo "Start Build: "`date` >> $OUT/log/build_time.log
            copy_fw_from_prebuilt $where_to_find_feature_mk $feature_mk
            echo "Build bootloader..."
            # Check if the source dir is existed
            if [ ! -d "project/${argv[1]}/apps/bootloader" ]; then
                echo "Error: no bootloader source in project/${argv[1]}/apps/bootloader"
                exit 1
            fi

            mkdir -p $OUT
            echo "make -C $TARGET_PATH BUILD_DIR=$OUT/obj/bootloader OUTPATH=$OUT BL_MAIN_PROJECT=${argv[2]} $bl_extra_opt"
            make -C $TARGET_PATH BUILD_DIR=$OUT/obj/bootloader OUTPATH=$OUT BL_MAIN_PROJECT=${argv[2]} $bl_extra_opt 2>> $OUT/err.log
            BUILD_RESULT=$?
            mkdir -p $OUT/lib
            mv -f $OUT/*.a $OUT/lib/ 2> /dev/null
            mkdir -p $OUT/log
            mv -f $OUT/*.log $OUT/log/ 2> /dev/null
            if [ $BUILD_RESULT -ne 0 ]; then
                echo "Error: bootloader build failed!!"
                echo "BOOTLOADER BUILD : FAIL" >> $OUT/log/build_time.log
                exit 2;
            else
                echo "BOOTLOADER BUILD : PASS" >> $OUT/log/build_time.log
            fi
            echo "Build bootloader...Done"

            # build RTOS firmware
            echo "Build RTOS Firmware..."
            TARGET_PATH=$RTOS_TARGET_PATH_BAK
            mkdir -p $OUT/autogen
            EXTRA_VAR+="$extra_opt"
            echo "make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR"
            make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR 2>> $OUT/err.log
            BUILD_RESULT=$?
            mkdir -p $OUT/lib
            mv -f $OUT/*.a $OUT/lib/ 2> /dev/null
            mkdir -p $OUT/log
            mv -f $OUT/*.log $OUT/log/ 2> /dev/null
            echo "Build RTOS Firmware...Done"
            echo "End Build: "`date` >> $OUT/log/build_time.log
            cat $OUT/log/build.log | grep "MODULE BUILD" >> $OUT/log/build_time.log
            if [ "$BUILD_RESULT" -eq "0" ]; then
                #python $CODING_STYLE $1 $2 $3 $rmode
                #python $CODING_STYLE $1 $2 check_build
                BUILD_RESULT=$?
                if [[ "$BUILD_RESULT" -eq "0" ]]; then
                    echo "TOTAL BUILD: PASS" >> $OUT/log/build_time.log
                else
	            echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
	                  exit $BUILD_RESULT
                fi
            else
                python $DEPENDENCY_MSG $1 $2 $3  
                echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
                exit $BUILD_RESULT
            fi
            echo "=============================================================="
            cat $OUT/log/build_time.log
        done
        exit $BUILD_RESULT
    fi
elif [ "${argv[3]}" == "clean" ]; then
    if [ "${#argv[@]}" != "4" ]; then
        show_usage
        exit 1
    fi
    if [ "$use_new_out_folder" == "true" ]; then
        rm -rf $OUT
    else
        rm -rf $OUT/${argv[1]}/${argv[2]}
    fi
elif [ "${argv[2]}" == "clean" ]; then
    if [ "${#argv[@]}" != "3" ]; then
        show_usage
        exit 1
    fi
    if [ "$use_new_out_folder" == "true" ]; then
        rm -rf $OUT
    else
        rm -rf $OUT/${argv[1]}
    fi
elif [ "${argv[1]}" == "clean" ]; then
    if [ "${#argv[@]}" != "2" ]; then
        show_usage
        exit 1
    fi
    rm -rf $OUT
else
    if [ "${argv[3]}" == "release" ]; then
        cfgmode="release"
    elif [ "${argv[3]}" == "mfg" ]; then
        cfgmode="mfg"
    elif [ "${argv[3]}" == "debug" ]; then
        cfgmode="debug"
    else
        if [ "${#argv[@]}" != "3" ]; then
           show_usage
           exit 1
        fi
    fi

    target_check ${argv[1]} ${argv[2]}
    if [ "$?" -ne "0" ]; then
        echo "Error: ${argv[1]} ${argv[2]} is not available board & project or module"
        show_usage
        exit 1
    fi

    mingw_check
    #Keep OUTPUT Base
    OUT_Base="$OUT"

    #Check feature_mk
    feature_mk_check

    #Check release mode
    release_mode_check
    echo "Config : File=[$releaseCfg] MODE=[$cfgmode] OUT=[$OUT_Base]"

    #Keep Image/BL Extra Option
    feature_mk_base="$feature_mk"
    EXTRA_VAR_base="$EXTRA_VAR"
    extra_opt_base="$extra_opt"
    bl_extra_opt_base="$bl_extra_opt"

    for i in `echo $cfgmode | tr " " "\n"`
    do
        #Keep Original Extra Options
        feature_mk="$feature_mk_base"
        EXTRA_VAR="$EXTRA_VAR_base"
        extra_opt="$extra_opt_base"
        bl_extra_opt="$bl_extra_opt_base"

        #Release Mode Config
        if [ "$no_mcfg" != "true" ]; then
            #Release Mode Assign
            rmode=$i
            extra_opt+=" MTK_RELEASE_MODE=$rmode"
            bl_extra_opt+=" MTK_RELEASE_MODE=$rmode"
            #echo "Check: rmode = $rmode EXTRA_VAR=$EXTRA_VAR extra_opt=$extra_opt bl_extra_opt=$bl_extra_opt"

            release_mode_cfg $rmode
            OUT="$OUT_Base/$rmode"
            WIFI_RAM_CODE_MT7933_ALL="WIFI_RAM_CODE_MT7933_MSHRINK_ALL"
        else
            OUT="$OUT_Base"    #For backward compatible
            WIFI_RAM_CODE_MT7933_ALL="WIFI_RAM_CODE_MT7933_ALL"
        fi
        echo "Release: Mode=[$rmode]"
        echo "Define : FEATURE=[$feature_mk] HAL_FEATURE=[$hal_feature_mk] FLASH_LD=[$flash_ld] MEM_LD=[$mem_ld] MEM_LD_PATH=[$mem_ld_path]"
        echo "         MAIN_PRJ_DIR=[$MAIN_PRJ_DIR]"
        echo "OUT    : $OUT"

        if [ -e "$OUT/obj/$TARGET_PATH/tmp.mk" ]; then
            diff -q $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk
            if [ $? -ne 0 ]; then
                do_make_clean="true"
            fi
        fi
        if [ -e "$OUT/obj/$TARGET_PATH/extra_opts.lis" ]; then
            echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.current
            diff -q $OUT/obj/$TARGET_PATH/extra_opts.current $OUT/obj/$TARGET_PATH/extra_opts.lis
            if [ $? -ne 0 ]; then
                do_make_clean="true"
            else
                if [ $do_make_clean != "true" ]; then
                    do_make_clean="false"
                    rm -f $OUT/obj/$TARGET_PATH/extra_opts.current
                fi
            fi
        fi
        if [ $do_make_clean == "true" ]; then
            clean_out $OUT
        fi
        mkdir -p $OUT/obj/$TARGET_PATH
        echo $extra_opt | grep -Eo  "[_[:alnum:]]+=[[:graph:]]*" | sort | uniq > $OUT/obj/$TARGET_PATH/extra_opts.lis
        cp $where_to_find_feature_mk/$feature_mk $OUT/obj/$TARGET_PATH/tmp.mk
        mkdir -p $OUT/autogen
        mkdir -p $OUT/log
        echo "$0 $ori_argv" > $OUT/log/build_time.log
        echo "Start Build: "`date` >> $OUT/log/build_time.log
        copy_fw_from_prebuilt $where_to_find_feature_mk $feature_mk
        if [ ! -z $feature_mk ]; then
            EXTRA_VAR+=" FEATURE=$feature_mk"
        fi
        EXTRA_VAR+="$extra_opt"
        echo "make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR"
        make -C $TARGET_PATH BUILD_DIR=$OUT/obj OUTPATH=$OUT $EXTRA_VAR 2>> $OUT/err.log
        BUILD_RESULT=$?
        mkdir -p $OUT/lib
        mv -f $OUT/*.a $OUT/lib/ 2> /dev/null
        mv -f $OUT/*.log $OUT/log/ 2> /dev/null
        echo "Build RTOS Firmware...Done"
        echo "End Build: "`date` >> $OUT/log/build_time.log
        cat $OUT/log/build.log | grep "MODULE BUILD" >> $OUT/log/build_time.log
        if [ "$BUILD_RESULT" -eq "0" ]; then
            #python $CODING_STYLE $1 $2 $3 $rmode
            #python $CODING_STYLE $1 $2 check_build
            BUILD_RESULT=$?
            if [[ "$BUILD_RESULT" -eq "0" ]]; then
                echo "TOTAL BUILD: PASS" >> $OUT/log/build_time.log
            else
                echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
            fi
        else
            python $DEPENDENCY_MSG $1 $2 $3  
            echo "TOTAL BUILD: FAIL" >> $OUT/log/build_time.log
        fi
        echo "=============================================================="
        cat $OUT/log/build_time.log
    done
    exit $BUILD_RESULT
fi


#!/usr/bin/env python3

import sys, time, re
from optparse import OptionParser

def to_int(s):
    unit = re.sub('([0-9x]*)', '', s)
    if unit == 'k' or unit == 'K':
        mul = 1024
    elif unit == 'm' or unit == 'M':
        mul = 1024 * 1024
    else:
        mul = 1
    return mul * int( re.sub('([kKMm])', '', s), 0 )

def eval_math(exp):
    v = 0
    op = 1
    tokens = re.split('([+-])', exp)
    for x in tokens:
        if x == '-':
            op = -1
        elif x == '+':
            op = 1
        else:
            v = v + op * to_int(x)
    return v

def memory_line_parse(line):
    p = re.compile("([a-zA-Z_]*)\([rwx]*\):ORIGIN=([0-9a-fA-FxK+-]*),LENGTH=([0-9a-fA-FxK+-]*)")
    r = p.match(line)
    if r != None:
        r = r.groups()
        r = ( r[0], eval_math(r[1]), eval_math(r[2]) )
    return r

def generate(f, partition, en='n', addr=0, len=0, fn='', rb='n'):
    if type(addr) != str:
        addr = '{0:#0{1}x}'.format(addr - 0x18000000, 10)
    if type(len) != str:
        len  = '{0:#0{1}x}'.format(len, 10)
    print('[' + partition + ']',   file=f)
    print('enable=' + en,          file=f)
    print('start_addr=' + addr,    file=f)
    print('partition_size=' + len, file=f)
    print('file_name=' + fn,       file=f)
    print('readback=' + rb,        file=f)

# bootloader is always signed, hence offset and length are
# always shifted.
def rom_bl_func(f, partition, chip, filename, addr, len, **kwargs):
    if chip == 'mt7931' or chip == 'mt7933':
        file     = 'mt7931an_bootloader-xip.sgn'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# TFM_INT is not generated, provide the erase option to user
def rom_tfm_int_func(f, partition, chip, filename, addr, len, **kwargs):
    generate(f, partition, addr = addr, len = len, fn = 'erase only')

# TFM is always signed, hence offset and length are always shifted.
def rom_tfm_func(f, partition, chip, filename, addr, len,tfm_en=0, **kwargs):
    #if chip == 'mt7931' or chip == 'mt7933':
    if tfm_en==1:
        file     = 'tfm.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# RTOS may be signed, decide offset according to suffix.
def rom_rtos_func(f, partition, chip, filename, addr, len, **kwargs):
    hdr_size = 0x80 if re.match('[a-zA-Z0-9-_]*.sgn', filename) != None else 0
    addr     = addr - hdr_size
    len      = len  + hdr_size
    if chip != '' and filename != '':
        file     = filename
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# NVDM is not generated, provide the erase option to user
def rom_nvdm_func(f, partition, chip, filename, addr, len, **kwargs):
    generate(f, partition, addr = addr, len = len, fn = 'erase only')

# DSP is not generated if dsp_en is not set
def rom_dsp_func(f, partition, chip, filename, addr, len, dsp_en = 0, **kwargs):
    if chip == 'mt7933' and dsp_en == 1:
        file = 'hifi4dsp_load.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# BT
def rom_bt_func(f, partition, chip, filename, addr, len, prj_name = '', bt_en = 0,bt_dual_en=0, **kwargs):
    if prj_name == "hqa_desense" and bt_en == 1:
        if filename.find("qfn",0)!=-1:
            file = 'BT_RAM_CODE_MT7933_2_1_hdr.bin'
        elif filename.find("bga",0)!=-1:
            file = 'BT_RAM_CODE_MT7933_1_1_hdr.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    elif bt_dual_en==1 and bt_en==1:
        file = 'BT_RAM_CODE_MT7933_1_1_hdr.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    elif bt_en == 1:
        file = 'BT_RAM_CODE_MT7933_2_1_hdr.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# ROM_WIFI_PWRTBL
def rom_wifi_pwrtbl_func(f, partition, chip, filename, addr, len,  **kwargs):
    generate(f, partition, addr = addr, len = len)

# ROM_WIFI_PATCH
def rom_wifi_patch_func(f, partition, chip, filename, addr, len, wifi_en = 0, **kwargs):
    if wifi_en:
        file = 'mt7933_patch_e1_hdr.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# ROM_WIFI_EXT
def rom_wifi_ext_func(f, partition, chip, filename, addr, len, wifi_en = 0, **kwargs):
    if wifi_en:
        file = 'WIFI_RAM_CODE_MT7933_ALL.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

# ROM_WIFI
def rom_wifi_func(f, partition, chip, filename, addr, len, wifi_en = 0, **kwargs):
    if wifi_en:
        file = 'WIFI_RAM_CODE_MT7933_APSOC.bin'
        generate(f, partition, en = 'y', addr = addr, len = len, fn = file, rb = 'y')
    else:
        generate(f, partition, addr = addr, len = len)

def rom_buffer_bin_func(f, partition, chip, filename, addr, len,qfn_en=0,bga_en=0, **kwargs):
    #if prj_name.find("bga",0)!=-1:
    if bga_en==1:
        file = 'MT7933_BGA_TDD_EEPROM.bin'
        generate(f, partition, en = 'n', addr = addr, len = len, fn = file, rb = 'n')
    #elif prj_name.find("qfn",0)!=-1:
    elif qfn_en==1:
        file = 'MT7931_QFN_TDD_EEPROM.bin'
        generate(f, partition, en = 'n', addr = addr, len = len, fn = file, rb = 'n')
    else:
        generate(f, partition, addr = addr, len = len)

# EFUSE is a special case, can't follow general rule
def otp_efuse_func(f, partition, chip, filename, addr, len, **kwargs):
    generate(f, partition, addr = '', len = '')

def search_feature_mk_define(new_line,feature_define):
    if new_line.find(feature_define,0)!=-1:
        i=new_line.find("=",0)
        new_line=new_line[i+1::].strip()
        if new_line.find("y",0)!=-1:
            return True
        else:
            return False

def start(in_file, bin_index, scatter_output_dir, prj_name, feature_mk):
    bt_en=0
    bt_dual_en=0
    wifi_en=0
    dsp_en=0
    qfn_en=0
    bga_en=0
    tfm_en=0
    if bin_index!="":
        #print("project name:",bin_index)
        if bin_index.find("ram",0)!=-1: #step1:filter ram
           #print("ram: return")
            return 0
        if bin_index.find("7931",0)!=-1: #step2: identify 7931
            ic_inx="mt7931"
        elif bin_index.find("7933",0)!=-1: #identify 7933
            ic_inx="mt7933"
        elif in_file.find("7933",0)!=-1: #identify 7933
            ic_inx="mt7933"
        elif in_file.find("7931",0)!=-1: #identify 7931
            ic_inx="mt7931"
        else:
            ic_inx=""

        if bin_index.find("al",0)!=-1: #
            bt_en=1
            wifi_en=1
            dsp_en=1
        elif bin_index.find("bt",0)!=-1: #
            bt_en=1
            wifi_en=0
            dsp_en=0
        elif bin_index.find("wf",0)!=-1: #
            bt_en=0
            wifi_en=1
            dsp_en=0
        elif bin_index.find("au",0)!=-1: #
            bt_en=0
            wifi_en=0
            dsp_en=1
        elif bin_index.find("bw",0)!=-1: #
            bt_en=1
            wifi_en=1
            dsp_en=0
        if bin_index.find("qfn_bw",0)!=-1: #
            qfn_en=1
        if bin_index.find("bga_al",0)!=-1: #
            bga_en=1
    else:
        ic_inx=""
    if feature_mk!="":
        with open(feature_mk,'r') as in_f:
            for line in in_f:
                new_line=line.strip()
                if search_feature_mk_define(new_line,"MTK_BT_FW_DL_TO_EMI_ENABLE")==True:
                        bt_dual_en=1
                if search_feature_mk_define(new_line,"MTK_TFM_ENABLE")==True:
                        tfm_en=1
    rom_func_table = {
        'ROM_BL'            : rom_bl_func,
        'ROM_TFM_INT'       : rom_tfm_int_func,
        'ROM_TFM'           : rom_tfm_func,
        'ROM_RTOS'          : rom_rtos_func,
        'ROM_NVDM'          : rom_nvdm_func,
        'ROM_DSP'           : rom_dsp_func,
        'ROM_BT'            : rom_bt_func,
        'ROM_WIFI_PWRTBL'   : rom_wifi_pwrtbl_func,
        'ROM_WIFI_PATCH'    : rom_wifi_patch_func,
        'ROM_WIFI_EXT'      : rom_wifi_ext_func,
        'ROM_WIFI'          : rom_wifi_func,
        'ROM_BUFFER_BIN'    : rom_buffer_bin_func,
        'EFUSE'             : otp_efuse_func,
    }

    #print("bt.wf.dsp",bt_en,wifi_en,dsp_en)
    f_out = open(scatter_output_dir, 'w')
    if bin_index.find("bootloader",0)!=-1: #find bootloader
        bin_index="" #no assign on RTOS partition
    with open(in_file,'r') as in_f:
        for line in in_f:
            # remove comments in a single line
            line = re.sub(r"/\*.*?\*/", " ", line)
            # remove extra spaces
            line = ''.join(line.split())
            # try to get and handle: <ROM_xxx> <origin> <length>
            tuple = memory_line_parse(line)
            if tuple is not None:
                # there is a handler function
                if tuple[0] in rom_func_table:
                    func = rom_func_table[tuple[0]]
                    func(f_out, tuple[0], ic_inx, bin_index,
                         tuple[1], tuple[2],
                         dsp_en = dsp_en, bt_en = bt_en, wifi_en = wifi_en,bt_dual_en=bt_dual_en,qfn_en=qfn_en,bga_en=bga_en,tfm_en=tfm_en,prj_name=prj_name)
                # generate default for unknown <ROM_xxx> section
                elif re.match('ROM[a-zA-Z0-9_]*', tuple[0]):
                    generate(f_out, tuple[0], addr = tuple[1], len = tuple[2])
        # EFUSE doesn't exist in linker script, generate here
        generate(f_out, 'EFUSE', addr = '', len = '',fn = "efuse.bin")
    f_out.close()

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('-f', '--file', dest='configfile', help='read ld file',metavar='FILE', default='')
    parser.add_option('-m', '--file_mk', dest='feature_mk', help='read feature_mk file',metavar='FILE', default='')
    parser.add_option('-s', '--bin_inx', dest='bin_index', help='bin name index',type='string', action='store', default='')
    parser.add_option('-o', '--scatter_out', dest='scatter_out_dir', help='scatter output dir',type='string', action='store', default='')
    parser.add_option('-p', '--prj_name', dest='prj_name', help='project name index',type='string', action='store', default='')
    options, args = parser.parse_args()
    config_file = options.configfile
    feature_mk_file=options.feature_mk
    bin_index = options.bin_index
    scatter_output_dir=options.scatter_out_dir
    prj_name= options.prj_name
    #print("bin_index:",bin_index)
    start(config_file,bin_index,scatter_output_dir,prj_name,feature_mk_file)

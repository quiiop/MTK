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

    # remove comments in a single line
    line = re.sub(r"/\*.*?\*/", " ", line)
    # remove extra spaces
    line = ''.join(line.split())

    p = re.compile("([a-zA-Z_]*)\([rwx]*\):ORIGIN=([0-9a-fA-FxK+-]*),LENGTH=([0-9a-fA-FxK+-]*)")
    r = p.match(line)
    if r != None:
        r = r.groups()
        r = ( r[0], eval_math(r[1]), eval_math(r[2]) )
    return r


def generate(f, partition, en = 'n', addr = 0, len = 0, fn = '', rb = 'n'):

    if options.scat_ini.find("no",0)!=-1:
        if not (partition=="ROM_BL" or  partition=="ROM_RTOS"  or  partition=="ROM_NVDM" or  partition=="EFUSE"):
            en="n"
            rb="n"
            fn=""
           
    if type(addr) != str:
        addr = '{0:#0{1}x}'.format(addr - 0x18000000, 10)

    if type(len) != str:
        len  = '{0:#0{1}x}'.format(len, 10)

    print('[' + partition + ']',    file=f)
    print('enable='         + en,   file=f)
    print('start_addr='     + addr, file=f)
    print('partition_size=' + len,  file=f)
    print('file_name='      + fn,   file=f)
    print('readback='       + rb,   file=f)


def generate_wrap(f, partition, addr, len, file):

    en = ['y','n'][file == '']
    if options.scat_ini.find("no",0)!=-1:
        if not (partition=="ROM_BL" or  partition=="ROM_RTOS" or  partition=="ROM_NVDM" or  partition=="EFUSE"):
            en="n"
            file=""

    generate( f, partition,
              en   = en,
              addr = addr,
              len  = len,
              fn   = file,
              rb   = en )


# bootloader is always signed, hence offset and length are
# always shifted.
def rom_bl_func(f, options, partition, addr, len, **kwargs):

    generate_wrap(f, partition, addr, len, options.bl_fw)


# TFM_INT is not generated, provide the erase option to user
def rom_tfm_int_func(f, options, partition, addr, len, **kwargs):

    generate(f, partition, addr = addr, len = len, fn = 'erase only')


# TFM is always signed, hence offset and length are always shifted.
def rom_tfm_func(f, options, partition, addr, len, tfm_en = False, **kwargs):

    generate_wrap(f, partition, addr, len, options.tfm_fw)


def rom_rtos_func(f, options, partition, addr, len, **kwargs):

    # RTOS may be signed, decide offset according to suffix.
    hdr_size = 0x80 if re.match('[a-zA-Z0-9-_]*.sgn', options.rtos_fw) != None else 0
    addr     = addr - hdr_size
    len      = len  + hdr_size

    generate_wrap(f, partition, addr, len, options.rtos_fw)


def rom_mfg_func(f, options, partition, addr, len, **kwargs):

    # RTOS may be signed, decide offset according to suffix.
    hdr_size = 0x80 if re.match('[a-zA-Z0-9-_]*.sgn', options.mfg_fw) != None else 0
    addr     = addr - hdr_size
    len      = len  + hdr_size

    generate_wrap(f, partition, addr, len, options.mfg_fw)


# NVDM is not generated, provide the erase option to user
def rom_nvdm_func(f, options, partition, addr, len, **kwargs):

    generate(f, partition, addr = addr, len = len, fn = 'erase only')


def rom_dsp_func(f, options, partition, addr, len, **kwargs):

    generate_wrap(f, partition, addr, len, options.dsp_fw)


def rom_bt_func(f, options, partition, addr, len, **kwargs):

    file = ''

    if options.prj_name == 'hqa_desense':
        if options.bt_fw.find("qfn", 0) != -1:
            file = 'BT_RAM_CODE_MT7933_2_1_hdr.bin'
        elif options.bt_fw.find("bga", 0) != -1:
            file = 'BT_RAM_CODE_MT7933_1_1_hdr.bin'
    elif options.bt_fw != '':
        file = options.bt_fw

    en = 'y' if file != '' else 'n'

    generate(f, partition, en = en, addr = addr, len = len, fn = file, rb = en)


# ROM_WIFI_PWRTBL
def rom_wifi_pwrtbl_func(f, options, partition, addr, len,  **kwargs):

    generate(f, partition, addr = addr, len = len)


# ROM_WIFI
def rom_wifi_func(f, options, partition, addr, len, **kwargs):

    generate_wrap(f, partition, addr, len, options.wifi_fw)


# ROM_WIFI_EXT
def rom_wifi_ext_func(f, options, partition, addr, len, **kwargs):

    generate_wrap(f, partition, addr, len, options.wifi_ext)


# ROM_WIFI_PATCH
def rom_wifi_patch_func(f, options, partition, addr, len, **kwargs):

    generate_wrap(f, partition, addr, len, options.wifi_pat)


def rom_buffer_bin_func(f, options, partition, addr, len, **kwargs):

    file = options.buff_bin

    generate( f, partition,
              addr = addr,
              len  = len,
              fn   = file )


# EFUSE is a special case, can't follow general rule
def otp_efuse_func(f, options, partition, addr, len, **kwargs):

    generate(f, partition, addr = '', len = '')


def generate_scatter(options):

    #print("project name:",rtos_fw)

    #if options.rtos_fw.find("ram", 0) != -1:
    #    #print("ram: return")
    #    return 0

    rom_func_table = {
        'ROM_BL'            : rom_bl_func,
        'ROM_TFM_INT'       : rom_tfm_int_func,
        'ROM_TFM'           : rom_tfm_func,
        'ROM_RTOS'          : rom_rtos_func,
        'ROM_MFG'           : rom_mfg_func,
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

    f_out = open(options.scat_ini, 'w')

    # go through lines in LD file and find lines containing memory region
    # description, parsing it to get the tuple: <ROM_xxx> <origin> <length>
    # then generate the corresponding section in scatter file
    for line in open(options.ld_file,'r'):

        tuple = memory_line_parse(line)

        if tuple is not None:
            # there is a handler function
            if tuple[0] in rom_func_table:
                func = rom_func_table[ tuple[0] ]
                func( f_out, options, tuple[0], tuple[1], tuple[2] )
            # generate default for unknown <ROM_xxx> section
            elif re.match('ROM[a-zA-Z0-9_]*', tuple[0]):
                generate(f_out, tuple[0], addr = tuple[1], len = tuple[2])
            else:
                pass # do nothing if not matching expected pattern

    # EFUSE doesn't exist in linker script, generate here
    generate(f_out, 'EFUSE', addr = '', len = '', fn = "efuse.bin")

    f_out.close()


if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option('--prj_name',
                      dest    = 'prj_name',
                      help    = 'project name index',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--ld_file',
                      dest    = 'ld_file',
                      help    = 'read ld file',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--scatter',
                      dest    = 'scat_ini',
                      help    = 'scatter output file',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--bl_fw',
                      dest    = 'bl_fw',
                      help    = 'bootloader F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--tfm_fw',
                      dest    = 'tfm_fw',
                      help    = 'TF-M F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--rtos_fw',
                      dest    = 'rtos_fw',
                      help    = 'RTOS F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--mfg_fw',
                      dest    = 'mfg_fw',
                      help    = 'MFG F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--wifi_fw',
                      dest    = 'wifi_fw',
                      help    = 'Wi-Fi F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--wifi_ext',
                      dest    = 'wifi_ext',
                      help    = 'Wi-Fi F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--wifi_pat',
                      dest    = 'wifi_pat',
                      help    = 'Wi-Fi F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--bt_fw',
                      dest    = 'bt_fw',
                      help    = 'BT F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--dsp_fw',
                      dest    = 'dsp_fw',
                      help    = 'DSP F/W file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')
    parser.add_option('--buff_bin',
                      dest    = 'buff_bin',
                      help    = 'buffer BIN file name',
                      type    = 'string',
                      action  = 'store',
                      default = '')

    options, args = parser.parse_args()

    generate_scatter( options )

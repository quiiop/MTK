#!/usr/bin/env python3
# -*- coding: utf-8 -*
from fbtool_v2p84 import *
from globals import *

  
if __name__ == '__main__':
    #windows/linux package execuctable :pyarmor pack  fbtool_v2p82.py
    #linux package(python37): python3 setup.py build_ext --inplace
    global config_list_data
    config_list_data=[]

    globals.test_DA=0

    usage="show version: --version, show help: -h"
    #parser = OptionParser(usage=usage,version="fbtool_2.83(2022/9/29)")
    parser = OptionParser(usage=usage)
    parser.add_option('-e', '', dest='EfuseFile', help='input eFuse file,eg:-e d:/your_efuse_tbl.txt',metavar='FILE',default='')
    parser.add_option('-f', '', dest='ScatterFile', help='input sactter file,eg:-f d:/your_scatter.ini',metavar='FILE',default='burn_files/scatter.ini')
    parser.add_option('-c', '', dest='configfile', help='input config file,eg:-c config.ini',metavar='FILE',default='config.ini')
    parser.add_option('-s', '', dest ='efuse_control_data', help='input 4 byte ctl_data,eg:-s 0x01003344', default='')
    parser.add_option('-r', '', dest ='run_cycle', help='set running cycle,eg:-r 3', default='1')
    parser.add_option('-o', '', dest ='command', help='order cmd,eg:-o [erase,burn,burn_o,rdback,rdback_s,rdefuse,rdefuse_c,wrefuse,cmds_table,customer] (ps:customer only on cmds_table with -s for application)',default='')
    parser.add_option('-p', '', dest='serial_port', help='set serial port,eg: winsows:-p comxx or Linux: -p /dev/ttyUSB0',type='string', action='store', default='')
    parser.add_option('-v', '--version', dest='version', help='show version', action='store_true', default=False)

    options, args = parser.parse_args()
    config_file = options.configfile
    efuse_file = options.EfuseFile
    scatter_file = options.ScatterFile
    efuse_control_data = options.efuse_control_data
    max_cycle = int(options.run_cycle)
    cmd_data = options.command

    version = options.version
    if version==True :
        print("fbtool_2.84(2022/10/5)")
        sys.exit(0)

    cmds_tbl_en=""
    if cmd_data=="cmds_table":
    #if cmd_data.find("cmds_table")!=-1:
        cmds_tbl_en="y"
    #print("efuse ctl_data:",efuse_control_data)
    serial_port = options.serial_port
    status=0
    cycle=0

    while max_cycle>cycle: ##Mp line test
        if cmds_tbl_en=="y":
            status=run_cmds_tbl("table")
            print("run cmds end",status)
        else:
            #scatter_file=scatter_file.replace("\\","/")
            #fbtool = Fbtool(config_file,efuse_file,cmd_data,"",efuse_control_data,cycle,"",serial_port)
            #fbtool.search_write_line(config_file,"File_location_dir="+get_fdir(scatter_file) ,15)
            # fbtool = Fbtool(config_file,efuse_file,cmd_data,"",efuse_control_data,cycle, "",serial_port)
            # try:
            #     status=fbtool.start(scatter_file)
            # except Exception as e:
            #     print ("try_except:",str(e))
            #     break
            #else:
            in_line=""
            if scatter_file!="burn_files/scatter.ini":
                in_line="-f "+scatter_file
            if config_file!="config.ini":
                in_line+=" -c "+config_file
            if efuse_file!="burn_files/customer_ef_tbl.txt":
                in_line+=" -e "+efuse_file
            if efuse_control_data!="":
                in_line+=" -s "+efuse_control_data
            if cmd_data!="":
                in_line+=" -o "+cmd_data
            if serial_port!="":
                in_line+=" -p "+serial_port
            status=run_cmds_tbl(in_line)

        if status!=0:
            break
        else:
            cycle=cycle+1

    # parser1 = argparse.ArgumentParser(prog="fbtool")
    # parser1.add_argument('-v','--version',action='version', version="%(prog)s_2.84(2022/09/29)")
    # parser1.parse_args(['-v'])
    # parser1.parse_args(['--version'])
    if status!=0:
        sys.exit(1)
    else:
        sys.exit(0)


#! /usr/bin/env python3
#
# Copyright 2017 Linaro Limited
# Copyright 2019 Arm Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import optparse,os
from imgtool import image_opt, imgtool_version
from imgtool.version import decode_version
from optparse import OptionParser


class BasedIntParamType():
    name = 'integer'

    def convert(self, value, param, ctx):
        try:
            if value[:2].lower() == '0x':
                return int(value[2:], 16)
            elif value[:1] == '0':
                return int(value, 8)
            return int(value, 10)
        except ValueError:
            self.fail('%s is not a valid integer' % value, param, ctx)


def hexToDoubleWords(value, chars):
    if len(value) != chars:
        raise ValueError
    hex = []
    while len(value):
        hex.append(int(value[:8], 16))
        value = value[8:]
    return hex


class sha256ParamType():
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 64)
        except ValueError:
            self.fail('%s is not a 64 digits hex number' % value, param, ctx)
            return None

class sha384ParamType():
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 96)
        except ValueError:
            self.fail('%s is not a 96 digits hex number' % value, param, ctx)
            return None

class sha512ParamType():
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 128)
        except ValueError:
            self.fail('%s is not a 128 digits hex number' % value, param, ctx)
            return None


class hex32ParamType():
    name = 'hex32'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 32)
        except ValueError:
            self.fail('%s is not a 32 digits hex number' % value, param, ctx)
            return None

def sign(options, **kargs):
    print("----sign--------")

    img = image_opt.Image(version=decode_version(options.version), header_size=int(options.header_size, 16),
                      pad_header=options.pad_header, pad=options.pad, align=int(
                          options.align, 10),
                      slot_size=int(options.slot_size, 10),
                      overwrite_only=options.overwrite_only, endian=options.endian,
                      load_addr=int(options.load_addr, 16), erased_val=options.erased_val,
                      save_enctlv=options.save_enctlv, pubkey=options.pubkey)
    img.load(options.infile)
    if options.key!="" and (os.path.exists(options.key)==True):
        pass
    else:
        options.key=None
    if options.key384!="" and (os.path.exists(options.key384)==True):
        pass
    else:
        options.key384=None
    if options.key521!=""and (os.path.exists(options.key521)==True):
        pass
    else:
        options.key521=None
    if options.to != "SIGN_ALL":
        img.create(options.key, "", "", options.to, options)
    else:
        img.create(options.key,options.key384,options.key521,options.to, options)
    img.save(options.outfile, options.hex_addr)


def imgtool():
    parser = OptionParser()
    parser.add_option('--pad_header',
                      dest='pad_header',
                      help='add pad-header to image',
                      action="store_true",
                      default=False)
    parser.add_option('--header_size',
                      dest='header_size',
                      help=' ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--load_addr',
                      dest='load_addr',
                      help=' ',
                      type='string',
                      action='store',
                      default='0x0')
    parser.add_option('--S',
                      dest='slot_size',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--to',
                      dest='to',
                      help=' SIGN_ALL:ECDSA algrithm select all:256/384/521',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--pem256',
                      dest='key',
                      help='read pem_file_ec256 file',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--pem384',
                      dest='key384',
                      help='read pem_file_ec384 file',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--pem521',
                      dest='key521',
                      help='read pem_file_ec521 file',
                      type='string',
                      action='store',
                      default='')

    parser.add_option('--infile',
                      dest='infile',
                      help='in file file name',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--outfile',
                      dest='outfile',
                      help='out file file name',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--align',
                      dest='align',
                      help='align pos',
                      type='string',
                      action='store',
                      default='1')
    parser.add_option('--v',
                      dest='version',
                      help='version ',
                      type='string',
                      action='store',
                      default='0.0.1')
    parser.add_option('--pubkey',
                      dest='pubkey',
                      help='add pubkey',
                      action="store_true",
                      default=False)
    parser.add_option('--type',
                      dest='type',
                      help='add to type img/auth/cert',
                      type='string',
                      action='store',
                      default='img')
    parser.add_option('--pad',
                      dest='pad',
                      help='add t ',
                      action="store_true",
                      default=False)
    # parser.add_option('--max_sectors',
    #                   dest='max_sectors',
    #                   help='add t ',
    #                   type='string',
    #                   action='store',
    #                   default='')
    parser.add_option('--hex_addr',
                      dest='hex_addr',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--overwrite_only',
                      dest='overwrite_only',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--endian',
                      dest='endian',
                      help='add t ',
                      type='string',
                      action='store',
                      default='little')
    parser.add_option('--erased_val',
                      dest='erased_val',
                      help='add t ',
                      type='string',
                      action='store',
                      default='0xff')
    parser.add_option('--save_enctlv',
                      dest='save_enctlv',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--encrypt',
                      dest='encrypt',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--dependencies',
                      dest='dependencies',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')

    parser.add_option('--jtag_cm33_ns_enable',
                      dest='jtag_cm33_ns_enable',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--jtag_cm33_s_enable',
                      dest='jtag_cm33_s_enable',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--jtag_dsp_enable',
                      dest='jtag_dsp_enable',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--jtag_wifi_enable',
                      dest='jtag_wifi_enable',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--jtag_bt_enable',
                      dest='jtag_bt_enable',
                      help='add t ',
                      action="store_true",
                      default=False)

    parser.add_option('--jtag_pw',
                      dest='jtag_pw',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--meid',
                      dest='meid',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--ar_ver',
                      dest='ar_ver',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--daa_pubkey',
                      dest='daa_pubkey',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--daa_pubkey384',
                      dest='daa_pubkey384',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--daa_pubkey521',
                      dest='daa_pubkey521',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')

    parser.add_option('--da_hash',
                      dest='da_hash',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--da_hash384',
                      dest='da_hash384',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--da_hash521',
                      dest='da_hash521',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')

    parser.add_option('--cust_name',
                      dest='cust_name',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    ###
    parser.add_option('--sattrs_no_cmd_read_filter',
                      dest='sattrs_no_cmd_read_filter',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sattrs_no_cmd_write_filter',
                      dest='sattrs_no_cmd_write_filter',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sattrs_cmd_filter_enable',
                      dest='sattrs_cmd_filter_enable',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--no_bootrom_log',
                      dest='no_bootrom_log',
                      help='add t ',
                      action="store_true",
                      default=False)
    ###
    parser.add_option('--asic_mpu',
                      dest='asic_mpu',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--infra_dapc',
                      dest='infra_dapc',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    parser.add_option('--aud_dapc',
                      dest='aud_dapc',
                      help='add t ',
                      type='string',
                      action='store',
                      default='')
    ###
    parser.add_option('--sctrl_cert_jtag_en',
                      dest='sctrl_cert_jtag_en',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sctrl_cert_sec_debug_en',
                      dest='sctrl_cert_sec_debug_en',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sctrl_cert_dsp_jtag_en',
                      dest='sctrl_cert_dsp_jtag_en',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--jsctrl_cert_wifi_jtag_en',
                      dest='sctrl_cert_wifi_jtag_en',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sctrl_cert_bt_jtag_en',
                      dest='sctrl_cert_bt_jtag_en',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--sctrl_cert_ar_dis',
                      dest='sctrl_cert_ar_dis',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--brom_sec_cfg_cmd_read_filter_dis',
                      dest='brom_sec_cfg_cmd_read_filter_dis',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--brom_sec_cfg_cmd_write_filter_dis',
                      dest='brom_sec_cfg_cmd_write_filter_dis',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--brom_sec_cfg_cmd_filter_en',
                      dest='brom_sec_cfg_cmd_filter_en',
                      help='add t ',
                      action="store_true",
                      default=False)

    parser.add_option('--tool_auth_bind2da',
                      dest='tool_auth_bind2da',
                      help='add t ',
                      action="store_true",
                      default=False)
    parser.add_option('--tool_auth_ar_version',
                      dest='tool_auth_ar_version',
                      help='add t ',
                      action="store_true",
                      default=False)

    options, args = parser.parse_args()
    sign(options)

if __name__ == '__main__':
    #(0)openssl version:
    #OpenSSL 1.0.1f 6 Jan 2014
    #(1)gen private key:
    #openssl ecparam -genkey -name prime256v1 -noout -out my_prv_256.pem
    #openssl ecparam -genkey -name secp384r1 -noout -out my_prv_384.pem
    #openssl ecparam -genkey -name secp521r1 -noout -out my_prv_521.pem
    #(1.1)get key bytes for signing to TLV
    #coding on image.py :get_key_bytes()

    #(2)Create public key from private key:
    #openssl ec -in private.pem -pubout -out publicKey.pem
    #(3) Sign by private key:
    #openssl dgst -sha256 -sign privateKey256.pem mt7931an_bootloader.bin > sign_data.txt
    #openssl dgst -sha384 -sign privateKey384.pem mt7931an_bootloader.bin > sign_data.txt
    #openssl dgst -sha512 -sign privateKey521.pem mt7931an_bootloader.bin > sign_data.txt
    #(3.1)get signing data for signing to TLV
    #coding on image.py :get_sign_bytes_with_verify()
    #(4)verify by public key:(local verify)
    #openssl dgst -sha256 -verify publicKey.pem -signature sign_data.txt mt7931an_bootloader.bin
    #openssl dgst -sha384 -verify publicKey.pem -signature sign_data.txt mt7931an_bootloader.bin
    #openssl dgst -sha512 -verify publicKey.pem -signature sign_data.txt mt7931an_bootloader.bin
    #(4.1)verify by public key with original image+payload:(auto verify)
    #coding on image.py :get_sign_bytes_with_verify()
    #(5)order command examples:
    #image :
    #python imgtool.py  --pad_header  --header_size 0x80 --load_addr 0x18000080 --pem256 mtk-dev.pem  --pem256 sign_prv_256.pem --pem384 sign_prv_384.pem
    #                   --pem521 sign_prv_521.pem  --infile mt7931an_bootloader.bin --outfile mt7931-xip.sgn --align 4 --v 0.0.1 --pubkey   --type img
    #                   --S 65536 --ar_ver 13 --to SIGN_ALL --asic_mpu "0x0f00,0x80000000" --infra_dapc "0x0765,0xa0000000" --aud_dapc "0x3f80,0x80000000"
    #auth
    #python imgtool.py  --pad_header --header_size 0x20  --pem256  mtk-dev.pem --pem384 sign_prv_384.pem --daa_pubkey daa_pub_256.pem  --daa_pubkey384 daa_pub_384.pem
    #                   --S 65536 --type auth --infile mt7933_auth.bin --outfile mt7933_auth.auth  --align 4 --v 0.0.1 --pubkey --cust_name aabbcc88 --tool_auth_bind2da
    #                   --da_hash 11223344556677889900aabbccddeeffffeeddccbbaa00998877665544332211
    #                   --da_hash384 1111111111111111111111111111ff0000112233445566778899aabbccddeeff112233445566778899aabbccddeeff00 --to SIGN_ALL
    #cert:
    #python imgtool.py  --pad_header --header_size 0x20  --pem256 mtk-dev.pem --pem384 sign_prv_384.pem --pem521 sign_prv_521.pem  --infile mt7933_cert.bin --S 65536
    #                   --type cert --infile mt7933_cert.bin --outfile mt7933_cert.cert --align 4 --v 0.0.1 --pubkey --cust_name aabbcc88   --meid 1122334455667788aa99bbccddeeff00
    #                   --jtag_pw 1122334455667788aa99bbccddeeff00 --jtag_cm33_ns_enable --ar_ver 15 --to SIGN_ALL
    imgtool()

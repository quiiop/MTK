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

import re,os
import click
import getpass
import imgtool.keys as keys
import sys
from imgtool import image, imgtool_version
from imgtool.version import decode_version
from .keys import RSAUsageError, ECDSAUsageError, Ed25519UsageError


def gen_rsa2048(keyfile, passwd):
    keys.RSA.generate().export_private(path=keyfile, passwd=passwd)


def gen_rsa3072(keyfile, passwd):
    keys.RSA.generate(key_size=3072).export_private(path=keyfile,
                                                    passwd=passwd)


def gen_ecdsa_p256(keyfile, passwd):
    keys.ECDSA256P1.generate().export_private(keyfile, passwd=passwd)


def gen_ecdsa_p384(keyfile, passwd):
    keys.ECDSA384R1.generate().export_private(keyfile, passwd=passwd)


def gen_ecdsa_p521(keyfile, passwd):
    keys.ECDSA521R1.generate().export_private(keyfile, passwd=passwd)


def gen_ecdsa_p224(keyfile, passwd):
    print("TODO: p-224 not yet implemented")


def gen_ed25519(keyfile, passwd):
    keys.Ed25519.generate().export_private(path=keyfile, passwd=passwd)


valid_langs = ['c', 'rust']
keygens = {
    'rsa-2048':   gen_rsa2048,
    'rsa-3072':   gen_rsa3072,
    'ecdsa-p256': gen_ecdsa_p256,
    'ecdsa-p384': gen_ecdsa_p384,
    'ecdsa-p521': gen_ecdsa_p521,
    'ecdsa-p224': gen_ecdsa_p224,
    'ed25519': gen_ed25519,
}


def load_key(keyfile):
    # TODO: better handling of invalid pass-phrase
    #print("load key", keyfile)

    #pwd=os.popen("pwd").read()
    #print("pwd--", pwd,keyfile)
    cmd="openssl ec -in "+keyfile+" -pubout -outform der -out temp.der"
    os.system(cmd)
    if os.path.exists("temp.der"):
        with open("temp.der",'rb') as f:
            key=f.read()
        os.remove("temp.der")
        if len(key)==91:
            type="SBC_PUBK_HASH_256_FBTool_customer_ef_tbl.txt"
            cmd="openssl pkey -inform PEM -in "+keyfile+" -pubout -outform DER | sha256sum"
        elif len(key)==120:
            type="SBC_PUBK_HASH_384_FBTool_customer_ef_tbl.txt"
            cmd="openssl pkey -inform PEM -in "+keyfile+" -pubout -outform DER | sha384sum"
        elif len(key)==158:
            type="SBC_PUBK_HASH_512_FBTool_customer_ef_tbl.txt"
            cmd="openssl pkey -inform PEM -in "+keyfile+" -pubout -outform DER | sha512sum"

        re_val=os.popen(cmd).read()
        i=re_val.find("-",0)
        re_val=re_val[0:i].strip()
        for i in range(0,len(re_val),8) :
            if i==0:
                re_sort="[SBC_PUBK_HASH0]"
                re_sort+="\ndata ="
            elif i==64:
                j=re_sort.rfind(",",0)
                re_sort=re_sort[0:j]
                re_sort+="\n[SBC_PUBK_HASH1]"
                re_sort+="\ndata ="
            re_sort+=(re_val[i+6:i+8]+","+re_val[i+4:i+6]+"," +re_val[i+2:i+4]+"," +re_val[i:i+2])+","
        i=re_sort.rfind(",",0)
        re_sort=re_sort[0:i]
        #notice: below path is different from caller
        if os.path.exists("../../../../../out/mt7933_hdk/"): #be called from SDK: projects/.../bootloader/GCC/makefile
            txt_f=open("../../../../../out/mt7933_hdk/"+type,"w")
            txt_f.write(re_sort)
        elif os.path.exists("../../../../out/mt7933_hdk/"):#be called from SDK: middleware/third_party/tfm/build
            txt_f=open("../../../../out/mt7933_hdk/"+type,"w")
            txt_f.write(re_sort)
        elif os.path.exists("../../out/mt7933_hdk/"):#be called from SDK: tools/mcuboot/script
            txt_f=open("../../out/mt7933_hdk/"+type,"w")
            txt_f.write(re_sort)


    key = keys.load(keyfile)
    if key is not None:
        return key
    passwd = getpass.getpass("Enter key passphrase: ").encode('utf-8')
    return keys.load(keyfile, passwd)


def get_password():
    while True:
        passwd = getpass.getpass("Enter key passphrase: ")
        passwd2 = getpass.getpass("Reenter passphrase: ")
        if passwd == passwd2:
            break
        print("Passwords do not match, try again")

    # Password must be bytes, always use UTF-8 for consistent
    # encoding.
    return passwd.encode('utf-8')


@click.option('-p', '--password', is_flag=True,
              help='Prompt for password to protect key')
@click.option('-t', '--type', metavar='type', required=True,
              type=click.Choice(keygens.keys()), prompt=True,
              help='{}'.format('One of: {}'.format(', '.join(keygens.keys()))))
@click.option('-k', '--key', metavar='filename', required=True)
@click.command(help='Generate pub/private keypair')
def keygen(type, key, password):
    #print("keygen")
    password = get_password() if password else None
    keygens[type](key, password)


@click.option('-l', '--lang', metavar='lang', default=valid_langs[0],
              type=click.Choice(valid_langs))
@click.option('-k', '--key', metavar='filename', required=True)
@click.command(help='Dump public key from keypair')
def getpub(key, lang):
    #print("getpub", key)
    key = load_key(key)
    if key is None:
        print("Invalid passphrase")
    elif lang == 'c':
        key.emit_c_public()
    elif lang == 'rust':
        key.emit_rust_public()
    else:
        raise ValueError("BUG: should never get here!")


@click.option('--minimal', default=False, is_flag=True,
              help='Reduce the size of the dumped private key to include only '
                   'the minimum amount of data required to decrypt. This '
                   'might require changes to the build config. Check the docs!'
              )
@click.option('-k', '--key', metavar='filename', required=True)
@click.command(help='Dump private key from keypair')
def getpriv(key, minimal):
    key = load_key(key)
    if key is None:
        print("Invalid passphrase")
    try:
        key.emit_private(minimal)
    except (RSAUsageError, ECDSAUsageError, Ed25519UsageError) as e:
        raise click.UsageError(e)


@click.argument('imgfile')
@click.option('-k', '--key', metavar='filename')
@click.command(help="Check that signed image can be verified by given key")
def verify(key, imgfile):
    key = load_key(key) if key else None
    ret, version = image.Image.verify(imgfile, key)
    if ret == image.VerifyResult.OK:
        print("Image was correctly validated")
        print("Image version: {}.{}.{}+{}".format(*version))
        return
    elif ret == image.VerifyResult.INVALID_MAGIC:
        print("Invalid image magic; is this an MCUboot image?")
    elif ret == image.VerifyResult.INVALID_TLV_INFO_MAGIC:
        print("Invalid TLV info magic; is this an MCUboot image?")
    elif ret == image.VerifyResult.INVALID_HASH:
        print("Image has an invalid digest")
    elif ret == image.VerifyResult.INVALID_SIGNATURE:
        print("No signature found for the given key")
    else:
        print("Unknown return code: {}".format(ret))
    sys.exit(1)


def validate_version(ctx, param, value):
    try:
        decode_version(value)
        return value
    except ValueError as e:
        raise click.BadParameter("{}".format(e))


def validate_header_size(ctx, param, value):
    min_hdr_size = image.IMAGE_HEADER_SIZE
    if value < min_hdr_size:
        raise click.BadParameter(
            "Minimum value for -H/--header-size is {}".format(min_hdr_size))
    return value


def get_dependencies(ctx, param, value):
    if value is not None:
        versions = []
        images = re.findall(r"\((\d+)", value)
        if len(images) == 0:
            raise click.BadParameter(
                "Image dependency format is invalid: {}".format(value))
        raw_versions = re.findall(r",\s*([0-9.+]+)\)", value)
        if len(images) != len(raw_versions):
            raise click.BadParameter(
                '''There's a mismatch between the number of dependency images
                and versions in: {}'''.format(value))
        for raw_version in raw_versions:
            try:
                versions.append(decode_version(raw_version))
            except ValueError as e:
                raise click.BadParameter("{}".format(e))
        dependencies = dict()
        dependencies[image.DEP_IMAGES_KEY] = images
        dependencies[image.DEP_VERSIONS_KEY] = versions
        return dependencies


class BasedIntParamType(click.ParamType):
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


class sha256ParamType(click.ParamType):
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 64)
        except ValueError:
            self.fail('%s is not a 64 digits hex number' % value, param, ctx)
            return None

class sha384ParamType(click.ParamType):
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 96)
        except ValueError:
            self.fail('%s is not a 96 digits hex number' % value, param, ctx)
            return None

class sha512ParamType(click.ParamType):
    name = 'hex64'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 128)
        except ValueError:
            self.fail('%s is not a 128 digits hex number' % value, param, ctx)
            return None


class hex32ParamType(click.ParamType):
    name = 'hex32'

    def convert(self, value, param, ctx):
        try:
            return hexToDoubleWords(value, 32)
        except ValueError:
            self.fail('%s is not a 32 digits hex number' % value, param, ctx)
            return None


class hex8ParamType(click.ParamType):
    name = 'hex8'

    def convert(self, value, param, ctx):
        try:
            if len(value) != 8:
                raise ValueError
            return [int(value[0:2], 16), int(value[2:4], 16),
                    int(value[4:6], 16), int(value[6:8], 16)]
        except ValueError:
            self.fail('%s is not a 8 digits hex number' % value, param, ctx)
            return None


class avpParamType(click.ParamType):
    name = 'address-value pairs'

    def convert(self, value, param, ctx):
        try:
            payload = []
            for s in value.split():
                s = s.split(',')
                payload.append([int(s[0], base=16), int(s[1], base=16)])
            return payload
        except ValueError:
            self.fail('%s is not a valid address-value pair list' %
                      value, param, ctx)
            return None


class arVerParamType(click.ParamType):
    name = '1-64'

    def convert(self, value, param, ctx):
        try:
            ver = int(value, 10)
            if ver < 1 or ver > 64:
                raise ValueError
            return ver
        except ValueError:
            self.fail('%s is not a valid version (1~64)' % value, param, ctx)
            return None


def mtk_param_process(**kargs):

    opts = image.ImageOption(kargs['cert'], kargs['auth'])

    # JTAGEN & JTAGPW
    opts.jtag_cm33_ns_enable = kargs['jtag_cm33_ns_enable']
    opts.jtag_cm33_s_enable = kargs['jtag_cm33_s_enable']
    opts.jtag_dsp_enable = kargs['jtag_dsp_enable']
    opts.jtag_wifi_enable = kargs['jtag_wifi_enable']
    opts.jtag_bt_enable = kargs['jtag_bt_enable']
    opts.jtag_pw = kargs['jtag_pw']

    # MEID
    opts.meid = kargs['meid']

    # AR_VER
    opts.ar_ver = kargs['ar_ver']

    # DAA_PUBKEY
    if kargs['daa_pubkey'] != None:
        opts.daa_pubkey = load_key(kargs['daa_pubkey'])
    if kargs['daa_pubkey384'] != None:
        opts.daa_pubkey384 = load_key(kargs['daa_pubkey384'])
    if kargs['daa_pubkey521'] != None:
        opts.daa_pubkey521 = load_key(kargs['daa_pubkey521'])
    # DA_HASH
    opts.da_hash = kargs['da_hash']
    print("-------da hash", opts.da_hash)
    opts.da_hash384 = kargs['da_hash384']
    print("-------da hash384", opts.da_hash384)
    opts.da_hash521 = kargs['da_hash521']
    print("-------da hash521", opts.da_hash521)
    # CUST_NAME
    opts.cust_name = kargs['cust_name']

    # BL_SECURE_ATTR
    opts.sattrs_no_cmd_read_filter = kargs['no_cmd_read_filter']
    opts.sattrs_no_cmd_write_filter = kargs['no_cmd_write_filter']
    opts.sattrs_cmd_filter_enable = kargs['cmd_filter_enable']
    opts.sattrs_no_bootrom_log = kargs['no_bootrom_log']

    # SMPU / DAPC / AUD_DAPC
    opts.infra_dapc = kargs['infra_dapc']
    opts.aud_dapc = kargs['aud_dapc']
    opts.asic_mpu = kargs['asic_mpu']

    # SCTRL_CERT_ATTR
    opts.sctrl_cert_jtag_en = kargs['sctrl_cert_jtag_en']
    opts.sctrl_cert_sec_debug_en = kargs['sctrl_cert_sec_debug_en']
    opts.sctrl_cert_dsp_jtag_en = kargs['sctrl_cert_dsp_jtag_en']
    opts.sctrl_cert_wifi_jtag_en = kargs['sctrl_cert_wifi_jtag_en']
    opts.sctrl_cert_bt_jtag_en = kargs['sctrl_cert_bt_jtag_en']
    opts.sctrl_cert_ar_dis = kargs['sctrl_cert_ar_dis']
    opts.brom_sec_cfg_cmd_read_filter_dis = kargs['brom_sec_cfg_cmd_read_filter_dis']
    opts.brom_sec_cfg_cmd_write_filter_dis = kargs['brom_sec_cfg_cmd_write_filter_dis']
    opts.brom_sec_cfg_cmd_filter_en = kargs['brom_sec_cfg_cmd_filter_en']

    # TOOL_AUTH_ATTR
    opts.tool_auth_bind2da = kargs['tool_auth_bind2da']
    opts.tool_auth_ar_version = kargs['tool_auth_ar_version']

    return opts


@click.argument('outfile')
@click.argument('infile')
@click.option('-R', '--erased-val', type=click.Choice(['0', '0xff']),
              required=False,
              help='The value that is read back from erased flash.')
@click.option('-x', '--hex-addr', type=BasedIntParamType(), required=False,
              help='Adjust address in hex output file.')
@click.option('-L', '--load-addr', type=BasedIntParamType(), required=False,
              help='Load address for image when it is in its primary slot.')
@click.option('--save-enctlv', default=False, is_flag=True,
              help='When upgrading, save encrypted key TLVs instead of plain '
                   'keys. Enable when BOOT_SWAP_SAVE_ENCTLV config option '
                   'was set.')
@click.option('-E', '--encrypt', metavar='filename',
              help='Encrypt image using the provided public key')
@click.option('-e', '--endian', type=click.Choice(['little', 'big']),
              default='little', help="Select little or big endian")
@click.option('--overwrite-only', default=False, is_flag=True,
              help='Use overwrite-only instead of swap upgrades')
@click.option('-M', '--max-sectors', type=int,
              help='When padding allow for this amount of sectors (defaults '
                   'to 128)')
@click.option('--pad', default=False, is_flag=True,
              help='Pad image to --slot-size bytes, adding trailer magic')
@click.option('-S', '--slot-size', type=BasedIntParamType(), required=True,
              help='Size of the slot where the image will be written')
@click.option('--pad-header', default=False, is_flag=True,
              help='Add --header-size zeroed bytes at the beginning of the '
                   'image')
@click.option('-H', '--header-size', callback=validate_header_size,
              type=BasedIntParamType(), required=True)
@click.option('-d', '--dependencies', callback=get_dependencies,
              required=False, help='''Add dependence on another image, format:
              "(<image_ID>,<image_version>), ... "''')
@click.option('-v', '--version', callback=validate_version,  required=True)
@click.option('--align', type=click.Choice(['1', '2', '4', '8']),
              required=True)
@click.option('-k', '--key', metavar='filename')
@click.option('-k384', '--key384', metavar='filename')
@click.option('-k521', '--key521', metavar='filename')
@click.option('-p', '--pubkey', default=False, is_flag=True,
              help='Add the public-key to image. If not specified, add the '
                   'hash of the public-key to image')
@click.option('--cert', help='generate cert file',
              default=False, is_flag=True)
@click.option('--auth', help='generate auth file',
              default=False, is_flag=True)
@click.option('--jtag-cm33-ns-enable', help='enable cm33 jtag',
              default=False, is_flag=True)
@click.option('--jtag-cm33-s-enable', help='enable cortex-m33 secure jtag',
              default=False, is_flag=True)
@click.option('--jtag-dsp-enable', help='enable DSP jtag',
              default=False, is_flag=True)
@click.option('--jtag-wifi-enable', help='enable Wi-Fi jtag',
              default=False, is_flag=True)
@click.option('--jtag-bt-enable', help='enable Bluetooth jtag',
              default=False, is_flag=True)
@click.option('--jtag-pw', help='jtag password (32 hex digits)',
              type=hex32ParamType())
@click.option('--meid', help='MEID (128-bits)',
              type=hex32ParamType())
@click.option('--ar-ver', help='anti-rollback version, 1~64',
              type=arVerParamType())
@click.option('-daa', '--daa-pubkey', help='DA authentication file',
              metavar='filename')
@click.option('-daa384', '--daa-pubkey384', help='DA authentication file',
              metavar='filename')
@click.option('-daa521', '--daa-pubkey521', help='DA authentication file',
              metavar='filename')
@click.option('--da-hash', help='hash value of the DA image (64 hex digits)',
              type=sha256ParamType())
@click.option('--da-hash384', help='hash value of the DA image (96 hex digits)',
              type=sha384ParamType())
@click.option('--da-hash521', help='hash value of the DA image (128 hex digits)',
              type=sha512ParamType())
@click.option('--cust-name', help='custom name (8 hex digits)',
              type=hex8ParamType())
@click.option('--no-cmd-read-filter', help='disable command read filter',
              default=False, is_flag=True)
@click.option('--no-cmd-write-filter', help='disable command write filter',
              default=False, is_flag=True)
@click.option('--cmd-filter-enable', help='enable command filter',
              default=False, is_flag=True)
@click.option('--no-bootrom-log', help='disable ROM log',
              default=False, is_flag=True)
@click.option('--infra-dapc', help='add infra dapc register addr-value pairs',
              type=avpParamType(), required=False)
@click.option('--aud-dapc', help='add aurio dapc register addr-value pairs',
              type=avpParamType(), required=False)
@click.option('--asic-mpu', help='add asic_mpu register addr-value pairs',
              type=avpParamType(), required=False)
@click.option('--sctrl-cert-jtag-en', help='',
              default=False, is_flag=True)
@click.option('--sctrl-cert-sec-debug-en', help='',
              default=False, is_flag=True)
@click.option('--sctrl-cert-dsp-jtag-en', help='',
              default=False, is_flag=True)
@click.option('--sctrl-cert-wifi-jtag-en', help='',
              default=False, is_flag=True)
@click.option('--sctrl-cert-bt-jtag-en', help='',
              default=False, is_flag=True)
@click.option('--sctrl-cert-ar-dis', help='',
              default=False, is_flag=True)
@click.option('--brom-sec-cfg-cmd-read-filter-dis', help='',
              default=False, is_flag=True)
@click.option('--brom-sec-cfg-cmd-write-filter-dis', help='',
              default=False, is_flag=True)
@click.option('--brom-sec-cfg-cmd-filter-en', help='',
              default=False, is_flag=True)
@click.option('--tool-auth-bind2da', help='',
              default=False, is_flag=True)
@click.option('--tool-auth-ar-version', help='',
              required=False, type=arVerParamType())
@click.option('-t', '--to', help='SIGN_ALL:signature all EDCSA algrithm p256/384/521', default='', show_default=True)
@click.command(help='''Create a signed or unsigned image\n
               INFILE and OUTFILE are parsed as Intel HEX if the params have
               .hex extension, otherwise binary format is used''')
def sign(key, key384, key521, align, version, header_size, pad_header, slot_size, pad,
         max_sectors, overwrite_only, endian, encrypt, infile, outfile, to,
         dependencies, load_addr, hex_addr, erased_val, save_enctlv,
         pubkey, **kargs):
    #print("----sign to--------", to)
    opts = mtk_param_process(**kargs)
    print("----header size--------", version,
          header_size, pad_header, slot_size, pad, max_sectors, hex_addr)
    img = image.Image(version=decode_version(version), header_size=header_size,
                      pad_header=pad_header, pad=pad, align=int(align),
                      slot_size=slot_size, max_sectors=max_sectors,
                      overwrite_only=overwrite_only, endian=endian,
                      load_addr=load_addr, erased_val=erased_val,
                      save_enctlv=save_enctlv, pubkey=pubkey)
    img.load(infile)
    key = load_key(key) if key else None
    key384 = load_key(key384) if key384 else None
    key521 = load_key(key521) if key521 else None
    enckey = load_key(encrypt) if encrypt else None
    if to != "SIGN_ALL":
        if enckey and key:  # SDK no use
            print("---enc ")
            if ((isinstance(key, keys.ECDSA256P1) and
                not isinstance(enckey, keys.ECDSA256P1Public))
                    or (isinstance(key, keys.RSA) and
                        not isinstance(enckey, keys.RSAPublic))
                or (isinstance(key, key.ECDSA384R1) and
                    not isinstance(enckey, keys.ECDSA384R1Public))
                or (isinstance(key, key.ECDSA521R1) and
                    not isinstance(enckey, keys.ECDSA521R1Public))):
                # FIXME
                raise click.UsageError("Signing and encryption must use the same "
                                       "type of key")
        img.create(key, "", "", enckey, dependencies, to, opts)
    else:
        img.create(key, key384, key521, enckey, dependencies, to, opts)
    img.save(outfile, hex_addr)


class AliasesGroup(click.Group):

    _aliases = {
        "create": "sign",
    }

    def list_commands(self, ctx):
        cmds = [k for k in self.commands]
        aliases = [k for k in self._aliases]
        return sorted(cmds + aliases)

    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        if cmd_name in self._aliases:
            return click.Group.get_command(self, ctx, self._aliases[cmd_name])
        return None


@click.command(help='Print imgtool version information')
def version():
    print(imgtool_version)


@click.command(cls=AliasesGroup,
               context_settings=dict(help_option_names=['-h', '--help']))
def imgtool():
    pass


imgtool.add_command(keygen)
imgtool.add_command(getpub)
imgtool.add_command(getpriv)
imgtool.add_command(verify)
imgtool.add_command(sign)
imgtool.add_command(version)


if __name__ == '__main__':
    imgtool()

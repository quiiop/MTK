# Copyright 2018 Nordic Semiconductor ASA
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

"""
Image signing and management.
"""

from ast import For
from . import version as versmod
import click
from enum import Enum
from intelhex import IntelHex
import hashlib
import struct
import os.path
import datetime
from .keys import rsa, ecdsa
from cryptography.hazmat.primitives.asymmetric import ec, padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, hmac
from cryptography.exceptions import InvalidSignature

IMAGE_MAGIC = 0xbd9fc018
IMAGE_HEADER_SIZE = 32
BIN_EXT = "bin"
INTEL_HEX_EXT = "hex"
DEFAULT_MAX_SECTORS = 128
MAX_ALIGN = 8
DEP_IMAGES_KEY = "images"
DEP_VERSIONS_KEY = "versions"

# Image header flags.
IMAGE_F = {
    'PIC':                   0x0000001,
    'NON_BOOTABLE':          0x0000010,
    'ENCRYPTED':             0x0000004,
    'SCTRL_CERT':            0x0000100,
    'TOOL_AUTH':             0x0000200,
}

TLV_VALUES = {
    'KEYHASH': 0x01,
    'SHA256': 0x10,
    'RSA2048': 0x20,
    'ECDSA224': 0x21,
    'ECDSA256': 0x22,
    'RSA3072': 0x23,
    'ED25519': 0x24,
    'ENCRSA2048': 0x30,
    'ENCKW128': 0x31,
    'ENCEC256': 0x32,
    'DEPENDENCY': 0x40,
    'ECDSA384': 0x50,
    'ECDSA521': 0x51,
    'SHA384': 0x55,
    'SHA512': 0x56,
    'PUBKEY_EC384': 0x59,
    'PUBKEY_EC521': 0x5a,
    'PUBKEY_EC256': 0x60, # formerly called SBCKEY
    'JTAGEN': 0x61,
    'JTAGPW': 0x62,
    'MEID': 0x63,
    'AR_VER': 0x64,
    'DAA_PUBKEY_EC256': 0x65,
    'DA_HASH_SHA256': 0x66,
    'CUST_NAME': 0x67,
    'DAA_PUBKEY_EC384': 0x68,
    'DAA_PUBKEY_EC521': 0x69,
    'DA_HASH_SHA384': 0x6a,
    'DA_HASH_SHA521': 0x6b,
    'BL_SECURE_ATTR': 0x71,
    'ASIC_MPU': 0x72,
    'INFRA_DAPC': 0x73,
    'AUDIO_DAPC': 0x74,
    'SCTRL_CERT_ATTR': 0x80,
    'TOOL_AUTH_ATTR': 0x90
}

TLV_SIZE = 4
TLV_INFO_SIZE = 4
TLV_INFO_MAGIC = 0x5676
TLV_PROT_INFO_MAGIC = 0x5677

boot_magic = bytes([
    0x77, 0xc2, 0x95, 0xf3,
    0x60, 0xd2, 0xef, 0x7f,
    0x35, 0x52, 0x50, 0x0f,
    0x2c, 0xb6, 0x79, 0x80, ])

STRUCT_ENDIAN_DICT = {
    'little': '<',
    'big':    '>'
}

VerifyResult = Enum('VerifyResult',
                    """
                    OK INVALID_MAGIC INVALID_TLV_INFO_MAGIC INVALID_HASH
                    INVALID_SIGNATURE
                    """)


class TLV():
    def __init__(self, endian, magic=TLV_INFO_MAGIC):
        self.magic = magic
        self.buf = bytearray()
        self.endian = endian

    def __len__(self):
        return TLV_INFO_SIZE + len(self.buf)

    def add(self, kind, payload):
        """
        Add a TLV record.  Kind should be a string found in TLV_VALUES above.
        """
        e = STRUCT_ENDIAN_DICT[self.endian]
        buf = struct.pack(e + 'BBH', TLV_VALUES[kind], 0, len(payload))
        self.buf += buf
        self.buf += payload
        #print("TLV ADD", kind, payload.hex())

    def get(self):
        if len(self.buf) == 0:
            return bytes()
        e = STRUCT_ENDIAN_DICT[self.endian]
        header = struct.pack(e + 'HH', self.magic, len(self))
        return header + bytes(self.buf)


IMAGE_TYPE_FIRMWARE = 1
IMAGE_TYPE_AUTH = 2
IMAGE_TYPE_CERT = 3

"""
    ImageOption keeps arbitrary options.

    If any option (attribute) is queried but not defined, it returns None
    instead of raising an exception.
"""


class ImageOption:

    def __init__(self, cert, auth):
        if cert:
            self.type = IMAGE_TYPE_CERT
        elif auth:
            self.type = IMAGE_TYPE_AUTH
        else:
            self.type = IMAGE_TYPE_FIRMWARE

    """ any unset option is set to None """

    def __getattr__(self, name):
        if name not in self.__dict__:
            self.name = None
        return self.name


class Image():

    def __init__(self, version=None, header_size=IMAGE_HEADER_SIZE,
                 pad_header=False, pad=False, align=1, slot_size=0,
                 max_sectors=DEFAULT_MAX_SECTORS, overwrite_only=False,
                 endian="little", load_addr=0, erased_val=None,
                 save_enctlv=False, pubkey=False):
        self.version = version or versmod.decode_version("0")
        self.header_size = header_size
        self.pad_header = pad_header
        self.pad = pad
        self.align = align
        self.slot_size = slot_size
        self.max_sectors = max_sectors
        self.overwrite_only = overwrite_only
        self.endian = endian
        self.base_addr = None
        self.load_addr = 0 if load_addr is None else load_addr
        self.erased_val = 0xff if erased_val is None else int(erased_val, 0)
        self.payload = []
        self.enckey = None
        self.save_enctlv = save_enctlv
        self.enctlv_len = 0
        self.pubkey = pubkey
        #print("----image init--------", self.pubkey)

    def __repr__(self):
        return "<Image version={}, header_size={}, base_addr={}, load_addr={}, \
                align={}, slot_size={}, max_sectors={}, overwrite_only={}, \
                endian={} format={}, payloadlen=0x{:x}>".format(
            self.version,
            self.header_size,
            self.base_addr if self.base_addr is not None else "N/A",
            self.load_addr,
            self.align,
            self.slot_size,
            self.max_sectors,
            self.overwrite_only,
            self.endian,
            self.__class__.__name__,
            len(self.payload))

    def load(self, path):
        """Load an image from a given file"""
        ext = os.path.splitext(path)[1][1:].lower()
        try:
            if ext == INTEL_HEX_EXT:
                #print("----------intel hex")
                ih = IntelHex(path)
                self.payload = ih.tobinarray()
                self.base_addr = ih.minaddr()
            else:
                #print("----------non-intel hex")
                with open(path, 'rb') as f:
                    self.payload = f.read()
        except FileNotFoundError:
            raise click.UsageError("Input file not found")

        # Add the image header if needed.
        if self.pad_header and self.header_size > 0:
            if self.base_addr:
                # Adjust base_addr for new header
                self.base_addr -= self.header_size
            self.payload = bytes([self.erased_val] * self.header_size) + \
                self.payload

        self.check_header()

    def save(self, path, hex_addr=None):
        """Save an image from a given file"""
        ext = os.path.splitext(path)[1][1:].lower()
        if ext == INTEL_HEX_EXT:
            # input was in binary format, but HEX needs to know the base addr
            if self.base_addr is None and hex_addr is None:
                raise click.UsageError("No address exists in input file "
                                       "neither was it provided by user")
            h = IntelHex()
            if hex_addr is not None:
                self.base_addr = hex_addr
            h.frombytes(bytes=self.payload, offset=self.base_addr)
            if self.pad:
                trailer_size = self._trailer_size(self.align, self.max_sectors,
                                                  self.overwrite_only,
                                                  self.enckey,
                                                  self.save_enctlv,
                                                  self.enctlv_len)
                trailer_addr = (self.base_addr + self.slot_size) - trailer_size
                padding = bytes([self.erased_val] *
                                (trailer_size - len(boot_magic))) + boot_magic
                h.puts(trailer_addr, padding)
            h.tofile(path, 'hex')
        else:
            if self.pad:
                self.pad_to(self.slot_size)
            with open(path, 'wb') as f:
                f.write(self.payload)

    def check_header(self):
        if self.header_size > 0 and not self.pad_header:
            if any(v != 0 for v in self.payload[0:self.header_size]):
                raise click.UsageError("Header padding was not requested and "
                                       "image does not start with zeros")

    def check_trailer(self):
        if self.slot_size > 0:
            tsize = self._trailer_size(self.align, self.max_sectors,
                                       self.overwrite_only, self.enckey,
                                       self.save_enctlv, self.enctlv_len)
            padding = self.slot_size - (len(self.payload) + tsize)
            if padding < 0:
                msg = "Image size (0x{:x}) + trailer (0x{:x}) exceeds " \
                      "requested size 0x{:x}".format(
                          len(self.payload), tsize, self.slot_size)
                raise click.UsageError(msg)

    # needed for --encrypt argument, mt7933 doesn't use this option, ignore p384
    # and p521 update
    def ecies_p256_hkdf(self, enckey, plainkey):
        newpk = ec.generate_private_key(ec.SECP256R1(), default_backend())
        shared = newpk.exchange(ec.ECDH(), enckey._get_public())
        derived_key = HKDF(
            algorithm=hashes.SHA256(), length=48, salt=None,
            info=b'MCUBoot_ECIES_v1', backend=default_backend()).derive(shared)
        encryptor = Cipher(algorithms.AES(derived_key[:16]),
                           modes.CTR(bytes([0] * 16)),
                           backend=default_backend()).encryptor()
        cipherkey = encryptor.update(plainkey) + encryptor.finalize()
        mac = hmac.HMAC(derived_key[16:], hashes.SHA256(),
                        backend=default_backend())
        mac.update(cipherkey)
        ciphermac = mac.finalize()
        pubk = newpk.public_key().public_bytes(
            encoding=Encoding.X962,
            format=PublicFormat.UncompressedPoint)
        return cipherkey, ciphermac, pubk

    def create(self, key, key384, key521, enckey, dependencies, to, opts):

        self.enckey = enckey

        prot_tlv = TLV(self.endian, TLV_PROT_INFO_MAGIC)
        self.append_mtk_prot_tlv(prot_tlv, opts)

        if dependencies is not None:
            # size of a dependency TLV =
            # Header ('BBH') + Payload('IBBHI') = 16 Bytes
            dependencies_num = len(dependencies[DEP_IMAGES_KEY])

            for i in range(dependencies_num):
                e = STRUCT_ENDIAN_DICT[self.endian]
                payload = struct.pack(
                    e + 'B3x'+'BBHI',
                    int(dependencies[DEP_IMAGES_KEY][i]),
                    dependencies[DEP_VERSIONS_KEY][i].major,
                    dependencies[DEP_VERSIONS_KEY][i].minor,
                    dependencies[DEP_VERSIONS_KEY][i].revision,
                    dependencies[DEP_VERSIONS_KEY][i].build
                )
                prot_tlv.add('DEPENDENCY', payload)

        if len(prot_tlv) > TLV_INFO_SIZE:
            protected_tlv_size = len(prot_tlv)
        else:
            protected_tlv_size = 0

        # At this point the image is already on the payload, this adds
        # the header to the payload as well
        self.add_header(enckey, protected_tlv_size, opts)

        protected_tlv_off = len(self.payload)

        if len(prot_tlv) > TLV_INFO_SIZE:
            self.payload += prot_tlv.get()

        tlv = TLV(self.endian)

        if to == "SIGN_ALL":
            # sha = hashlib.sha256()
            # sha.update(self.payload)
            # digest = sha.digest()
            # tlv.add('SHA256', digest)
            if self.pubkey:
                if key is not None:
                    pub = key.get_public_bytes()
                    tlv.add('PUBKEY_EC256', pub)
                if key384 is not None:
                    pub = key384.get_public_bytes()
                    tlv.add('PUBKEY_EC384', pub)
                if key521 is not None:
                    pub = key521.get_public_bytes()
                    tlv.add('PUBKEY_EC521', pub)

                if hasattr(key, 'sign'):
                    while True:
                        sig = key.sign(bytes(self.payload))
                        if '021f' not in sig.hex():
                            break
                        else:
                            print("0x021f")
                    tlv.add("ECDSA256", sig)
                if hasattr(key384, 'sign'):
                    sig = key384.sign(bytes(self.payload))
                    tlv.add("ECDSA384", sig)
                if hasattr(key521, 'sign'):
                    sig = key521.sign(bytes(self.payload))
                    tlv.add("ECDSA521", sig)

        else:
            # Note that ecdsa wants to do the hashing itself, which means
            # we get to hash it twice.
            sha = hashlib.sha256()
            sha.update(self.payload)
            digest = sha.digest()
            tlv.add('SHA256', digest)

            if key is not None:
                pub = key.get_public_bytes()
                #print("---creat_key--", pub.hex())
                if self.pubkey:
                    tlv.add('PUBKEY_EC256', pub)
                else:
                    sha = hashlib.sha256()
                    sha.update(pub)
                    pubbytes = sha.digest()
                    tlv.add('KEYHASH', pubbytes)

                # `sign` expects the full image payload (sha256 done internally),
                # while `sign_digest` expects only the digest of the payload

                while True:
                    if hasattr(key, 'sign'):
                        sig = key.sign(bytes(self.payload))
                    else:
                        sig = key.sign_digest(digest)
                    if '021f' not in sig.hex():
                        break
                tlv.add(key.sig_tlv(), sig)
            # At this point the image was hashed + signed, we can remove the
            # protected TLVs from the payload (will be re-added later)
        if protected_tlv_off is not None:
            self.payload = self.payload[:protected_tlv_off]

        if enckey is not None:
            plainkey = os.urandom(16)

            if isinstance(enckey, rsa.RSAPublic):
                cipherkey = enckey._get_public().encrypt(
                    plainkey, padding.OAEP(
                        mgf=padding.MGF1(algorithm=hashes.SHA256()),
                        algorithm=hashes.SHA256(),
                        label=None))
                self.enctlv_len = len(cipherkey)
                tlv.add('ENCRSA2048', cipherkey)
            elif isinstance(enckey, ecdsa.ECDSA256P1Public):
                cipherkey, mac, pubk = self.ecies_p256_hkdf(enckey, plainkey)
                enctlv = pubk + mac + cipherkey
                self.enctlv_len = len(enctlv)
                tlv.add('ENCEC256', enctlv)

            nonce = bytes([0] * 16)
            cipher = Cipher(algorithms.AES(plainkey), modes.CTR(nonce),
                            backend=default_backend())
            encryptor = cipher.encryptor()
            img = bytes(self.payload[self.header_size:])
            self.payload[self.header_size:] = \
                encryptor.update(img) + encryptor.finalize()

        self.payload += prot_tlv.get()
        self.payload += tlv.get()

        self.check_trailer()

    def append_mtk_prot_tlv(self, prot_tlv, opts):
        print("START proteck MTK TLV", opts.type)
        e = STRUCT_ENDIAN_DICT[self.endian]

        """ Bootloader and Certificate file
            TYPE : IMAGE_TLV_JTAG_EN
                JTAG_EN          0x01
                SEC_DEBUG_EN     0x02
                DSP_JTAG_EN      0x04
                WIFI_JTAG_EN     0x08
                BT_JTAG_EN       0x10 """
        if opts.type is not IMAGE_TYPE_AUTH:
            jtag_en = 0x00
            jtag_en |= 0x01 * opts.jtag_cm33_ns_enable
            jtag_en |= 0x02 * opts.jtag_cm33_s_enable
            jtag_en |= 0x04 * opts.jtag_dsp_enable
            jtag_en |= 0x08 * opts.jtag_wifi_enable
            jtag_en |= 0x10 * opts.jtag_bt_enable
            if jtag_en:
                payload = struct.pack(e + 'B', jtag_en)
                prot_tlv.add("JTAGEN", payload)

        """ JTAG unlock password
            TYPE : IMAGE_TLV_JTAG_PW
                   jtag_pw0 = 0x12345678
                   jtag_pw1 = 0x87654321
                   jtag_pw2 = 0x56781234
                   jtag_pw3 = 0x43218765 """
        if opts.type is not IMAGE_TYPE_AUTH:
            if opts.jtag_pw:
                payload = struct.pack(e + 'IIII', *opts.jtag_pw)
                prot_tlv.add("JTAGPW", payload)

        """ MEID
            TYPE : IMAGE_TLV_MEID
                   32-digits hex number """
        if opts.type is IMAGE_TYPE_CERT:
            if opts.meid:
                payload = struct.pack(e + 'IIII', *opts.meid)
                prot_tlv.add("MEID", payload)

        """ Anti-rollback version number
            TYPE : IMAGE_TLV_AR_VER
                   ar_ver = 0x1 # support 1~64 (0x1 ~ 0x40) """
        if opts.type not in [IMAGE_TYPE_AUTH, IMAGE_TYPE_CERT]:
            if opts.ar_ver:
                payload = struct.pack(e + 'B', opts.ar_ver)
                prot_tlv.add("AR_VER", payload)

        """ DA authentication public key
            TYPE : IMAGE_TLV_DAA_PUBKEY """
        if opts.type is IMAGE_TYPE_AUTH:
            if opts.daa_pubkey:
                pubkey_bytes = opts.daa_pubkey.get_public_bytes()
                prot_tlv.add("DAA_PUBKEY_EC256", pubkey_bytes)
            if opts.daa_pubkey384:
                pubkey_bytes = opts.daa_pubkey384.get_public_bytes()
                prot_tlv.add("DAA_PUBKEY_EC384", pubkey_bytes)
            if opts.daa_pubkey521:
                pubkey_bytes = opts.daa_pubkey521.get_public_bytes()
                prot_tlv.add("DAA_PUBKEY_EC521", pubkey_bytes)

        """ DA SHA256 hash
            TYPE : IMAGE_TLV_DA_HASH """
        if opts.type is IMAGE_TYPE_AUTH:
            if opts.da_hash:
                payload = struct.pack(e + 'IIIIIIII', *opts.da_hash)
                prot_tlv.add("DA_HASH_SHA256", payload)
            if opts.da_hash384:
                payload = struct.pack(e + 'IIIIIIIIIIII', *opts.da_hash384)
                prot_tlv.add("DA_HASH_SHA384", payload)
            if opts.da_hash521:
                payload = struct.pack(e + 'IIIIIIIIIIIIIIII', *opts.da_hash521)
                prot_tlv.add("DA_HASH_SHA521", payload)
        """ Custom name
            TYPE : IMAGE_TLV_CUST_NAME """
        if opts.cust_name:
            payload = struct.pack(e + 'BBBB', *opts.cust_name)
            prot_tlv.add("CUST_NAME", payload)

        """ Bootloader secure attributes
            TYPE : IMAGE_BL_SECURE_ATTR
                   4-bytes bit mask.
                   BROM_SEC_CFG_CMD_READ_FILTER_DIS    0x10000000
                   BROM_SEC_CFG_CMD_WRITE_FILTER_DIS   0x20000000
                   BROM_SEC_CFG_CMD_FILTER_EN          0x80000000
                   BROM_SEC_CFG_UART_LOG_DISABLE       0x01000000 """
        if opts.type not in [IMAGE_TYPE_AUTH, IMAGE_TYPE_CERT]:
            sattrs = 0
            sattrs |= 0x10000000 * opts.sattrs_no_cmd_read_filter
            sattrs |= 0x20000000 * opts.sattrs_no_cmd_write_filter
            sattrs |= 0x80000000 * opts.sattrs_cmd_filter_enable
            sattrs |= 0x01000000 * opts.sattrs_no_bootrom_log

            if sattrs:  # add secure attributes TLV if used.
                payload = struct.pack(e + 'I', sattrs)
                prot_tlv.add('BL_SECURE_ATTR', payload)

        """ ASIC_MPU register address-value pairs
            TYPE : IMAGE_TLV_DAPC_CONFIG """
        if opts.type not in [IMAGE_TYPE_AUTH, IMAGE_TYPE_CERT]:
            if opts.asic_mpu and len(opts.asic_mpu) > 0:
                payload = bytearray()
                for p in opts.asic_mpu:
                    payload += struct.pack(e + 'HI', p[0], p[1])
                prot_tlv.add("ASIC_MPU", payload)

        """ INFRA_DAPC register address-value pairs
            TYPE : IMAGE_TLV_SMPU_CONFIG """
        if opts.type not in [IMAGE_TYPE_AUTH, IMAGE_TYPE_CERT]:
            if opts.infra_dapc and len(opts.infra_dapc) > 0:
                payload = bytearray()
                for p in opts.infra_dapc:
                    payload += struct.pack(e + 'HI', p[0], p[1])
                prot_tlv.add("INFRA_DAPC", payload)

        """ AUDIO_DAPC register address-value pairs
            TYPE : IMAGE_TLV_AUD_DAPC_CONFIG """
        if opts.type not in [IMAGE_TYPE_AUTH, IMAGE_TYPE_CERT]:
            if opts.aud_dapc and len(opts.aud_dapc) > 0:
                payload = bytearray()
                for p in opts.aud_dapc:
                    payload += struct.pack(e + 'HI', p[0], p[1])
                prot_tlv.add("AUDIO_DAPC", payload)

        """ Certificate security control attributes
            TYPE : IMAGE_TLV_SCTRL_CERT_ATTR
                   4-bytes bit mask.
                   SCTRL_CERT_JTAG_EN                   0x00000001
                   SCTRL_CERT_SEC_DEBUG_EN              0x00000002
                   SCTRL_CERT_DSP_JTAG_EN               0x00000004
                   SCTRL_CERT_WIFI_JTAG_EN              0x00000008
                   SCTRL_CERT_BT_JTAG_EN                0x00000010
                   SCTRL_CERT_AR_DIS                    0x00000200
                   BROM_SEC_CFG_CMD_READ_FILTER_DIS     0x10000000
                   BROM_SEC_CFG_CMD_WRITE_FILTER_DIS    0x20000000
                   BROM_SEC_CFG_CMD_FILTER_EN           0x80000000 """
        if opts.type is IMAGE_TYPE_CERT:
            cert_attr = 0
            cert_attr |= 0x00000001 * opts.sctrl_cert_jtag_en
            cert_attr |= 0x00000002 * opts.sctrl_cert_sec_debug_en
            cert_attr |= 0x00000004 * opts.sctrl_cert_dsp_jtag_en
            cert_attr |= 0x00000008 * opts.sctrl_cert_wifi_jtag_en
            cert_attr |= 0x00000010 * opts.sctrl_cert_bt_jtag_en
            cert_attr |= 0x00000200 * opts.sctrl_cert_ar_dis
            cert_attr |= 0x10000000 * opts.brom_sec_cfg_cmd_read_filter_dis
            cert_attr |= 0x20000000 * opts.brom_sec_cfg_cmd_write_filter_dis
            cert_attr |= 0x80000000 * opts.brom_sec_cfg_cmd_filter_en
            if cert_attr:
                payload = struct.pack(e + 'I', cert_attr)
                prot_tlv.add('SCTRL_CERT_ATTR', payload)

        """ Authentication file
            TYPE : IMAGE_TLV_TOOL_AUTH_ATTR
                   4-bytes bit mask.
                   TOOL_AUTH_BIND2DA              0x00000001
                   TOOL_AUTH_AR_VERSION_MASK      0x00000FF0 """
        if opts.type is IMAGE_TYPE_AUTH:
            if opts.tool_auth_ar_version is None:
                opts.tool_auth_ar_version = 0
            auth_attr = 0
            auth_attr |= 0x00000001 * opts.tool_auth_bind2da
            auth_attr |= 0x00000010 * opts.tool_auth_ar_version
            if auth_attr:
                payload = struct.pack(e + 'I', auth_attr)
                prot_tlv.add("TOOL_AUTH_ATTR", payload)
        print("end proteck MTK TLV")
        return prot_tlv

    def add_header(self, enckey, protected_tlv_size, opts):
        """Install the image header."""

        flags = 0
        if enckey is not None:
            flags |= IMAGE_F['ENCRYPTED']

        if opts.type == IMAGE_TYPE_CERT:
            flags = IMAGE_F['SCTRL_CERT']

        if opts.type == IMAGE_TYPE_AUTH:
            flags = IMAGE_F['TOOL_AUTH']
        t_build = datetime.datetime.now()
        t_build0 = int(t_build.strftime("%Y")[0:2], 16)
        t_build1 = int(t_build.strftime("%Y")[2:4], 16)
        t_build2 = int(t_build.strftime("%d%m"), 16)
        t_build3 = int(t_build.strftime("%S%M%H"), 16)
        e = STRUCT_ENDIAN_DICT[self.endian]
        fmt = (e +
               # type ImageHdr struct {
               'I' +     # Magic    uint32
               'I' +     # LoadAddr uint32
               'H' +     # HdrSz    uint16
               'H' +     # PTLVSz   uint16
               'I' +     # ImgSz    uint32
               'I' +     # Flags    uint32
               #'BBHI' +  # Vers     ImageVersion
               #'I'       # Pad1     uint32
               'BBH' +   # Vers     ImageVersion
               'B' +     #build_time:year0
               'B' +     #build_time:year1
               'H' +     #build_time:month/day
               'I')      #build_time:hours,min,sec
        assert struct.calcsize(fmt) == IMAGE_HEADER_SIZE
        header = struct.pack(fmt,
                             IMAGE_MAGIC,
                             self.load_addr,
                             self.header_size,
                             protected_tlv_size,  # TLV Info header + Protected TLVs
                             len(self.payload) - self.header_size,  # ImageSz
                             flags,
                             self.version.major,
                             self.version.minor or 0,
                             self.version.revision or 0,
                             #self.version.build or 0,
                             #0)  # Pad1
                             t_build0,
                             t_build1,
                             t_build2,
                             t_build3,
                             )   
        self.payload = bytearray(self.payload)
        self.payload[:len(header)] = header

    def _trailer_size(self, write_size, max_sectors, overwrite_only, enckey,
                      save_enctlv, enctlv_len):
        # NOTE: should already be checked by the argument parser
        magic_size = 16
        if overwrite_only:
            return MAX_ALIGN * 2 + magic_size
        else:
            if write_size not in set([1, 2, 4, 8]):
                raise click.BadParameter("Invalid alignment: {}".format(
                    write_size))
            m = DEFAULT_MAX_SECTORS if max_sectors is None else max_sectors
            trailer = m * 3 * write_size  # status area
            if enckey is not None:
                if save_enctlv:
                    # TLV saved by the bootloader is aligned
                    keylen = (int((enctlv_len - 1) / MAX_ALIGN) + 1) * MAX_ALIGN
                else:
                    keylen = 16
                trailer += keylen * 2  # encryption keys
            trailer += MAX_ALIGN * 4  # image_ok/copy_done/swap_info/swap_size
            trailer += magic_size
            return trailer

    def pad_to(self, size):
        """Pad the image to the given size, with the given flash alignment."""
        tsize = self._trailer_size(self.align, self.max_sectors,
                                   self.overwrite_only, self.enckey,
                                   self.save_enctlv, self.enctlv_len)
        padding = size - (len(self.payload) + tsize)
        pbytes = bytes([self.erased_val] * padding)
        pbytes += bytes([self.erased_val] * (tsize - len(boot_magic)))
        pbytes += boot_magic
        self.payload += pbytes

    @staticmethod
    def verify(imgfile, key):
        with open(imgfile, "rb") as f:
            b = f.read()

        magic, _, header_size, _, img_size = struct.unpack('IIHHI', b[:16])
        version = struct.unpack('BBHI', b[20:28])

        if magic != IMAGE_MAGIC:
            return VerifyResult.INVALID_MAGIC, None

        tlv_info = b[header_size+img_size:header_size+img_size+TLV_INFO_SIZE]
        magic, tlv_tot = struct.unpack('HH', tlv_info)
        if magic != TLV_INFO_MAGIC:
            return VerifyResult.INVALID_TLV_INFO_MAGIC, None

        if key.sig_tlv() == "ECDSA256":
            sha = hashlib.sha256()
        elif key.sig_tlv() == "ECDSA384":
            sha = hashlib.sha384()
        elif key.sig_tlv() == "ECDSA521":
            sha = hashlib.sha512()
        else:
            raise click.UsageError("Unknown signature type")

        sha.update(b[:header_size+img_size])
        digest = sha.digest()

        tlv_off = header_size + img_size
        tlv_end = tlv_off + tlv_tot
        tlv_off += TLV_INFO_SIZE  # skip tlv info
        while tlv_off < tlv_end:
            tlv = b[tlv_off:tlv_off+TLV_SIZE]
            tlv_type, _, tlv_len = struct.unpack('BBH', tlv)
            if ((key.sig_type() == "ECDSA256_SHA256" and
                    tlv_type == TLV_VALUES["SHA256"])
                or (key.sig_type() == "ECDSA384_SHA384" and
                    tlv_type == TLV_VALUES["SHA384"])
                or (key.sig_type() == "ECDSA521_SHA512" and
                    tlv_type == TLV_VALUES["SHA512"])):
                off = tlv_off + TLV_SIZE
                if digest == b[off:off+tlv_len]:
                    if key is None:
                        return VerifyResult.OK, version
                else:
                    return VerifyResult.INVALID_HASH, None
            elif key is not None and tlv_type == TLV_VALUES[key.sig_tlv()]:
                off = tlv_off + TLV_SIZE
                tlv_sig = b[off:off+tlv_len]
                payload = b[:header_size+img_size]
                try:
                    if hasattr(key, 'verify'):
                        key.verify(tlv_sig, payload)
                    else:
                        key.verify_digest(tlv_sig, digest)
                    return VerifyResult.OK, version
                except InvalidSignature:
                    # continue to next TLV
                    pass
            tlv_off += TLV_SIZE + tlv_len
        return VerifyResult.INVALID_SIGNATURE, None

#!/usr/bin/python
#
# otatool.py
#
# mt8516 Yocto ota tool to generate upg_id_file.bin & update.bin
#
# Author: Yuhuai.Huang
#

import os, sys, getopt, shutil ,zipfile ,hashlib, time

upg_id_file = os.path.join(sys.path[0], "upg_id_file.bin")
update_zip = "update.zip"
update_bin = os.path.join(sys.path[0], "update.bin")
unit_size = (1<<21)  # 2MB per segment

def show_help():
    print("\n This otatool would try to generate the related upg_id_file.bin")
    print(" and update.bin on the basis of update.zip for mt8516 Yocto system\n")
    print(" Usage: python otatool.py <option>")
    print(" [option]")
    print("  -f\t\t  force update")
    print("  -l\t\t  low memory mode")
    print("  -v [val]\t  version")
    print("  -s [val]\t  file size")
    print("  -p [val]\t  update.zip path, current path as default, if not indicated")
    print("  -h, --help\t  show this help\n")

def write_file(file_path, string):
    print(string)
    with open(file_path, "a+") as idf:
        idf.write(string + "\n")

def unzip_file(file_name, dst_dir):
    print("\nunzip " + file_name  + " to " + dst_dir)
    zfile = zipfile.ZipFile(file_name)
    if os.path.isdir(dst_dir):
        for imgf in os.listdir(dst_dir):
            img_path = os.path.join(dst_dir, imgf)
            if os.path.isfile(img_path):
                os.remove(img_path)
    else:
        os.mkdir(dst_dir)

    for fn in zfile.namelist():
        zfile.extract(fn, dst_dir)
    zfile.close()

def zip_file(file_dir, dst_file):
    zfile = zipfile.ZipFile(dst_file, "w", zipfile.ZIP_STORED)
    for fn in os.listdir(file_dir):
        zfile.write(os.path.join(file_dir, fn), fn)
    zfile.close()

def calc_file_sha256sum(file_name):
    with open(file_name, "rb") as sha256f:
        sha256obj = hashlib.sha256()
        while True:
            read_buf = sha256f.read(1024)
            if not read_buf:
                break
            else:
                sha256obj.update(read_buf)
        hash = sha256obj.hexdigest()
    return hash

def get_seg_checksum(dir_name):
    for fn in os.listdir(dir_name):
        offset = 0
        seg_cnt = 0
        img_name = os.path.join(dir_name, fn)
        file_size = os.path.getsize(img_name)
        print(fn + " size: " + str(file_size) + " unit size: " + str(unit_size))
        with open(img_name, "rb") as fr:
            while (offset < file_size):
                read_buf = fr.read(unit_size)
                print(fn + " offset: " + str(offset))
                read_len = len(read_buf)
                tmp_file = img_name + ".temp"
                with open(tmp_file, "wb") as fw:
                    fw.write(read_buf)
                seg_hash = calc_file_sha256sum(tmp_file)
                write_file(upg_id_file, fn + " | " + str(offset) + " - " + str(offset + read_len - 1) + " | " + seg_hash)
                offset += read_len
                seg_cnt += 1
                os.remove(tmp_file)

def main():
    force_update = False
    low_memory_mode = False
    version = 10
    file_size = 38163210
    pkg_path = ""

    try:
        opts, args = getopt.getopt(sys.argv[1:], "fhlv:s:p:", ["help"])
    except getopt.GetoptError as err:
        print(err)
        show_help()
        sys.exit(-1)

    for o, a in opts:
        if o == "-f":
            force_update = True
        elif o in ("-h", "--help"):
            show_help()
            sys.exit()
        elif o == "-l":
            low_memory_mode = True
        elif o == "-v":
            version = a
        elif o == "-s":
            file_size = a
        elif o == "-p":
            pkg_path = a
        else:
            assert False, "unhandled option"

    if update_zip in pkg_path:
        pass
    elif os.path.isfile(update_zip):
        pkg_path = update_zip
    else:
        print("update.zip not found")
        show_help()
        sys.exit(-1)

    print("\n")
    f = open(upg_id_file, "w")
    f.close()
    write_file(upg_id_file, "Force_Update=" + ("true" if force_update else "false"))
    write_file(upg_id_file, "Version=" + str(version))
    write_file(upg_id_file, "File_Size=" + str(file_size))
    write_file(upg_id_file, "Low_Memory=" + ("true" if low_memory_mode else "false"))

    unzip_dir = pkg_path + "_files"
    unzip_file(pkg_path, unzip_dir)

    print("\ncheck package sha256sum")
    time.sleep(2)
    pkg_size = os.path.getsize(pkg_path)
    pkg_sha256 = calc_file_sha256sum(pkg_path)
    write_file(upg_id_file, update_zip + " | 0" + " - " + str(pkg_size - 1) + " | " + pkg_sha256)

    print("\ncheck segment sha256sum")
    time.sleep(2)
    get_seg_checksum(unzip_dir)

    print("\nzip " + update_zip + " to " + update_bin)
    time.sleep(2)
    zip_file(unzip_dir, update_bin)
    shutil.rmtree(unzip_dir)
    print("\nOTA Tool DONE!")
    print("\nTry \"python otatool.py -h\" for more options")

if __name__ == '__main__':
    main()

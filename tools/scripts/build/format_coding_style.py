#!/usr/bin/env python2
from logging import warning
import os,sys,time
from os import path, walk
from os.path import isfile, isdir, join
import filecmp,re
import datetime

try:
    # python2
    import ConfigParser as configparser
except ImportError:
    # python3
    import configparser

def chk_comment_data(inp_data):
    i=inp_data.find("/*",0)
    if i!=-1:
        j=inp_data.find("*/",0)
        #new_data=inp_data[i+2:j]
        new_data=inp_data[0:i]+"("+inp_data[i+2:j]+")"
        return new_data
    return inp_data

def srh_line(line_data,inx):
    global define_list

    if line_data[0:6]=="#ifdef":
       define_list.append(line_data.strip())
       return "",inx+1,0
    if line_data[0:7]=="#ifndef":
       define_list.append(line_data.strip())
       return "",inx+1,0

    if line_data[0:11]=="#if defined":
       define_list.append(line_data.strip())
       return "",inx+1,0
    if line_data[0:11]!="#if defined" and line_data[0:4]=="#if ":
       define_list.append(line_data.strip())
       return "",inx+1,0
    if line_data[0:11]!="#if defined" and line_data[0:4]=="#if(":
       define_list.append(line_data.strip())
       return "",inx+1,0

    if line_data[0:5]=="#else":
       new_comment_data=chk_comment_data(define_list[inx].strip("\n"))
       comment_data=" /* "+new_comment_data+" */"
       return comment_data,inx,5

    if line_data[0:6]=="#endif" and inx>=1:
       #print("pop",inx,line_data)
       new_comment_data=chk_comment_data(define_list[inx].strip("\n"))
       comment_data=" /* "+new_comment_data+" */"
       define_list.pop(inx)
       #del define_list[inx]
       return comment_data,inx-1,6
    return "",inx,0

def filter_log(inp_string):
    i=inp_string.find(":",0)
    j=inp_string.find("..",0)
    if j!=-1:
        return inp_string[j+3::].strip()
    else:
        return inp_string[i+1::].strip()

def get_file_status(inp_string):
    lens=len(inp_string)
    #print("-----",lens,inp_string)
    if lens==2:
        return "","",False
    m=["tag_m:"]*1
    n=["tag_n:"]*1
    check=False
    for i in range(0,lens,1):
        if inp_string[i].find("modified:",0)!=-1:
            #print("modified:",i,inp_string[i])
            if (inp_string[i].find(".c",0)!=-1 or inp_string[i].find(".h",0)!=-1) and \
               (inp_string[i].find(".cmm",0)==-1 and inp_string[i].find(".bak",0)==-1) :
                temp=filter_log(inp_string[i])
                m.append(temp)
                check=True
        if inp_string[i].find("new file:",0)!=-1:
            #print("new file:",i,inp_string[i])
            if (inp_string[i].find(".c",0)!=-1 or inp_string[i].find(".h",0)!=-1) and \
                (inp_string[i].find(".cmm",0)==-1 and inp_string[i].find(".bak",0)==-1) :
                temp=filter_log(inp_string[i])
                n.append(temp)
                check=True
    #print("---modified files:",m)
    #print("---new files:",n)
    return m ,n,check

def find_memory_size(inp):
    global memory_srh_done
    if inp=="":
        return -1,-1
    key_w=[".sysram_text",".cached_sysram_data",".bss",".noncached_sysram_text",".noncached_sysram_data",".noncached_sysram_bss", \
            ".non_init_bss", \
            ".stack", \
            ".tcm_text",".tcm_data",".tcm_bss", \
            ".cached_ram_text",".cached_ram_data",".cached_ram_bss",".noncached_ram_text",".noncached_ram_data",".noncached_ram_bss", \
            ".non_init_cached_ram_bss", \
            ".wffw_code",".btfw_code" \
            ]
    lens=len(key_w)
    for m in range(0,lens,1):
        i=inp.find(key_w[m],0)
        if i!=-1 and memory_srh_done[m]!="done":
            j= i+len(key_w[m])
            k0=inp.find("0",j+1)
            k1=inp.find(" ",k0+1)
            size="0x"+inp[k0:k1]
            memory_srh_done[m]="done"
            return m,int(size,16)
    return -2,-2

def get_memory_status(inp_string,m_zone_limit):
    global btfw_2_1hdr_size,wffw_emi_size

    lens=len(inp_string)
    sysram_size=0
    sysram_heap_size=0
    stack_size=0
    tcm_size=0
    psram_size=0
    platform_heap_size=0
    wififw_size=0
    btfw_size=0

    for i in range(0,lens,1):
        type,size=find_memory_size(inp_string[i])
        if type>=0 and type<=5:
            #print("sys",type,hex(size))
            sysram_size+=size
        elif type==6:
             sysram_heap_size=size
        elif type==7:
             stack_size=size
        elif type>=8 and type<=10:
            #print("tcm",type,hex(size))
            tcm_size+=size
        elif type>=11 and type<=16:
            #print("ram",type,hex(size))
            psram_size+=size
        elif type==17:
            platform_heap_size=size
        elif type==18:
            wififw_size=size
        elif type==19:
            btfw_size=size

    percent=sysram_size*100/m_zone_limit[0]
    percent_str="{0:.1f}".format(percent)+"%"
    print("--sysram size usage:",percent_str,str(sysram_size//1024)+"K",str(m_zone_limit[0]//1024)+"K")
    #
    if sysram_heap_size!=0:
        percent=sysram_heap_size*100/m_zone_limit[0]
        percent_str="{0:.1f}".format(percent)+"%"
        print("--sysram heap size:",percent_str,str(sysram_heap_size//1024)+"K",str(m_zone_limit[0]//1024)+"K")
    #
    percent=stack_size*100/m_zone_limit[0]
    percent_str="{0:.1f}".format(percent)+"%"
    print("--stack size:",percent_str,str(stack_size//1024)+"K",str(m_zone_limit[0]//1024)+"K")
    #
    percent=tcm_size*100/m_zone_limit[1]
    percent_str="{0:.1f}".format(percent)+"%"
    print("--tcm size usage:",percent_str,str(tcm_size//1024)+"K",str(m_zone_limit[1]//1024)+"K")
    #
    percent=psram_size*100/m_zone_limit[2]
    percent_str="{0:.1f}".format(percent)+"%"
    print("--psram size usage:",percent_str,str(psram_size//1024)+"K",str(m_zone_limit[2]//1024)+"K")
    #
    if platform_heap_size!=0:
        percent=platform_heap_size*100/m_zone_limit[2]
        percent_str="{0:.1f}".format(percent)+"%"
        print("--psram heap size:",percent_str,str(platform_heap_size//1024)+"K",str(m_zone_limit[2]//1024)+"K")
    #
    if wififw_size!=0 and wffw_emi_size!=0:
        percent=wffw_emi_size*100/wififw_size
        percent_str="{0:.1f}".format(percent)+"%"
        print("--psram wffw code:",percent_str,str(wffw_emi_size//1024)+"K",str(wififw_size//1024)+"K","WIFI_RAM_CODE_iemi.bin")
    #
    if btfw_size!=0 and btfw_2_1hdr_size!=0:
        percent=btfw_2_1hdr_size*100/btfw_size
        percent_str="{0:.1f}".format(percent)+"%"
        print("--psram btfw code~=:",percent_str,str(btfw_2_1hdr_size/1024)+"K",str(btfw_size//1024)+"K","BT_RAM_CODE_MT7933_2_1_hdr.bin")
    if m_zone_limit[0]!=0:
        remaind_size=m_zone_limit[0]-sysram_size-sysram_heap_size-stack_size
        if remaind_size<102400:
            print("Warning:SYSRAM remaind size:",str(remaind_size//1024)+"K")
    return True

def modify_files_mtime(list_tbl):
    lens=len(list_tbl)
    for i in range(1,lens,1):
        dt = datetime.datetime.now()
        now=time.mktime(dt.timetuple())
        os.utime(list_tbl[i], (now, now))

def run_format_file(fullpath,files_tbl):
    global define_list,last_build_time

    if files_tbl=="force_mode":
        pass
    elif serach_file_tbl(fullpath,files_tbl)!=True:
        return
    else:
        modTimesinceEpoc =os.path.getmtime(fullpath)
        if (last_build_time!=0) and (modTimesinceEpoc<=last_build_time) :
            #print("pass file:",fullpath)
            return

    CODING_STYLE='/mtkoss/astyle/3.1-ubuntu.14.04/bin/astyle --style=kr \
    --indent=spaces=4 --indent-switches --indent-labels --indent-cases --indent-col1-comments \
    --align-pointer=name --align-reference=name --convert-tabs\
    --pad-header --pad-oper --unpad-paren \
    --break-one-line-headers -xf -xh -xV -m0 -M120 '+fullpath
    #--lineend=linux
    cnt=0
    while True:
        os.system(CODING_STYLE)
        out_file=fullpath+".orig"
        if os.path.exists(out_file)==True:
            del_f="rm -rf "+out_file
            comp_result = filecmp.cmp(fullpath,out_file,shallow = False)
            if comp_result==True:
                os.system(fullpath)
                os.system("cp "+out_file+" "+fullpath)
                os.system(del_f)
                break
            else:
                os.system(del_f)
                cnt+=1 #protect of C++ type .h that with class declare.
                if cnt>5:
                    break
        else:
            break

    out_file=fullpath+".bak"
    if os.path.exists(out_file)==True:
        del_f="rm -rf "+out_file
        os.system(del_f)
    #out_file="dos2unix -k "+fullpath
    #os.system(out_file)

    #comment of :#ifdef#else#endif
    define_list=["tag"]*1
    with open(fullpath,"r") as file:
        try:
            inp_line=file.readlines()
        except Exception as e:
            return
    disable=0
    with open(fullpath,"w") as file:
        k=0
        for i,line in enumerate(inp_line,1):
            j0=line.find("/*")
            j1=line.find("*/")
            if j0!=-1 and j1==-1:
                disable=1
            if j0==-1 and j1!=-1:
                disable=0
            if disable==0:
                fd,j,m=srh_line(line,k)
                k=j
                if fd!="":
                    #newline=line[0:m].strip("\n")+fd+"\n"
                    if m==5:
                        newline="#else"+fd+"\n"
                    elif m==6:
                        newline="#endif"+fd+"\n"
                    file.writelines(newline)
                else:
                    file.writelines(line)
            else:
                file.writelines(line)
    os.utime(fullpath, (last_build_time, last_build_time))

def serach_file_tbl(fullpath,files_tbl):
    lens=len(files_tbl)
    for i in range(1,lens,1):
        if files_tbl[i]=="":
            return False
        if fullpath.find(files_tbl[i],0)!=-1:
            return True
    return False

def formating(tar_dir,type,files_tbl):
    i=len(files_tbl)
    if i==1:
        return
    for root, dirs, files in walk(tar_dir):
        for f in files:
            fullpath = join(root, f)
            if type=="file_c_h":
                if f.endswith('.c') or f.endswith('.h'):
                    run_format_file(fullpath,files_tbl)
            elif type=="file_c" and f.endswith('.c'):
                run_format_file(fullpath,files_tbl)
            elif type=="file_h" and f.endswith('.h'):
                run_format_file(fullpath,files_tbl)

def parse_minifest_repo(mode,create_file,prj_name):
    global repo_mani

    cwd = os.getcwd()
    manifest = cwd+"/.repo/manifests/default.xml"
    myfile=open(manifest,"r")
    myline=myfile.readline().strip()
    second_srh_trg=0
    while myline!="</manifest>":#end of this file
        i=myline.find("hadron",0)
        if i!=-1:
            if not (myline.find("tools",0)!=-1 or myline.find("8512",0)!=-1 or myline.find("7686",0)!=-1 or myline.find("testframework",0)!=-1 or \
                myline.find("76x7",0)!=-1 or myline.find("prebuilt",0)!=-1 or myline.find("doc",0)!=-1 or myline.find("third_party",0)!=-1 or \
                myline.find("kernel/rtos",0)!=-1 or myline.find("config",0)!=-1 or myline.find("sec_drv",0)!=-1):

                if myline.find("mt7933_hdk/apps/",0)!=-1 and mode=="create_prj_file" and myline.find("slt_",0)==-1:
                        j=myline.find("path=",i)
                        k=myline.find('"',j+7)
                        temp_str="["+myline[j+30:k]+"]"
                        create_file.write(temp_str+"\n")
                        create_file.write("prj_repo=\n")
                        create_file.write("active_repo=\n")
                        create_file.write("clean_index=empty\n")
                        create_file.write("warning_files=\n")
                        create_file.write("last_build_time=\n")
                        create_file.write("rtos_reserved_limit_inx=\n")
                if myline.find("mt7933_hdk/apps/",0)==-1 and mode=="get_data":
                    j=myline.find("path=",i)
                    k=myline.find('"',j+7)
                    repo_mani.append(myline[j+6:k])
                    second_srh_trg=0#disable
                    #print(m,i,j,k,myline[j+6:k])
        if second_srh_trg==1 and mode=="get_data":
            if myline.find("RELEASE_POLICY",0)!=-1:
                second_srh_trg=0
                if myline.find("binary",0)!=-1:#temporary remove
                    repo_mani.pop()

        myline=myfile.readline().strip()
    myfile.close()
    if mode=="get_data":
        repo_mani.append("project/mt7933_hdk/apps/bootloader")
        if prj_name!="bootloader":
            repo_mani.append("project/mt7933_hdk/apps/"+prj_name)
        repo_mani.append("middleware/MTK/config_framework")
        repo_mani.append("tinysys/adsp/HIFI4/kernel")
        repo_mani.append("tinysys/adsp/HIFI4/drivers")
        repo_mani.append("tinysys/adsp/HIFI4/middleware")
        #repo_mani.append("tinysys/adsp/HIFI4/project/mt7933/config")#only .mk file

        #temporary remove of some bt repos.(need sync code with airhora)
        if "middleware/MTK/bt_connection_manager" in repo_mani:
            repo_mani.remove("middleware/MTK/bt_connection_manager")
        if "middleware/MTK/sink" in repo_mani:
            repo_mani.remove("middleware/MTK/sink")
        if "middleware/MTK/mesh_protected" in repo_mani:
            repo_mani.remove("middleware/MTK/mesh_protected")#binary
        if "middleware/MTK/audio/bt_codec" in repo_mani:
            repo_mani.remove("middleware/MTK/audio/bt_codec")
        if "middleware/MTK/audio_manager" in repo_mani:
            repo_mani.remove("middleware/MTK/audio_manager")
    print("-------")

def serch_match_repo(mode,inp_des):
    global prj_repo,repo_mani,active_repo
    lens=len(repo_mani)
    for i in range(1,lens,1):
        if repo_mani[i]=="":
            break
        lens_out=len(inp_des)
        for j in range(1,lens_out,1):
            k=inp_des[j].find(repo_mani[i],0)
            len_r=len(repo_mani[i])+k
            if k!=-1:
                #print("~~~~",inp_des[j][len_r:len_r+1])
                if inp_des[j][len_r:len_r+1]=="/" or inp_des[j][len_r:len_r+1]=="":
                    repo_add = os.getcwd()+"/"+repo_mani[i]
                    if mode=="prj_repo":
                        prj_repo.append(repo_add)
                    elif  mode=="active_repo":
                        active_repo.append(repo_add)
                    break


def parse_prj_all_repo(folder_path,pram):
    global prj_repo,repo_mani

    current_pwd=os.getcwd()
    repo_add = current_pwd+"/project/mt7933_hdk/apps/"+pram
    prj_repo.append(repo_add)
    if pram!="bootloader":
        repo_add = current_pwd+"/project/mt7933_hdk/apps/bootloader"
        prj_repo.append(repo_add)
    repo_add = current_pwd+"/driver/board/mt7933_hdk"
    prj_repo.append(repo_add)
    repo_add = current_pwd+"/driver/chip/inc"
    prj_repo.append(repo_add)
    repo_add = current_pwd+"/driver/chip/mt7933/inc"
    prj_repo.append(repo_add)
    repo_add = current_pwd+"/driver/chip/mt7933/src"
    prj_repo.append(repo_add)
    inx=0
    p=os.listdir(folder_path)
    n=0
    out_folder=["build_out_folder:"]*1
    for i in p:
        if not (i=="src" or i=="project" or i=="wpa_supplicant"):
            sub_folder_path=folder_path+"/"+i
            for rootdir, dirs, files in os.walk(sub_folder_path):
                for subdir in dirs:
                    full_path=os.path.join(rootdir, subdir)
                    if not (full_path.find("project",0)!=-1 or full_path.find("third_party",0)!=-1 or \
                        full_path.find("rtos/FreeRTOS",0)!=-1 or full_path.find("driver/chip",0)!=-1 or \
                        full_path.find("board/mt7933_hdk",0)!=-1):
                        n+=1
                        #print(n,full_path)
                        out_folder.append(full_path)
                    elif full_path.find("chip/mt7933/src_core",0)!=-1:
                        inx=1
    if inx==1:
       repo_add = current_pwd+"/driver/chip/mt7933/src_core"
       prj_repo.append(repo_add)
    serch_match_repo("prj_repo",out_folder)
    print(prj_repo,len(prj_repo))
    print("-----end of prj repo-------")

def parse_keyword_file(keyword,file_path,delta):
    myfile=open(file_path,"r")
    myline=myfile.readline().strip()
    data=""
    while myline:
        i=myline.find(keyword,0)
        if i!=-1:
            data=myline[i+delta::]
            break
        myline=myfile.readline().strip()
    myfile.close()
    return data

def filter_repo_by_build_time(path,build_time_start,build_time_end):
    global active_repo

    out_file=["build_o_file:"]*1
    n=0
    for root, dirs, files in walk(path):
        for f in files:
            fullpath = join(root, f)
            if f.endswith('.o'):
                modTimesinceEpoc =os.path.getmtime(fullpath)

                if (modTimesinceEpoc>build_time_start) and (modTimesinceEpoc<build_time_end):
                    n+=1
                    i=fullpath.find("obj",0)
                    temp_str=fullpath[i+4::]
                    out_file.append(temp_str.strip())
                    print("Modified",n,temp_str)
    #print(out_file)
    serch_match_repo("active_repo",out_file)
    print(active_repo)
    print("-----end of active repo-------")

def run_format_repo(run_repo,check_match):

    lens=len(run_repo)
    for i in range(1,lens,1):
        if run_repo[i]=="":
            break

        os.chdir(run_repo[i])
        print("REPO.:",run_repo[i])
        #os.system("git config core.whitespace cr-at-eol")
        #os.system("git config core.safecrlf false")#true --global
        #os.system("git config --global core.autocrlf true")#false input
        os.system('git add --ignore-removal .')
        #os.popen('git add --ignore-removal .')
        log_out  = os.popen('git status').readlines()

        modified_files,new_files,check=get_file_status(log_out)
        all_files=modified_files+new_files
        if check==True:
            #os.system("git rm --cached -r .")
            #os.system("git reset --hard")
            os.system("git reset")
            #os.system("git stash")#drop
            #print("CHECK:",check)
            if check_match==1:
                active_match_repo.append(run_repo[i])

        # if not (run_repo[i]=="middleware/MTK/bt_connection_manager" or \
        #         run_repo[i]=="middleware/MTK/sink" or \
        #         run_repo[i]=="middleware/MTK/mesh_protected" or \
        #         run_repo[i]=="middleware/MTK/audio/bt_codec" or \
        #         run_repo[i]=="middleware/MTK/audio_manager"):
        formating(run_repo[i],"file_c_h",all_files)

def transform(inp_data,bmode):
    if isinstance(inp_data, list):
        inp_data = ' '.join(map(str, inp_data))
    if inp_data.find("[",0)!=-1: #python2
        a=4
    else: #python3
        a=0

    temp=["tag:"]*1
    k=0
    current_pwd=os.getcwd()
    cwd_lens=len(current_pwd)
    while True:
        i=inp_data.find(current_pwd,k)
        j=inp_data.find(current_pwd,i+1)
        #print(i,j,inp_data)
        if j!=-1:
            if bmode==0:
                temp.append(inp_data[i:j-a].strip())
            else:
                temp.append(inp_data[i+cwd_lens:j-a].strip())
            k=j
        elif i!=-1:
            if bmode==0:
                if a==4:
                    j=inp_data.find("]",i+1)
                    temp.append(inp_data[i:j-1].strip())
                else:
                    temp.append(inp_data[i::].strip())
            else:
                if a==4:
                    j=inp_data.find("]",i+1)
                    temp.append(inp_data[i+cwd_lens:j-1].strip())
                else:
                    temp.append(inp_data[i+cwd_lens::].strip())
            break
        else:
            break
    return temp

def list_edit(inp_list1,inp_list2,bmode):
    len1=len(inp_list1)
    len2=len(inp_list2)
    #print("len",len1,len2)
    temp_list=["tag_remove"]*1
    for i in range(1,len1,1):
        inx=1
        k1=len(inp_list1[i])
        for j in range(1,len2,1):
            k2=len(inp_list2[j])
            #print(i,j,k1,k2,inp_list1[i],inp_list2[j])
            #if inp_list1[i].strip()==inp_list2[j].strip():
            if inp_list1[i][2:k1-1].find(inp_list2[j][2:k2-1],0)!=-1:
                inx=0
                break
        if inx==1:
            if bmode=="add":
                temp_list.append(inp_list1[i])
                print("Add warning file:",inp_list1[i])
            else:
                temp_list.append(inp_list1[i])
                print("Remove warning file",inp_list1[i])

    len1=len(temp_list)
    for i in range(1,len1,1):
        if bmode=="add":
            inp_list2.append(temp_list[i])
        else:
            inp_list1.remove(temp_list[i])
    if bmode=="add":
        return  inp_list2
    else:
        return  inp_list1


def build_w_filter_case(inp_str,inp_repo):
    if inp_str.find("prebuilt",0)!=-1:
        return True
    if inp_str.find("protected",0)!=-1:
        if inp_repo.find("protected",0)==-1:
           return True

    if inp_str.find("wlan_daemon/ated_ext",0)!=-1:
        return True
    if inp_str.find("wlan_tool/sigma",0)!=-1:
        return True

    if inp_str.find("[-Wcast-function-type]",0)!=-1:
        if inp_str.find("FreeRTOS_POSIX_timer.c",0)!=-1:
            return True

    # if inp_str.find("[-Wenum-conversion]",0)!=-1:
    #     if inp_str.find("hal_usb_mtu3_rscs.h",0)!=-1:#
    #         return True

    return False

def judge_build_pass(file,inp_repo ):
    global backup_warning_files
    #print(inp_repo)
    #Add NEPTUNE repo.
    inp_repo.append("/middleware/MTK/bt_mw")
    inp_repo.append("/middleware/MTK/bt_tools/boots")
    inp_repo.append("/middleware/MTK/bt_tools/picus")
    inp_repo.append("/middleware/MTK/connectivity/bt")
    #temporary disable    inp_repo.append("/middleware/MTK/connectivity/wlan_daemon/ated_ext")
    inp_repo.append("/middleware/MTK/connectivity/wlan_service")
    inp_repo.append("/middleware/MTK/connectivity/wlan_src_protected")
    #temporary disable    inp_repo.append("/middleware/MTK/connectivity/wlan_tool/sigma")
    inp_repo.append("/middleware/MTK/connectivity/wlan_tool/wifi_test_tool")

    #Remove special repo. on hadron
    #inp_repo.remove("/middleware/MTK/connectivity/wlan")
    #inp_repo.remove("/middleware/MTK/minisupp_protected")
    repo_remove="/middleware/MTK/ssusb_protected"#only bga
    if repo_remove in inp_repo:
         inp_repo.remove(repo_remove)
    #repo_remove="/middleware/MTK/audio/bt_codec"#only bga_sdk_audio
    #if repo_remove in inp_repo:
    #    inp_repo.remove(repo_remove)

    warning_files=["record_warning_file:"]*1
    warning_msg=["record_warning_msg:"]*1
    build_as_error=0
    lens=len(inp_repo)
    myfile=open(file,"r")
    myline=myfile.readline().strip()
    while myline:
        j=myline.find("warning",0)
        if j!=-1:
            for i in range(1,lens,1):
                k=myline.find(inp_repo[i],0)
                if k!=-1 and build_w_filter_case(myline,inp_repo[i])!=True:#special pass of some warning

                    k1=myline.find(".c",0)
                    if k1==-1:
                        k1=myline.find(".h",0)
                    file_path=myline[k:k1+2]
                    full_path=os.getcwd()+file_path

                    if full_path in warning_files:
                        pass
                    else:
                        warning_files.append(full_path)
                    warning_msg.append(myline)
                    build_as_error=1

        myline=myfile.readline().strip()
    myfile.close()
    backup_warning_files=list_edit(warning_files,backup_warning_files,"add")
    backup_warning_files=list_edit(backup_warning_files,warning_files,"remove")
    if build_as_error==1:
        modify_files_mtime(backup_warning_files)
        len1=len(warning_msg)
        if len1>0:
            if len1>15:
                len1=16#skip 1st
                print("---Top 15 warnings(build warning as ERROR)---")
            else:
                print("---"+str(len1-1)+" warnings(build warning as ERROR)---")
            for i in range(1,len1,1):
                print(warning_msg[i])
        return False
    else:
        return True

def search_content(inp_file,key_word):
    with open(inp_file,'r') as in_f:
        for line in in_f:
            new_line=line.strip()
            if new_line.find(key_word,0)!=-1:
                i=new_line.find("=",0)
                content=new_line[i+1::].strip()
                j=new_line.find("K",i)
                content=int(new_line[i+1:j])*1024
                print("RTOS reserved size:",str(new_line[i+1:j])+"K" )
                return content
    return ""

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

def memory_usage_lmt_chk(current_pwd,prj,build_type,out_folder,feature):
    global btfw_2_1hdr_size,wffw_emi_size
    check_zone=["ROM_BL","ROM_TFM","ROM_RTOS","ROM_DSP","ROM_BT","ROM_WIFI_EXT"]#
    #out_folder=current_pwd+"/out/mt7933_hdk/"+prj+"/"
    feature_file=current_pwd+"/project/mt7933_hdk/apps/"+prj+"/GCC/release/"+feature
    if build_type=="release" and os.path.exists(feature_file):
        rtos_reserved_size=search_content(feature_file,"MTK_FLASH_RTOS_ZONE_RESERVED_FOR_CUSTOMER")
    else:
        rtos_reserved_size=""
    bootloader_inx=0
    build_judge_fail=0
    elf_file=""
    for filename in os.listdir(out_folder):
        if filename.endswith('.ini'):
            out_scatter_file=out_folder+filename
            cfg = configparser.ConfigParser()
            cfg.read(out_scatter_file)
            for i in range(0,6,1):
                try:
                    img_file=cfg.get(check_zone[i], 'file_name')
                    if img_file!="" and os.path.exists(out_folder+img_file):
                        f_size=os.path.getsize(out_folder+img_file)
                        lim_size = int(cfg.get(check_zone[i], 'partition_size'),16)
                        if build_type=="release" and rtos_reserved_size!="" and check_zone[i]=="ROM_RTOS":
                            if (lim_size-f_size)<rtos_reserved_size:
                                overflow=rtos_reserved_size-(lim_size-f_size)
                                if prj=="hqa_desense":
                                    print("------build release:RTOS code size overflowed:",str(overflow)+" bytes")
                                build_judge_fail=1
                        percent=f_size*100/lim_size
                        percent_str="{0:.1f}".format(percent)+"%"
                        if bootloader_inx==0 or check_zone[i]!="ROM_BL":
                            print("--flash zone size usage:",check_zone[i],percent_str,str(f_size//1024)+"K",str(lim_size//1024)+"K",img_file)#,hex(lim_size-f_size))
                        if bootloader_inx==0 and check_zone[i]=="ROM_BL":
                            bootloader_inx=1
                        if check_zone[i]=="ROM_BT" and img_file=="BT_RAM_CODE_MT7933_2_1_hdr.bin":
                            if os.path.exists(out_folder+img_file):
                                btfw_2_1hdr_size=os.path.getsize(out_folder+img_file)
                                #print("btfw_size",btfw_2_1hdr_size)
                        if check_zone[i]=="ROM_WIFI_EXT":
                            if os.path.exists(out_folder+"WIFI_RAM_CODE_iemi.bin"):
                                wffw_emi_size=os.path.getsize(out_folder+"WIFI_RAM_CODE_iemi.bin")
                                #print("wffw_size",wffw_emi_size)
                except Exception as e:#for scatter zone exist
                    pass
        if filename.endswith('.elf') and filename.find("xip",0)!=-1 and filename.find("bootloader",0)==-1 and filename.find("_no",0)==-1:
            elf_file=filename
    tool=current_pwd+"/tools/gcc/linux/gcc-arm-none-eabi/bin/arm-none-eabi-objdump"

    ld_file=current_pwd+"/project/mt7933_hdk/apps/"+prj+"/GCC/ld/memory.ld"
    if not os.path.exists(ld_file):
        ld_file=current_pwd+"/project/mt7933_hdk/apps/"+prj+"/GCC/mt7933_flash.ld"
    m_zone=[]*1
    tcm=0
    sysram=0
    psram=0
    # psram_wffw=0
    # psram_btfw=0
    # psram_tfm=0
    with open(ld_file,'r') as in_f:
        for line in in_f:
            # remove comments in a single line
            line = re.sub(r"/\*.*?\*/", " ", line)
            # remove extra spaces
            line = ''.join(line.split())
            tuple = memory_line_parse(line)
            if tuple is not None:
                if re.match('TCM[a-zA-Z0-9_]*', tuple[0]):
                    if tuple[0]=="TCM":
                        tcm=tuple[2]
                    elif tuple[0]=="TCM_SEC_SHM":
                        #tcm+=tuple[2]
                        pass
                    elif tuple[0]=="TCM_TFM":
                        #tcm+=tuple[2]
                        pass
                if re.match('VRAM[a-zA-Z0-9_]*', tuple[0]):
                    if tuple[0]=="VRAM":
                        psram=tuple[2]
                    elif tuple[0]=="VRAM_TFM" :
                        #psram_tfm=tuple[2]
                        pass
                    elif tuple[0]=="VRAM_WFFW":
                        #psram_wffw=tuple[2]
                        pass
                    elif tuple[0]=="VRAM_BTFW":
                        #psram_btfw=tuple[2]
                        pass
                elif re.match('VSYSRAM[a-zA-Z0-9_]*', tuple[0]):
                    if tuple[0]=="VSYSRAM":
                        sysram=tuple[2]
                    elif tuple[0]=="VSYSRAM_TFM" :
                        #sysram_tfm=tuple[2]
                        pass
    m_zone.append(sysram)
    m_zone.append(tcm)
    m_zone.append(psram)
    # m_zone.append(psram_wffw)
    # m_zone.append(psram_btfw)

    if prj!="bootloader" and elf_file!="":
        elf_file=out_folder+elf_file
        run_cmd=tool+" -h "+elf_file
        log_out  = os.popen(run_cmd).readlines()
        status =get_memory_status(log_out,m_zone)

    if build_judge_fail==1:
        if prj!="hqa_desense":
            print("--ERROR:build release:RTOS code size overflowed:",str(overflow)+" bytes")
        return False
    else:
        return True

if __name__ == '__main__':
    prj_repo=["prj_repo:"]*1 #actually using repo on out/.. folder
    active_repo=["active_repo:"]*1#judge by build time of .o file
    repo_mani=["manifest_repo:"]*1#actually using repo on hadron defined on manifest.xml.
    active_match_repo=["active_match_repo:"]*1
    backup_warning_files=["record_warning_file:"]*1
    define_list=["tag:"]*1
    last_build_time=0
    memory_srh_done= [""]*20
    btfw_2_1hdr_size=0
    wffw_emi_size=0

    current_pwd=os.getcwd()
    if (len(sys.argv)==2 and sys.argv[1]=="force_format"):
        print("force format start...!!")
        formating(current_pwd,"file_c_h","force_format")
        print("force format end...!!")
        sys.exit(0)

    if not os.path.exists(current_pwd+"/doc_internal"):
        sys.exit(0)

    if len(sys.argv)==3 and sys.argv[2]=="check_empty":
       sys.exit(0)

    if len(sys.argv)==4 and sys.argv[2]=="clean" and sys.argv[3]=="check_empty":
       sys.exit(0)

    if sys.argv[2].find("-o")!=-1 or  sys.argv[2].find("bl")!=-1:
       sys.exit(0)
    if  sys.argv[2].find("-f")!=-1:
        feature_f=sys.argv[2][3::]
        sys.argv[2]=sys.argv[1]
    else:
        feature_f="feature.mk"

    record_repo=current_pwd+"/out/mt7933_hdk/record_repo.txt"
    if (len(sys.argv)==4 and sys.argv[3]=="release") or (len(sys.argv)==5 and sys.argv[4]=="release"):
        out_folder=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/release/"
        buile_type="release"
    elif  (len(sys.argv)==4 and sys.argv[3]=="debug") or (len(sys.argv)==5 and sys.argv[4]=="debug"):
        out_folder=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/debug/"
        buile_type="debug"
    elif  (len(sys.argv)==4 and sys.argv[3]=="mfg") or (len(sys.argv)==5 and sys.argv[4]=="mfg"):
        out_folder=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/mfg/"
        buile_type="mfg"
    else:
        out_folder=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/"
        buile_type=""
    out_dir= out_folder+"obj"
    out_buildtime_log=out_folder+"log/build_time.log"
    out_build_err_log=out_folder+"log/err.log"

    if not os.path.exists(record_repo):
        try:
            create_prj_f = open(record_repo, 'a' ,buffering=1)
            parse_minifest_repo("create_prj_file",create_prj_f,sys.argv[2])
        except:
            pass
    cfg = configparser.ConfigParser()
    cfg.read(record_repo)

    try:#for old ver. no feature of warning_files and RD getch code by repo. sync cmd.
        backup_data=cfg.get(sys.argv[2],"warning_files")
        backup_warning_files=transform(backup_data,0)
    except Exception as e:
        del_f="rm -rf "+record_repo
        os.system(del_f)

    if not os.path.exists(record_repo):
        create_prj_f = open(record_repo, 'a' ,buffering=1)
        parse_minifest_repo("create_prj_file",create_prj_f,sys.argv[2])

    clean_index=cfg.get(sys.argv[2],"clean_index")
    n = len(sys.argv)

    if len(sys.argv)==4:
        if sys.argv[3]=="check_empty":
            if os.path.exists(out_dir)!=True or os.path.exists(out_buildtime_log)!=True:
                if clean_index!="empty":
                    cfg.set(sys.argv[2],"clean_index","with_record_repo")
                    cfgfile = open(record_repo,'w')
                    cfg.write(cfgfile)

            backup_data=cfg.get(sys.argv[2],"warning_files")
            if backup_data!="":
                backup_warning_files=transform(backup_data,0)
                modify_files_mtime(backup_warning_files)

            sys.exit(0)
        if sys.argv[3]=="check_build":
            backup_data=cfg.get(sys.argv[2],"warning_files")
            backup_active_repo=transform(backup_data,0)
            len1=len(backup_active_repo)
            if len1>1:
                sys.exit(1) #notice of can't have any log message before this line.
            else:
                check_size_limit=cfg.get(sys.argv[2],"rtos_reserved_limit_inx")
                if check_size_limit=="fail":
                    sys.exit(1)
                else:
                    sys.exit(0) #notice of can't have any log message before this line.
    print("clean_inx",clean_index,sys.argv[2],n)
    parse_minifest_repo("get_data","",sys.argv[2])
    #print(len(repo_mani),repo_mani)
    if clean_index=="empty":
        print("---empty_state")
        parse_prj_all_repo(out_dir,sys.argv[2])
        cfg.set(sys.argv[2],"prj_repo",str(prj_repo))
        cfg.set(sys.argv[2],"clean_index","with_prj_repo")
        run_format_repo(prj_repo,0)

    elif clean_index=="with_active_repo" or clean_index=="with_prj_repo":
        print("---by build time ")
        last_build_time=cfg.get(sys.argv[2],"last_build_time")
        if last_build_time!="":
            last_build_time=int(last_build_time,10)
        else:
            last_build_time=0
        timeString=parse_keyword_file("Start Build:",out_buildtime_log,13)
        struct_time = time.strptime(timeString, "%a %b %d %H:%M:%S CST %Y")
        build_time_start = int(time.mktime(struct_time))
        timeString=parse_keyword_file("End Build:",out_buildtime_log,11)
        struct_time = time.strptime(timeString, "%a %b %d %H:%M:%S CST %Y")
        build_time_end = int(time.mktime(struct_time))
        cfg.set(sys.argv[2],"last_build_time",str(build_time_start))
        #print("build time",build_time_start,build_time_end)
        filter_repo_by_build_time(out_dir,build_time_start,build_time_end)

        #append history when match.
        backup_data=cfg.get(sys.argv[2],"active_repo")
        backup_active_repo=transform(backup_data,0)
        len1=len(backup_active_repo)
        for i in range(1,len1,1):
            print("backup:",i,backup_active_repo[i])
        #print("active:",active_repo)

        run_format_repo(active_repo,1)
        #print("match:",active_match_repo)
        if len(active_match_repo)>1:#tag + repo
            backup_active_repo=list_edit(active_match_repo,backup_active_repo,"add")
            cfg.set(sys.argv[2],"active_repo",str(backup_active_repo))
        cfg.set(sys.argv[2],"clean_index","with_active_repo")
        # if (len(active_repo)-len(active_match_repo))>1:#detect modify .h case, no append to history
        #     repo_tmp1=["chk_h_repo:"]*1
        #     repo_add = current_pwd+"/driver/chip/inc"
        #     repo_tmp1.append(repo_add)
        #     repo_add = current_pwd+"/driver/chip/mt7933/inc"
        #     repo_tmp1.append(repo_add)
        #     repo_add = current_pwd+"/middleware/MTK/audio/inc"
        #     repo_tmp1.append(repo_add)
        #     run_format_repo(repo_tmp1,1)

    elif clean_index=="with_record_repo":
        print("---by record data")
        backup_data=cfg.get(sys.argv[2],"active_repo")
        if 1:#backup_data=="":#temporary disable form history repo.
            backup_data=cfg.get(sys.argv[2],"prj_repo")
            if backup_data=="":
                parse_prj_all_repo(out_dir,sys.argv[2])
                cfg.set(sys.argv[2],"prj_repo",str(prj_repo))
                cfg.set(sys.argv[2],"clean_index","with_prj_repo")
                run_format_repo(prj_repo,0)
            else:
                backup_prj_repo=transform(backup_data,0)
                cfg.set(sys.argv[2],"clean_index","with_prj_repo")
                run_format_repo(backup_prj_repo,0)
        else:
            backup_active_repo=transform(backup_data,0)
            cfg.set(sys.argv[2],"clean_index","with_active_repo")
            run_format_repo(backup_active_repo,0)
    os.chdir(current_pwd)
    #build error/warnig judge by repo.
    backup_data=cfg.get(sys.argv[2],"prj_repo")
    backup_prj_repo=transform(backup_data,1)
    backup_data=cfg.get(sys.argv[2],"warning_files")
    backup_warning_files=transform(backup_data,0)

    if judge_build_pass(out_build_err_log,backup_prj_repo)!=True:
        out_build_file=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/*.bin"
        del_f="rm -rf "+out_build_file
        os.system(del_f)
        out_build_file=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/*.sgn"
        del_f="rm -rf "+out_build_file
        os.system(del_f)
        out_build_file=current_pwd+"/out/mt7933_hdk/"+sys.argv[2]+"/*.elf"
        del_f="rm -rf "+out_build_file
        os.system(del_f)
    else:
        print("===============================================================")
        print("build type:",buile_type)
        if memory_usage_lmt_chk(current_pwd,sys.argv[2],buile_type,out_folder,feature_f)!=True:
            if buile_type=="release":
                cfg.set(sys.argv[2],"rtos_reserved_limit_inx","fail")
        else:
            if buile_type=="release":
                cfg.set(sys.argv[2],"rtos_reserved_limit_inx","pass")

    cfg.set(sys.argv[2],"warning_files",str(backup_warning_files))
    cfgfile = open(record_repo,'w')
    cfg.write(cfgfile)



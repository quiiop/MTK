#!/usr/bin/env python3
import sys, os,datetime,shutil
from optparse import OptionParser
import time

class dependency_chk():
    def __init__(self):
        self.list_dep_2d_inv_index=0 #for lower layer child
        self.list_dep_2d_inv=[[] for i in range(100)]
        self.list_dep_2d_invf_index=0 #for top layer patent
        self.list_dep_2d_invf=[[] for i in range(100)]

    def dep_shr_keyword(self,inp_file,key_word):
        line_no=1
        with open(inp_file,'r') as in_f:
            for line in in_f:
                new_line=line.strip()
                if new_line.find(key_word,0)!=-1:
                    i=new_line.find("=",0)
                    content=new_line[0:i+1].strip()#content: MTK_XX=
                    content1=line[i+1::].strip()#content: y/n or others
                    content=content.replace(" ","")
                    return content,content1,line_no
                line_no+=1
        return "","",""

    def trn_list(self,inp_file):#transfer line content to list.
        list_dep_2d=[[] for i in range(100)]
        list_dep_2d_eq=[[] for i in range(100)]
        list_dep_2d_not=[[] for i in range(100)]

        list_data= []
        list_dep_1d= []
        #list_not= []
        f= open(inp_file, "r")
        i=0
        j=0
        k=0
        for line in f.readlines():
            dep=line.find("@",0)
            eq=line.find("~",0)
            _not=line.find("!",0)
            m=line.find("=",0)
            if m!=-1:
                list_data.append(line[0:m].strip())
            if dep!=-1:
                list_dep_2d[i].append(line[0:m].strip())
                while True:
                    dep_e=line.find("@",dep+1)
                    empty=line.find(" ",dep+1)
                    if dep_e!=-1:
                        list_dep_2d[i].append(line[dep+1:dep_e].strip())
                        list_dep_1d.append(line[dep+1:dep_e].strip())
                        dep=dep_e
                    else:
                        list_dep_2d[i].append(line[dep+1:empty].strip())
                        list_dep_1d.append(line[dep+1:empty].strip())
                        break
                i+=1
            if eq!=-1:
                list_dep_2d_eq[j].append(line[0:m].strip())
                while True:
                    eq_e=line.find("~",eq+1)
                    empty=line.find(" ",eq+1)
                    if eq_e!=-1:
                        list_dep_2d_eq[j].append(line[eq+1:eq_e].strip())
                        eq=eq_e
                    else:
                        list_dep_2d_eq[j].append(line[eq+1:empty].strip())
                        break
                j+=1

            if _not!=-1:
                list_dep_2d_not[k].append(line[0:m].strip())
                while True:
                    _not_e=line.find("!",_not+1)
                    empty=line.find(" ",_not+1)
                    if _not_e!=-1:
                        list_dep_2d_not[k].append(line[_not+1:_not_e].strip())
                        _not=_not_e
                    else:
                        list_dep_2d_not[k].append(line[_not+1:empty].strip())
                        break
                k+=1
        return list_data,list_dep_2d,list_dep_1d,list_dep_2d_eq,list_dep_2d_not

    def srh_update(self,type,list_d,content_head,logical,feature_file,content,line_no):
        global check_result
        try:
            for k in range(0,len(list_d),1):
                if list_d[k][0]==content_head:
                    #print("shr update=>",k,list_d[k][0])
                    if len(list_d[k])>=1:
                        for j in range(1,len(list_d[k]),1):
                            dep_content,dep_bool,line_srh=self.dep_shr_keyword(feature_file,list_d[k][j] )
                            if dep_content!="":#filter wrong define
                                if type=="DEP":
                                    #print("----parent_all_to_y",dep_content,dep_bool,logical.strip())
                                    if dep_bool!="" and dep_bool!=logical.strip():
                                        temp_str="[dependency issue]:line:"+str(line_srh)+":"+dep_content+"it should set as:"+logical+",due to line:"+str(line_no)+":"+content
                                        print(temp_str)
                                        check_result=False
                                elif type=="EQ":
                                    #print("--eq_to",logical,dep_content,dep_bool)
                                    if dep_bool!="" and dep_bool!=logical.strip():
                                        temp_str="[dependency equal issue]:line:"+str(line_srh)+":"+dep_content+"it should set as:"+logical+",due to line:"+str(line_no)+":"+content
                                        print(temp_str)
                                        check_result=False
                                        #print("definition equal issue,it should be:",logical,dep_content)
                                elif type=="DEP_NOT":
                                    #print("--_not_to",logical,dep_content,dep_bool)
                                    # if dep_bool!="" and dep_bool!=logical.strip():
                                    #     temp_str="[dependency not issue]:line:"+str(line_srh)+":"+dep_content+"it should set as:"+logical+",due to line:"+str(line_no)+":"+content
                                    #     print(temp_str)
                                    if dep_bool!="" and (logical.strip()=="n" and dep_bool=="y"):
                                        temp_str="[dependency not issue]:line:"+str(line_srh)+":"+dep_content+"it should set as:"+logical+",due to line:"+str(line_no)+":"+content
                                        print(temp_str)
                                        check_result=False
                                else:
                                    print("no support!!")
                                    return

                    break
        except Exception as e:
            #print ("try_except:",str(e))
            pass

    def gen_feature_def(self,check_prj,feature):#
        global check_result
        current_pwd=os.getcwd()
        feature_file=current_pwd+"/project/mt7933_hdk/apps/"+check_prj+"/GCC/"+feature
        feature_com_dis=current_pwd+"/project/mt7933_hdk/apps/bga_favor_build/source/feature_com_dis.mk"
        feature_common=current_pwd+"/project/mt7933_hdk/apps/bga_favor_build/source/feature_com_en.mk"

        # feature_com_dis=current_pwd+"/tools/scripts/build/feature_com_dis.mk"
        # feature_common=current_pwd+"/tools/scripts/build/feature_com_en.mk"
        feature_depence=current_pwd+"/out/mt7933_hdk/"+check_prj+"/feature_dep.mk"
        list_dis,no_use1,no_use2,no_use3,no_use4=self.trn_list(feature_com_dis)
        list_c,no_use1,no_use2,no_use3,no_use4=self.trn_list(feature_common)
        list_c+=list_dis
        list_d,list_d1,no_use1,list_d1_eq,list_d1_not=self.trn_list(feature_depence)
        #print("list_c",list_c)
        #print("list_d",list_d)
        #print("list_not",list_not)
        #print("list_d1",list_d1)
        #print("lens_",len(list_d1))
        #print("list_d1_eq",list_d1_eq)
        #print("lens",len(list_d1_eq))
        #print("list_d1_not",list_d1_not)
        #print("lens",len(list_d1_not))
        feature_f= open(feature_file, "r")
        line_no=1
        for content in feature_f.readlines():
            mark=content.find("#",0,1)
            if mark==-1:
                i=content.find("=",0)
                if i!=-1:
                    content0=content[0:i+1]#MTK_XXX=
                    content1=content[i+1::].strip()#content: y/n or others
                    content_head=(content[0:i].strip())
                else:
                    content0=""
                    content1=""
                    content_head=""
                content=content.replace(" ","")
                if content_head in list_d and not content_head in list_c :#filter commond part
                    #dependency child part:
                    if content1=="n":
                        #print("--child_as_n",content0,content1)
                        for j1 in range(0,len(self.list_dep_2d_inv),1):#
                            try:
                                if content_head.find(self.list_dep_2d_inv[j1][0],0)!=-1:
                                    #print("lower layer_child",len(self.list_dep_2d_inv[j1]))
                                    for j2 in range(0,len(self.list_dep_2d_inv[j1]),1):#
                                        dep_content,dep_bool,line_srh=self.dep_shr_keyword(feature_file,self.list_dep_2d_inv[j1][j2] )
                                        #print("-----L child_all_to_n",dep_content,dep_bool)
                                        if dep_bool!="n" and dep_bool!="":
                                            temp_str="[dependency issue child]:line:"+str(line_srh)+":"+dep_content+"it should set as: n, due to line:"+str(line_no)+":"+content
                                            print(temp_str)
                                            check_result=False
                            except:
                                pass
                        self.srh_update("EQ",list_d1_eq,content_head," n",feature_file,content,line_no)
                        self.srh_update("DEP_NOT",list_d1_not,content_head," y",feature_file,content,line_no)

                    elif content1=="y":
                        #print("--child_as_y",content0,content1)
                        self.srh_update("DEP",list_d1,content_head," y",feature_file,content,line_no)
                        self.srh_update("EQ",list_d1_eq,content_head," y",feature_file,content,line_no)
                        self.srh_update("DEP_NOT",list_d1_not,content_head," n",feature_file,content,line_no)
                else:#dependency parent part
                    if i!=-1 and not content_head in list_c :#filter commond part
                        created=0
                        for j in range(0,len(list_d1),1):
                            if content_head in list_d1[j]:
                                if created==0:
                                    created=1
                                    #print("--parent_as",content0,content1)

                                if content1=="n":
                                    dep_content,dep_bool,line_srh=self.dep_shr_keyword(feature_file,list_d[j])
                                    if dep_content!="":#filter wrong define
                                        #dep_content+=" n"
                                        #print("--child_all_to_n",dep_content,dep_bool)
                                        if dep_bool!="n" and dep_bool!="":
                                            temp_str="[dependency issue]:line:"+str(line_srh)+":"+dep_content+"it should set as: n, due to line:"+str(line_no)+":"+content
                                            print(temp_str)
                                            check_result=False
                                        #search lower layer_child
                                        for j1 in range(0,len(self.list_dep_2d_inv),1):#
                                            try:
                                                if dep_content.find(self.list_dep_2d_inv[j1][0],0)!=-1:
                                                    #print("lower layer_child",len(list_dep_2d_inv[j1]))
                                                    for j2 in range(0,len(self.list_dep_2d_inv[j1]),1):#
                                                        dep_content,dep_bool,line_srh=self.dep_shr_keyword(feature_file,self.list_dep_2d_inv[j1][j2] )
                                                        #print("-----child_all_to_n",dep_content,dep_bool)
                                                        if dep_bool!="n" and dep_bool!="":
                                                            #print("error2 define set,it should be n:",dep_content)
                                                            temp_str="[dependency issue]:line:"+str(line_srh)+":"+dep_content+"it should set as: n, due to line:"+str(line_no)+":"+content
                                                            print(temp_str)
                                                            check_result=False
                                            except:
                                                pass

                                    self.srh_update("EQ",list_d1_eq,content_head," n",feature_file,content,line_no)
                                    self.srh_update("DEP_NOT",list_d1_not,content_head," y",feature_file,content,line_no)
                                else:
                                    self.srh_update("EQ",list_d1_eq,content_head," y",feature_file,content,line_no)
                                    self.srh_update("DEP_NOT",list_d1_not,content_head," n",feature_file,content,line_no)
            line_no+=1

    def srh_child(self,f,list_inp):
        global list_dep_2d_inv,list_dep_2d_inv_index
        global list_dep_2d_invf,list_dep_2d_invf_index
        self.list_dep_2d_inv[self.list_dep_2d_inv_index].append(list_inp)
        raw_f= open(f, "r")
        src_find0=0
        src_find1=0
        for line_raw in raw_f.readlines():
            mark=line_raw.find("#",0,1)
            if mark!=-1:
                continue
            i=line_raw.find("=",0)
            dep=line_raw.find("@",0)

            if i!=-1 and dep!=-1:
                if line_raw[i::].find("@"+list_inp,0)!=-1:
                    self.list_dep_2d_inv[self.list_dep_2d_inv_index].append(line_raw[0:i].strip())
                    src_find0=1
                if line_raw[0:i+1].find(list_inp,0)!=-1:
                    self.list_dep_2d_invf[self.list_dep_2d_invf_index].append(list_inp)
                    src_find1=1
                    while True:
                        dep_e=line_raw.find("@",dep+1)
                        empty=line_raw.find(" ",dep+1)
                        if dep_e!=-1:
                            self.list_dep_2d_invf[self.list_dep_2d_invf_index].append(line_raw[dep+1:dep_e].strip())
                            dep=dep_e
                        else:
                            #list_dep_2d_invf[list_dep_2d_invf_index].append(line_raw[dep+1::].strip())
                            self.list_dep_2d_invf[self.list_dep_2d_invf_index].append(line_raw[dep+1:empty].strip())
                            break
        if src_find0==1:
            self.list_dep_2d_inv_index+=1
        if src_find1==1:
            self.list_dep_2d_invf_index+=1

    def gen_raw_dep_data(self,check_prj,feature):
        current_pwd=os.getcwd()
        #print("--raw--",current_pwd,check_prj)
        feature_file=current_pwd+"/project/mt7933_hdk/apps/"+check_prj+"/GCC/"+feature
        #feature_raw_depence=current_pwd+"/tools/scripts/build/feature_dep_raw.mk"
        feature_raw_depence=current_pwd+"/project/mt7933_hdk/apps/bga_favor_build/source/feature_dep_raw.mk"
        feature_depence=current_pwd+"/out/mt7933_hdk/"+check_prj+"/feature_dep.mk"
        list_d,list_d1,no_use1,list_d1_eq,list_d1_not=self.trn_list(feature_raw_depence)
        #print("list_d_raw",list_d,list_d1)

        if os.path.exists(feature_depence):
            os.remove(feature_depence)
        feature_depence = open(feature_depence, 'a' ,buffering=1)
        feature_f= open(feature_file, "r")
        for line in feature_f.readlines():#sort and recode to feature_dep.mk"
            mark=line.find("#",0,1)
            if mark==-1:
                i=line.find("=",0)
                if i!=-1:
                    content_head=(line[0:i].strip())
                    if content_head in list_d:#
                        raw_f= open(feature_raw_depence, "r")
                        for line_raw in raw_f.readlines():
                            mark1=line_raw.find("#",0)
                            if mark1==-1:
                                j=line.find("=",0)
                                if line_raw[0:j].find(content_head,0)!=-1:
                                    feature_depence.write(line_raw)
                                    break
        list_dep_inv= []
        raw_f= open(feature_raw_depence, "r")
        for line in raw_f.readlines():
            i=line.find("=",0)
            dep=line.find("@",0)
            mark=line.find("#",0,1)
            if mark!=-1:
                continue
            if dep!=-1:
                while True:
                    dep_e=line.find("@",dep+1)
                    empty=line.find(" ",dep+1)
                    if dep_e!=-1:
                        if not (line[dep+1:dep_e].strip() in list_dep_inv):
                            list_dep_inv.append(line[dep+1:dep_e].strip())
                            self.srh_child(feature_raw_depence,line[dep+1:dep_e].strip())
                        dep=dep_e

                    else:
                        if not (line[dep+1:empty].strip() in list_dep_inv):
                            list_dep_inv.append(line[dep+1:empty].strip())
                            self.srh_child(feature_raw_depence,line[dep+1:empty].strip())
                        break
        #print("2d_inv-----",self.list_dep_2d_inv)
        #print("2d_invf-----",self.list_dep_2d_invf)

    def judge_fatal_err(self,check_prj):
        myfile=open("out/mt7933_hdk/"+check_prj+"/log/err.log","r")
        for myline in myfile.readlines():
            err=myline.find("fatal error:",0)
            if err!=-1:
                i=myline.find(".c",0,err)
                j=myline.find(".h",0)
                if i!=-1 or j!=-1:
                    #file_c=myline[0:i+2].strip()
                    return myline
        return ""

    def start(self,check_prj,feature):
        global check_result
        self.gen_raw_dep_data(check_prj,feature)
        fatal_err=self.judge_fatal_err(check_prj)
        if fatal_err!="":
            #self.gen_raw_dep_data(check_prj,feature)
            print("-----dependency check-----")
            self.gen_feature_def(check_prj,feature)
            print("[Also reference feature dependency: out/mt7933_hdk/"+check_prj+"/feature_dep.txt]")
if __name__ == '__main__':
    check_result=True
    if sys.argv[2].find("-o")!=-1 or  sys.argv[2].find("bl")!=-1:
        sys.exit("0")
    if  sys.argv[2].find("-f")!=-1:
        feature_f=sys.argv[2][3::]
        sys.argv[2]=sys.argv[1]
    else:
        feature_f="feature.mk"
    dep_chk=dependency_chk()
    dep_chk.start(sys.argv[2],feature_f)



from FBTool_v2p84_gui import *

if __name__ == '__main__': 
    global sel_clr_all,dis_check_box_list,check_v,check_v_state,init_bar,config_list_data
    config_list_data=[]  
    init_bar=0
    sel_clr_all = 0
    dis_check_box_list=[""]*1
    check_v={}
    check_v_state={}
    root = tk.Tk()
    FBTool(root)
    root.mainloop()

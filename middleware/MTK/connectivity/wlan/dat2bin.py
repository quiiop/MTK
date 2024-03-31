#!/usr/bin/python3
import struct
import re
import string
import argparse


# All Supported Phy Mode supported in V3
AX_PHYMODE_TBL = ('cck','ofdm','ht20',
 	'ht40',		\
 	'vht20', 	\
 	'vht40', 	\
 	'vht80', 	\
 	'vht160', \
 	'ru26', 'ru52', 'ru106', 'ru242', \
 	'ru484', \
 	'ru996', \
 	'ru996x2' \
 	)
# For MT79XX AX, BW20 Only
mt79xx_section_name_tuple = ('cck','ofdm','ht20', \
 	'vht20', 	\
 	'ru26', 'ru52', 'ru106', 'ru242', \
 	)

# The script transform <Ver:03> to from binary with extra information for MT79XX series
# <Ver:03> is an extension for Ver:02, which transform FILE to binary form and add
# extra information on the binary. eg. PHY_MODE, number of table, offset
COMPATIBLE_VER_CODE = 2
# Ver:03 format
# HEADER {
#	uint8 VER;
#	uint8 NUM_COUNTRY; /* number of TLV value following */
#	uint32 LEN_TOTAL_TBL; /* len for All PWR TBL */
#	uint32 LEN_SINGLE_TBL; /* len for single PWR TBL */
#	uint32 SUPPORTED_PHYMODE; /* Supported PHYMODE */
#	uint32 SUPPORTED_PHYMODE; /* Supported PHYMODE */
# }
MAX_COUNTRY_CODE_LEN =  4
VER_CODE = 3

#DO NOT modify, Rate table entry parameter define
sku_cck_num = 4
sku_ofdm_num = 8
sku_ht20_num = 8
sku_ht40_num = 9
sku_vht20_2_num = 12
sku_vht40_2_num = 12
sku_vht80_2_num = 12
sku_vht160_2_num = 12
sku_ru26_num = 12
sku_ru52_num = 12
sku_ru106_num = 12
sku_ru242_num = 12
sku_ru484_num = 12
sku_ru996_num = 12
sku_ru996x2_num = 12

#Please align the PHY mode supported
# eg.
AX_PHYMODE_SECTION_NUM_TUPLE = (sku_cck_num, sku_ofdm_num, sku_ht20_num, sku_ht40_num, sku_vht20_2_num, \
    sku_vht40_2_num, sku_vht80_2_num, sku_vht160_2_num, sku_ru26_num, sku_ru52_num, \
	sku_ru106_num, sku_ru242_num, sku_ru484_num, sku_ru996_num, sku_ru996x2_num)
#section_num_tuple = [sku_cck_num, sku_ofdm_num, sku_ht20_num, 
#	sku_vht20_2_num, \
#	sku_ru26_num, sku_ru52_num, sku_ru106_num, sku_ru242_num	\
#	]

#DO NOT modify, All channel for 2G/5G band
wf_2g_ch_tuple = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)
wf_5g_ch_tuple = (36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 100, \
    102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 132, 134, \
        136, 138, 140, 142, 144, 149, 151, 153, 155, 157, 159, 161, 165)
# Number of Power Value element in a row. DO NOT modify it. If you do not familiar with the format
element_num = 12
wf_ch_tuple = wf_2g_ch_tuple + wf_5g_ch_tuple
wf_ch_num = len(wf_ch_tuple)

log = open("dbg.log", "w")
def print_ext(log_data, only_file=False):
	if only_file == False:
		print(log_data)
	log.write(str(log_data)+"\n")
	return 0
#split the result of all Tx Power table into group
def split_by_country(res, hdr_info):
	tbl_st = 0
	tbl = []
	for i in range(len(res)):
		if res[i][0] == '[':
			if tbl_st == 0:
				tbl_st = i
			else:
				# find begin next country
				tbl.append(res[tbl_st:i])
				tbl_st = i
	tbl.append(res[tbl_st:i+1])
	return tbl

#l1 is raw data, put coutry code in l1 country code append and write to file_name 
def write_coutry_code(l1, file_name) :
    country_code = []
    for c in l1[0]:
        if (c == '[') or (c == ']'):
            continue
        else :
            country_code.append(c)
    #print_ext("Country Code: ")
    #print_ext(country_code)
    
    fw = open(file_name, "ab+")
    for c in country_code :
        #print_ext(c)
        if c.isdigit() :
            #print_ext(c)
            #print_ext(ord(c))
            code = struct.pack("b", ord(c))	#koko
            #print_ext(code)
        else :
            #print_ext(c)
            #print_ext(ord(c))
            code = struct.pack("B", ord(c))
            #print_ext(code)
        fw.write(code)
    if len(country_code) == 2 :
        pad = struct.pack("B", 0)
        fw.write(pad)
        fw.write(pad)
    fw.close()

#l1 sec_idx_scope , l2 is result, sec_nm is name
def find_sec_idx(l1, l2, sec_nm):
    for i in range(len(l2)) :
        if sec_nm in l2[i] :
            if l1[0] == 0 :
                print_ext("Match ["+sec_nm+"] at : "+str(i)+" with [" + l2[i], True)
                l1[0] = i+1
            elif l1[1] != 0:
                break
            else:
                print_ext("Match ["+sec_nm+"] at : "+str(i)+" with [" + l2[i], True)
                #fix ru996 bug
                if sec_nm == "ru996" :
                    l1[1] = i
                else :
                    l1[1] = i
        #fix ht20/ht40/ru996 error
        elif l1[1] != 0 :
            break
        else :
            continue
    #print_ext(l1)
	
#fetch country tx pwr limit data
def fetch_sec_data(result1, section_num, section_name_tuple, tx_pwr_limit_val):
	print_ext("fetch_sec_data> ", True)
	for sec_idx in range(section_num) :
		sec_name = section_name_tuple[sec_idx]
		print_ext("Start Fetch Section: " + str(sec_name), True)
		sec_idx_scope = [0, 0]
		find_sec_idx(sec_idx_scope, result1, sec_name)
		#split result by section    
		print_ext("Range from " +str(sec_idx_scope[0])+ " to " + str(sec_idx_scope[1]), True)
		sec_ch_list = result1[sec_idx_scope[0]:sec_idx_scope[1]]
		print_ext(str(sec_ch_list), True)
		#per channel
		for ch_idx in range(len(sec_ch_list)) :
			element = sec_ch_list[ch_idx].split(',')
			#print_ext(element)
			ch = re.sub("\D", "", sec_ch_list[ch_idx].split(',')[0])
			print_ext("Fetch Channel: " + str(ch) +" " + str(element), True)
			wf_ch_idx = wf_ch_tuple.index(int(ch))
			#print_ext("CH: "+str(sec_ch_list(wf_ch_idx))+", ELEM: " + str(len(element))) 
			#per element
			for ele_idx in range(len(element)) :
				if ele_idx == 0 :
					continue
				else :
					tx_pwr_limit = element[ele_idx].strip()
					tx_pwr_limit_val[wf_ch_idx][sec_idx][ele_idx-1] = int(tx_pwr_limit)
	for idx in range(len(tx_pwr_limit_val)):
		ch_info = tx_pwr_limit_val[idx]
		log.write("Channel Info["+str(wf_ch_tuple[idx])+"]\n")
		for idx_section in range(len(ch_info)):
			pwr_info = ch_info[idx_section]
			log.write(str(section_name_tuple[idx_section])+":"+str(pwr_info)+"\n")
			#print_ext (pwr_info)
	return tx_pwr_limit_val

#write country tx pwr limit data
def write_sec_data(OUTPUT_BIN, wf_ch_num, section_num, section_num_tuple, tx_pwr_limit_val):
	fw = open(OUTPUT_BIN, "ab+")
	write_len = 0
	for c in range(wf_ch_num):
		print_ext("Write to CH_IDX: " + str(wf_ch_tuple[c]) +  ", Start@Byte: " + str(write_len), True)
		for s in range(section_num):
			for e in range(section_num_tuple[s]):
				a = struct.pack("B", tx_pwr_limit_val[c][s][e])
				write_len += 1
				fw.write(a)
	fw.close()
	return write_len

def phymode_supported(phymode_tbl, supported_tbl):
	supported_bitwise = 0xffffffff

	if len(phymode_tbl) < len(supported_tbl):
		print_ext("[ERROR]PHYMODE table should be the super-set of supported table.")
		return supported_bitwise

	supported_bitwise = 0
	for i in range(len(phymode_tbl)):
		for j in range(len(supported_tbl)):
			if supported_tbl[j] == phymode_tbl[i]:
				#print_ext("Match[" + str(i)+"]: " + phymode_tbl[i] +"/"+supported_tbl[j])
				supported_bitwise |= 1 << i
				continue
	return supported_bitwise

def construct_num_section_tuple(phymode, ref_tuple):
	ret = []
	for i in range(len(ref_tuple)):
		if 1<<i & phymode:
			ret.append(ref_tuple[i])
	return ret

def write_v3_hdr(OUTPUT_BIN, tbl, phy_mode, section_num, section_num_tuple, wf_ch_num=wf_ch_num):
	# Ver:03 format
	# HEADER {
	#	uint8 VER; - write in previous stage
	#	uint8 NUM_COUNTRY; /* number of TLV value following */
	#	uint32 LEN_TOTAL_TBL; /* len for All PWR TBL */
	#	uint32 LEN_SINGLE_TBL; /* len for single PWR TBL */
	#	uint32 SUPPORTED_PHYMODE; /* Supported PHYMODE */
	# }
	# #define MAX_COUNTRY_CODE_LEN 4
	hdr_len = 0
	
	print_ext ("\nWrite V3 hdr to binary ...")
	print_ext ("Number of Power Table by Country: " +str(len(tbl)))
	#print_ext("[Code][Lines of Power Value]")
	#for idx in range(len(tbl)):
	#	print_ext(pwr_tbl[idx][0] + "[" +str(len(pwr_tbl[idx])) + "]")

	fw = open(OUTPUT_BIN, "ab+")

	# 1. pack num of country 1 byte
	hdr = struct.pack("B", len(tbl))	# current support
	fw.write(hdr)
	hdr_len += len(hdr)
	# 2. Fetch length of table and sum
	write_len = 0
	for c in range(wf_ch_num):
		for s in range(section_num):
			for e in range(section_num_tuple[s]):
				write_len += 1
	#write_len += MAX_COUNTRY_CODE_LEN
	tot_len = write_len * len(tbl)
	print_ext ("Len of Total Power Tbl (BYTE): " + str(tot_len))
	hdr = struct.pack("I", tot_len)	# current support
	fw.write(hdr)
	hdr_len += len(hdr)
	# 3. Write Single table length
	print_ext ("Len of Single Power Tbl (BYTE): " + str(write_len))
	hdr = struct.pack("I", write_len)	# current support
	print_ext ("Len of Single Power Tbl (0x): " + str(len(hdr)))
	fw.write(hdr)
	hdr_len += len(hdr)
	# 4. Write Supported PHYMODE 
	print_ext ("Supported phymode(base10):" + str(phy_mode))
	hdr = struct.pack("I", phy_mode)	# current support
	print_ext ("PHYMODE supported(BITWISE): " + str(hdr))
	fw.write(hdr)
	hdr_len += len(hdr)
	hdr_len += 1
	print_ext ("HDR LEN: " + str(hdr_len))
	fw.close()
###############################################################################################		
if __name__ == '__main__':
	SRC_DAT = 'TxPwrLimit_MT79x1.dat'
	OUTPUT_BIN = "TxPwrLimit_MT7933.bin"
	hdr_info = []
	NUM_TBL = 0
	section_num_tuple = []
	section_name_tuple = []

	parser = argparse.ArgumentParser(description='TX Power Binary Generator')
	parser.add_argument('-dat', '--dat', help='Source TX power Table .dat')
	parser.add_argument('-out', '--out', help='Output Name of Power Table .bin')

	args = parser.parse_args()
	
	if args.dat is None:
		print_ext ("No specific Tx powr .dat source. Use default " + SRC_DAT)
	else:
		SRC_DAT = args.dat
	if args.out is None:
		print_ext ("No specific Tx powr .bin output. Use default " + OUTPUT_BIN)
	else:
		OUTPUT_BIN = args.out
	
	# 0. read and parse $SRC_DAT to result
	result = []
	fr = open(SRC_DAT, 'r') 
	for line in fr.readlines() :
		data = line.strip()
		if not len(data) or data.startswith('#') :       
			continue
		result.append(data)
	fr.close()
	
	print_ext ("\nSource dat: " + SRC_DAT)
	print_ext ("Output bin: " + OUTPUT_BIN)
	#print_ext(result)
	version = 0
	# 1 check ver
	print_ext("\nVersion Code Check ...")
	log.write("Output bin: " + OUTPUT_BIN + "\n")
	if "<Ver:" in result[0]:
		version = int(result[0].split(":")[1].split(">")[0])
	else:
		print_ext("No match Version code. Please check <Ver:X> is your first ligit line")
		print_ext(result[0])
		exit()

	print_ext ("Ver: " + str(version))
	if version != VER_CODE and version != COMPATIBLE_VER_CODE:
		print_ext("[ERROR]Only Generate Ver Code :" + str(VER_CODE))
		print_ext("[ERROR]If you want to use binary form of PWR TBL, please filled in <Ver:03>")
		exit()
		
	# 1. group tbl by country code
	pwr_tbl = split_by_country(result, hdr_info)
	
	# 1-1. Backward compatible for version 2
	if version == COMPATIBLE_VER_CODE:
		NUM_TBL = 2
		print_ext("old version ["+str(COMPATIBLE_VER_CODE)+"], only gen 2 TBL in binary")
	else:
		NUM_TBL = len(pwr_tbl)
	print_ext ("\nNumber of Power Table by Country: " +str(NUM_TBL))
	#print_ext("[Code][Lines of Power Value]")
	#for idx in range(NUM_TBL):
	#	print_ext(pwr_tbl[idx][0] + "[" +str(len(pwr_tbl[idx])) + "]")
	#	#print_ext(tbl[idx])
	
	# 2. Write Tx Power Table version
	print_ext ("\nWrite to Binary ...")
	fw = open(OUTPUT_BIN, "wb")
	bversion = struct.pack("B", version)
	print_ext("Write table version code ...")
	print_ext(bversion)
	fw.write(bversion)
	fw.close()
	
	# 2.1 Write V3 header if its V3 POWER TABLE
	if version == VER_CODE:
		section_name_tuple = mt79xx_section_name_tuple
		section_num = len(section_name_tuple)
		phymode = phymode_supported(AX_PHYMODE_TBL, section_name_tuple)
		section_num_tuple = construct_num_section_tuple(phymode, AX_PHYMODE_SECTION_NUM_TUPLE)
		print_ext(section_num_tuple)
		write_v3_hdr(OUTPUT_BIN, pwr_tbl, phymode, section_num, section_num_tuple)
	else:
		section_name_tuple = AX_PHYMODE_TBL
		section_num = len(section_name_tuple)
		section_num_tuple = AX_PHYMODE_SECTION_NUM_TUPLE

	print_ext ("\nRate type in table[" +str(section_num)+ ":"+str(len(section_num_tuple))+"]: ")
	print_ext (section_name_tuple)
	print_ext (section_num_tuple)
	print_ext ("Number of Channels: " +str(wf_ch_num))
	print_ext (wf_ch_tuple)

	tx_pwr_limit_val = [[[127]*element_num for i in range(section_num)] for j in range(wf_ch_num)]
	# 2-2. Write each country Table into file
	print_ext ("\nWrite country power table ...")
	print_ext ("[CODE][#_CH] : $WRITE BYTES")
	for idx in range(NUM_TBL):
		write_coutry_code(pwr_tbl[idx],OUTPUT_BIN)
		sec_data = fetch_sec_data(pwr_tbl[idx], section_num, section_name_tuple, tx_pwr_limit_val)
		#print_ext(sec_data)
		write_len = write_sec_data(OUTPUT_BIN, wf_ch_num, section_num, section_num_tuple, sec_data)
		print_ext(pwr_tbl[idx][0] + "[" +str(len(sec_data)) + "] : " + str(write_len))
	
log.close()

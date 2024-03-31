#!/usr/bin/env python
# -*- coding: utf-8 -*-

'use "python fread.py to execute this program"'

__author__='Greta Zhang'

import sys

def read_data_from_binary_file(filename, list_data):
	f = open(filename, 'rb')
	f.seek(0, 0)
	while True:
		t_byte = f.read(1)
		if len(t_byte) == 0:
			break
		else:
			list_data.append("0x%.2X" % ord(t_byte)) #ord to convert string of length 1 to int

def write_data_to_text_file(filename, list_data, data_num_per_line):
	f_output = open(filename, 'w+')
	f_output.write('const unsigned char hifi4dsp_load[] =\n')
	f_output.write('{\n ')
	if ((data_num_per_line <= 0) or data_num_per_line > len(list_data)):
		data_num_per_line = 16
		print('data_num_per_line out of range, use default value\n')
	for i in range(0, len(list_data)):
		if ((i != 0) and (i % data_num_per_line == 0)): #for multi of 16, new line
			f_output.write('\n ')
			f_output.write(list_data[i] + ', ')
		elif i == len(list_data) - 1:                   #last byte
			f_output.write(list_data[i] + '\n};')
			f_output.close()
		elif (i % data_num_per_line == 15):
			f_output.write(list_data[i] + ',')
		else:
			f_output.write(list_data[i] + ', ')
			#f_output.write('\n};')

list_data = []
#input_f = raw_input("Please input source bin file_name:")
#output_f = raw_input("Please input dest C file name:")
#data_num_per_line = input("Please input a num which indicates how many bytes in one line:")
#input_f = "hifi4dsp_load.bin"
input_f = sys.argv[1]
output_f = sys.argv[2]
data_num_per_line = 16
print 'input_f: %s' %input_f
print 'output_f: %s' %output_f
read_data_from_binary_file(input_f, list_data)
write_data_to_text_file(output_f, list_data, data_num_per_line)

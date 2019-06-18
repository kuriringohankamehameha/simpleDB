#!/usr/bin/env python

import sys
import random
import subprocess
import time

from subprocess import Popen, PIPE, run

def create_empty_list(caller = None):
	if(caller == None):
		return []
	else:
		return caller

def flatten(empty_list, list):
	for i in list:
		if(type(i) == list):
			flatten(empty_list, i)
		else:
			empty_list.append(i)

def flatten_list(list):
	#Flattens a list of lists (potentially)
	# Ex. [1, 2, [3,4,5]] -> [1,2,3,4,5]
	empty_list = create_empty_list()
	empty_list = flatten(empty_list, list)
	return empty_list

def insert_statement(num_records, max_num):
	list = []
	for i in range(num_records):
		j = random.randint(0 , max_num)
		k = random.choice(['A+','B+','O+','O-', 'AB+', 'AB-', 'A-', 'B-'])
		list.append('insert {} user{} person{}@example.com address{} {} {} {}\n'.format(j, j, j, j, j, j, k))

	return list

def insert_records(num_records, max_num):
	list_insert = insert_statement(num_records, max_num)
	string = ""
	for i in list_insert:
		string = string + i
	p = run(['./test', 'test_records.txt'], input=string+'.exit', encoding='ascii')


def create_list_to_delete(filename, num_to_delete, delete_from_first = True):
	#Creates a list of records from "filename" which can be deleted
	#The indices 0..num_to_delete are deleted
	list_to_delete = []
	with open(filename, "r") as file:

		"""
		if(delete_from_first == True):
			#Start from the first
			file.seek(0)
			for line in f:
				#Read line by line
				list_to_delete.append(line)
		"""

		#As of now, read the whole file
		data = file.readlines()
		if(delete_from_first == True):
			count = 0
			for i in data:
				if(count >= num_to_delete):
					break
				j = i.split(" ")
				list_to_delete.append(j[0])
				count += 1

	string = ""
	for i in list_to_delete:
		#print('delete',i)
		string = string + 'delete ' + i + '\n'
	p = run(['./test', 'test_records.txt'], input = string + '.exit', encoding='ascii')


def start_program(num_records):
	string = ""
	"""
	with open("records.txt",'rb') as input_file:
	    for i in input_file:
	        string=string+i.decode("ascii").strip()+'\n'
	"""
	list_insert = insert_statement(int(num_records), 1000)
	for i in list_insert:
		string = string + i
	string = string + 'select\n'
	string = string + '.exit'
	p = run(['./test', 'test_records.txt'], input=string, encoding='ascii')


def select_statement():
	p = run(['./test', 'test_records.txt'], input='select\n.exit', encoding='ascii')

def insert_particular_record(record):
	p = run(['./test', 'test_records.txt'], input='insert ' + record + '\n' + '.exit', encoding='ascii')

def delete_particular_record(key_num):
	p = run(['./test', 'test_records.txt'], input='delete ' + key_num + '\n.exit', encoding='ascii')

def record_count():
	p = run(['./test', 'test_records.txt'], input='.count\n' + '.exit', encoding='ascii')



if __name__ == '__main__':
	#list_to_delete = create_list_to_delete("records.txt", 3)
	#delete_statement(list_to_delete)
	#list_to_insert = insert_statement(5, 500)
	#print(list_to_insert)

	while(True):
		print('1 : Select records from the database')
		print('2 : Insert a particular record into the database')
		print('3 : Insert random records into the database')
		print('4 : Delete a particular record from the database')
		print('5 : Delete a bunch of records from a specified file, from the database')
		print('6 : Get the number of records currently in the database')
		print('7 : Exit the Program')
		print('\n')
		choice = int(input('Please select an option :\n'))

		if choice == 1:
			select_statement()
			print('\n')
		elif choice == 2:
			record = input('Enter a record to insert (Format: \"ID UserName EmailID\" ) :\n')
			insert_particular_record(record)
			print('\n')
		elif choice == 3:
			number = int(input('Enter number of records to be inserted (<1000) :\n'))
			if(number > 1000):
				raise ValueError("Number of records must be less than 1000")
			insert_records(number, 10000)
			print('\n')
		elif choice == 4:
			key_num = int(input('Enter ID of the record to be deleted\n'))
			delete_particular_record(str(key_num))
			print('\n')
		elif choice == 5:
			number = int(input('Enter number of records to be inserted (<1000) :\n'))
			if(number > 1000):
				raise ValueError("Number of records must be less than 1000")
			create_list_to_delete('test_records.txt', number, delete_from_first = True)
			print('\n')
		elif choice == 6:
			record_count()
		else:
			break




"""
	#Do this in another script

	try:
		num_records = sys.argv[1]
	except Exception as e:
		raise ValueError("Atleast one argument required")
	else:
		pass
	finally:
		pass

	start_program(int(num_records))
"""

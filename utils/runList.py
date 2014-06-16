#!/usr/bin/python

import os
import re
import sys
import subprocess
import shlex

## run bto on a list of .m files
## use as 
## ./runList.py LIST PATH
## LIST is the file that contains the list of .m files
##   should be something like
##      aatx.m
##      bicg.m
##
## PATH is optional and assumed to be matlabKernels/ when absent.
##   PATH points to the location of the .m files.  So if aatx.m is
##   the first line of LIST then the first .m file will be
##   PATH/aatx.m from current working directory.

## run a collection of .m files
flags = " -s ex -cem --run_all  -r3055:3055:1"

if len(sys.argv) == 2:
	path = "matlabKernels"
elif len(sys.argv) == 3:
	path = sys.argv[2]
else:
	print "Usage: runList.py LIST PATH"
	print "\tLIST is the file containing a list of .m files\n"
	print "\tPATH is the path to the .m files to be tested\n"
	sys.exit()

mFiles = sys.argv[1]

f = open(mFiles)
files = []
for x in f:
	if re.search("\.m",x):
		files.append(re.sub("\n","",x))
	
cnt = 0
fail = 0	
routines = 0

endEarly = 0
for x in files:
	routines += 1
	print "-----------------------",x,"----------------------"
	cmd = "./bin/btoblas " + path + "/" + x + flags
	print cmd
	args = shlex.split(cmd)
	run = subprocess.Popen(args,stdout=subprocess.PIPE,\
		stderr=subprocess.STDOUT, stdin=subprocess.PIPE, bufsize=1)
	run.stdin.write("\n")
	
	## parse output while the program is running	
	lcnt = 0
	run.poll()
	lfail = 0
	while run.returncode == None:
		run.poll()
		stdOut = run.stdout.readline()
			
		if re.search("Testing",stdOut):
			cnt += 1
			print stdOut,
		elif re.search("Test failed to",stdOut):
			lfail = 1
			print stdOut,
		elif re.search("PASSED",stdOut):
			lcnt += 1
			print stdOut,
		elif re.search("FAILED",stdOut):
			lfail = 1
			print stdOut,
		if re.search("error",stdOut):
			lfail = 1
			print stdOut,
		if re.search("Segmentation",stdOut):
			lfail = 1
			print stdOut,
		
		if lfail:
			break
			
	if run.returncode < 0:
		print "Bad return code", run.returncode
		lfail = 1
		
	if lfail:
		print "Error or failure"
		fail += 1
	else:
		print "Passed %d" % (lcnt)
	if endEarly and fail:
		break
	
print
print "-------------------------"	
print routines, "routines tested"
print cnt, "total tests"
print fail, "total failures"
print "-------------------------"

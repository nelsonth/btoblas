#!/usr/bin/python

import os
import re

d = os.listdir(".")

dr = []
for x in d:
	if re.search("lower", x) and re.search("\.dot", x):
		dr.append(x)

from threading import Thread

class myThread(Thread):
	def __init__(self, l):
		self.l = l
		Thread.__init__(self)
		
	def run(self):
		for x in self.l:
			out = re.sub("dot","pdf",x)
			os.system("dot -Tpdf %s -o %s" % (x, out))

proc = 4
part = len(dr)/proc
if part*proc != len(dr):
	part = part + 1
	
for x in range(0, len(dr), part):
	myThread(dr[x:x+part]).start()
	
#for x in dr:
#	out = re.sub("dot","jpg",x)
#	os.system("dot -Tjpg %s -o %s" % (x, out))

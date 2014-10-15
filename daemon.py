#!/usr/bin/python
#coding=utf-8
#
#Author: TT_last
#
#This is a daemon for linux judge
#
#
#
import codecs
import sys, os, time, atexit
import subprocess
import fcntl
from pymongo import MongoClient
from signal import SIGTERM
import ConfigParser

#This is the class for judge
#
daemondir = "Path of mine"
cfgfile = "/daemon.ini" # Don't change it!!!!
host = "127.0.0.1"
dbname = "test_db"
cefile = "ce.txt"
dadir = "./data"
tmdir = "./temp"
lockerpath = "/home/kidx/"
langf = {1:"Main.c",2:"Main.cpp",3:"Main.java",4:"Main.cpp",5:"Main.cs",6:"Main.vb"}


#This is a daemon module
class Daemon:
	'''
	A daemon for gzhu judge
	'''
	def __init__(self,stdin='/dev/null',stdout='/dev/null',stderr='/dev/null'):
		self.stdin = stdin
		self.stdout = stdout
		self.stderr = stderr

	def start(self):
		'''
		'''
		print daemondir
		os.chdir(daemondir + "/judge");
		#os.umask(0)

		#redirect standard io
		'''
		sys.stdout.flush()
		sys.stderr.flush()
		sin = file(self.stdin,"r")
		sout = file(self.stdout,"a+")
		serr = file(self.stderr,"a+",0)
		os.dup2(sin.fileno(),sys.stdin.fileno())
		os.dup2(sout.fileno(),sys.stdout.fileno())
		os.dup2(serr.fileno(),sys.stderr.fileno())
		'''
		self.run()

	def run(slef):
		pass


def makefile(indir,lang,val):
	try:
		sfile = indir + "/" + langf[lang]
		print sfile
		fd = codecs.open(sfile,"wb",'utf-8')
		print sfile
		fd.write(val)
		fd.close()
		return True
	except:
		return False



class judge:
	'''

	'''
	def __init__(self,lang,datadir = "./data",tmpdir = "./temp",\
			timelimit = 1000,memlimit = 65535,outlimit = 8192,spj = False,tc = False):
		self.lang = lang
		self.datadir = datadir
		self.tmpdir = tmpdir
		self.timelimit = timelimit
		self.memlimit = memlimit
		self.outlimit = outlimit
		self.spj = spj
		self.tc = tc

	def setlimit(self,timelimit = 1000,memlimit = 65535,outlimit = 8192):
		self.timelimit = timelimit
		self.memlimit = memlimit
		self.outlimit = outlimit

	def run(self):
		try:
			#os.chdir("./Judge")
			self.result,self.mem,self.time = (0,0,0)
			if self.spj and self.tc:
				p = subprocess.Popen("./Judge -l "+str(self.lang)+" -D "+self.datadir\
						+" -d "+self.tmpdir+" -t "+str(self.timelimit)+" -m "+str(self.memlimit)+" -o "+str(self.outlimit) + " -S dd -T",shell=True,stdout=subprocess.PIPE)
			elif self.spj:
				p = subprocess.Popen("./Judge  -l "+str(self.lang)+" -D "+self.datadir\
						+" -d "+self.tmpdir+" -t "+str(self.timelimit)+" -m "+str(self.memlimit)+" -o "+str(self.outlimit) + " -S dd",shell=True,stdout=subprocess.PIPE)
			elif self.tc:
				p = subprocess.Popen("./Judge -l "+str(self.lang)+" -D "+self.datadir\
						+" -d "+self.tmpdir+" -t "+str(self.timelimit)+" -m "+str(self.memlimit)+" -o "+str(self.outlimit) + " -T",shell=True,stdout=subprocess.PIPE)
			else:
				p = subprocess.Popen("./Judge -l "+str(self.lang)+" -D "+self.datadir\
						+" -d "+self.tmpdir+" -t "+str(self.timelimit)+" -m "+str(self.memlimit)+" -o "+str(self.outlimit),shell=True,stdout=subprocess.PIPE)
			for l in p.stdout:
				(self.result,self.mem,self.time) = l.split()
			self.result = int(self.result)
			self.mem = int(self.mem)
			self.time = int(self.time)
		except :
			exit(1)
#		print (self.result,self.mem,self.timek

OJ_WAIT = 0
OJ_RUN = 1
OJ_AC = 2
OJ_PE = 3
OJ_TLE = 4
OJ_MLE = 5
OJ_WA = 6
OJ_OLE = 7
OJ_CE = 8

#this is the judgeDaemon
class JudgeDaemon(Daemon):
	def run(self):
		cfg = ConfigParser.ConfigParser()
		print daemondir
		cfg.readfp(open(daemondir+cfgfile))
		host = cfg.get('daemon','Host')
		dbname = cfg.get('daemon','DataBase')
		cefile = cfg.get('daemon','CE_File')
		dadir = cfg.get('daemon','DataFolder')
		tmdir = cfg.get('daemon','TempFolder')
		lockerpath = cfg.get('daemon','LockerPath')
		while True:
			try:
				con = MongoClient(host)
				db = con[dbname]
				users = db.users
				problems = db.problems
				solutions = db.solutions
				one_solution = solutions.find_and_modify({'result':OJ_WAIT}, {"$set":{"result":OJ_RUN}})
				if one_solution != None:
					user = users.find_one({'name':one_solution['userName']})
					#print one_solution["language"],one_solution["code"]
					if makefile(tmdir,int(one_solution["language"]),one_solution["code"]):
						#print one_solution["language"],one_solution["code"]
						print dadir + "/" + str(one_solution['problemID'])
						one_problem = problems.find_one({'problemID':int(one_solution['problemID'])})
						if int(one_problem['spj']) == 1 and int(one_problem['TC']) == 1:
							gzhujudge = judge(one_solution["language"],datadir = dadir +"/" + str(one_solution['problemID']),tmpdir=tmdir,spj=True,tc=True);
						elif int(one_problem['spj']) == 1:
							gzhujudge = judge(one_solution["language"],datadir = dadir +"/" + str(one_solution['problemID']),tmpdir=tmdir,spj=True);
						elif int(one_problem['TC']) == 1:
							gzhujudge = judge(one_solution["language"],datadir = dadir +"/" + str(one_solution['problemID']),tmpdir=tmdir,tc=True);
						else:
							gzhujudge = judge(one_solution["language"],datadir = dadir +"/" + str(one_solution['problemID']),tmpdir=tmdir);
						gzhujudge.setlimit(int(one_problem['timeLimit']),int(one_problem['memoryLimit']))	
						gzhujudge.run();
						if gzhujudge.result == OJ_CE:
							ce_file = open(tmdir + '/' + cefile)
							try:
								all_ce_text = ce_file.read()
								ce_file.close()
								solutions.update({"_id":one_solution["_id"]},{"$set":{"CE":all_ce_text}})
							except:
								ce_file.close()
								solutions.update({"_id":one_solution["_id"]},{"$set":{"CE":"错误：编译信息无法获取！（可能存在乱码）\n"}})
						if gzhujudge.result == OJ_AC: #AC
							problems.update({"problemID":int(one_solution['problemID'])},{"$inc":{"AC":1}})
							userfile = lockerpath + str(user['name']) + ".lock"
							userlocker = open(userfile, 'w')
							fcntl.flock(userlocker, fcntl.LOCK_EX)
							is_ac = solutions.find_one({'problemID':int(one_solution['problemID']),'userName':one_solution['userName'],'result':OJ_AC})
							if is_ac == None:
								users.update({'name':user['name']},{"$inc":{"solved":1}})
							solutions.update({"_id":one_solution["_id"]},{"$set":{"result":gzhujudge.result,"time":gzhujudge.time,"memory":gzhujudge.mem}})
							fcntl.flock(userlocker, fcntl.LOCK_UN)
							userlocker.close()
						else:
							solutions.update({"_id":one_solution["_id"]},{"$set":{"result":gzhujudge.result,"time":gzhujudge.time,"memory":gzhujudge.mem}})
				else:
					pass
			except:
				pass
			time.sleep(1)



if __name__ == "__main__":
	daemon = JudgeDaemon(stdout="/dev/stdout")
	daemondir = os.getcwd()
	daemon.start()


"""
scons build file

@author: Jean-Lou Dupont
"""
import os
import sys
import shutil

try:
	from pyjld.os import recursive_chmod, safe_copytree
except:
	print "*** Missing package: use easy_install pyjld.os"
	exit(1)


Help("""\
 Type:	
   'scons' to build the library,
   'scons deb' to build the .deb package
""")

#Progress('Evaluating $TARGET\n')

env = Environment(CPPPATH='project/includes')       
       
# DEFAULT: build `litm.a`
# =======
#
#  1- build library
#  2- prepare environment for packaging
#
#
if not COMMAND_LINE_TARGETS:
	litm = env.Library('litm', Glob("./project/src/*.c") )
	Default(litm)



# BUILDING .deb PACKAGE
# =====================
if 'deb' in COMMAND_LINE_TARGETS:
	print "Preparing .deb package"
	try:
		print """scons: cloning 'liblitm.a'""" 
		shutil.copy('liblitm.a', './packages/debian/usr/lib')
		
		print """scons: cloning 'litm.h'"""
		shutil.copy('./project/includes/litm.h', './packages/debian/usr/include')
		
		print """scons: removing /tmp/litm"""
		shutil.rmtree('/tmp/litm', ignore_errors=True)
		
		print """scons: cloning ./packages/debian to /tmp/litm"""
		safe_copytree('./packages/debian', '/tmp/litm', skip_dirs=['.svn',], dir_mode=0775, make_dirs=True)
		
		print """scons: adjusting permissions for `dkpg-deb`"""
		recursive_chmod("/tmp/litm", mode=0775)
		
	except Exception,e:
		print "*** ERROR [%s] ***" % e
	
env.Command("deb", "/tmp/litm", "dpkg-deb --build $SOURCE")


# INSTALLING on LOCAL MACHINE
if 'install' in COMMAND_LINE_TARGETS:
	print "scons: INSTALLING LOCALLY"
	shutil.copy('./project/includes/litm.h', '/usr/include')
	#shutil.copy('./liblitm.a', '/usr/lib')

env.Command("install", "./liblitm.a", "cp $SOURCE /usr/lib")

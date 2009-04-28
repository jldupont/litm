"""
scons build file

@author: Jean-Lou Dupont
"""
import os
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
   'scons debug' to build the debug library
""")



env_release = Environment(CPPPATH='#project/includes')
SConscript('project/src/SConscript', build_dir='release', exports={'env':env_release})


env_debug   = Environment(CPPPATH='#project/includes', CPPFLAGS="-D_DEBUG -g")
SConscript('project/src/SConscript', build_dir='debug', exports={'env':env_debug})

# INSTALLING on LOCAL MACHINE
if 'install' in COMMAND_LINE_TARGETS:
	print "scons: INSTALLING LOCALLY"
	shutil.copy('./project/includes/litm.h', '/usr/include')
	shutil.copy('./release/liblitm.a', '/usr/lib/liblitm.a')
	shutil.copy('./debug/liblitm.a', '/usr/lib/liblitm_debug.a')

env_release.Command("install", "./release/liblitm.a", "cp $SOURCE /usr/lib")


# BUILDING .deb PACKAGE
# =====================
if 'deb' in COMMAND_LINE_TARGETS:
	print "Preparing .deb package"
	try:
		print """scons: cloning 'liblitm.a'""" 
		shutil.copy('./release/liblitm.a', './packages/debian/usr/lib')
		shutil.copy('./debug/liblitm.a', './packages/debian/usr/lib/liblitm_debug.a')
		
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
	
env_release.Command("deb", "/tmp/litm", "dpkg-deb --build $SOURCE")

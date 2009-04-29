"""
scons build file

@author: Jean-Lou Dupont
"""
import os
import shutil
from string import Template
try:
	from pyjld.os import recursive_chmod, safe_copytree
except:
	print "*** Missing package: use easy_install pyjld.os"
	exit(1)


Help("""\
 Type:	
   'scons' to build the libraries (release and debug),
   'scons deb' to build the .deb package
   'scons install' to install on local machine
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


def read_version():
	file = open('./project/VERSION')
	version = file.read()
	file.close()
	return version

def generate_control(version):
	"""
	Reads in ./project/control
		updates the $version$ and
		places the result in ./packages/debian/DEBIAN/control
	"""
	file_src=open("./project/control")
	content = file_src.read()
	file_src.close()
	updated=Template(content)
	c=updated.substitute(version=version)
	
	file_target=open("./packages/debian/DEBIAN/control","w")
	file_target.write(c)
	file_target.close()


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

		version = read_version()
		print """scons: updating debian 'control' with version[%s]""" % version
		generate_control(version)
		
		print """scons: cloning ./packages/debian to /tmp/litm"""
		safe_copytree('./packages/debian', '/tmp/litm', skip_dirs=['.svn',], dir_mode=0775, make_dirs=True)
		
		print """scons: adjusting permissions for `dkpg-deb`"""
		recursive_chmod("/tmp/litm", mode=0775)


	except Exception,e:
		print "*** ERROR [%s] ***" % e
	
env_release.Command("deb", "/tmp/litm", "dpkg-deb --build $SOURCE")

	

# RELEASING
#
#  The 'deb' command is assumed to have been performed.
#  The 'deb' package is assumed to be sitting in /tmp as litm.deb 
#
# =========
if 'release' in COMMAND_LINE_TARGETS:

	# extract "version"
	version = read_version()
	print "scons: RELEASING version %s" % version
	
	name = "litm_%s-1_i386.deb" % version
	path = "/tmp/%s" % name
	print "scons: renaming debian package: %s" % name
	shutil.copy('/tmp/litm.deb', path)

	print "scons: copying to repo in /tags"
	shutil.copy(path, "../tags")

#dummy target
env_release.Command("release", "./project/VERSION", "cp $SOURCE ../tags/VERSION")

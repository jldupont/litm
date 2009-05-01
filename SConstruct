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
   'scons release' to release the package to tags/debian repository
   'scons install' to install on local machine
""")


# need to go up one level in order
#  for commands such as 'dpkg-scanpackages' to work as expected
#curdir = os.getcwd()
#root_dir  = os.path.dirname( curdir )
#print "scons: using root directory [%s]" % root_dir
#os.chdir( root_dir )
#print "getcwd[%s]" % os.getcwd()


env_release = Environment(CPPPATH='#project/includes')
SConscript('project/src/SConscript', build_dir='release', exports={'env':env_release})


env_debug   = Environment(CPPPATH='#project/includes', CPPFLAGS="-D_DEBUG -g")
SConscript('project/src/SConscript', build_dir='debug', exports={'env':env_debug})



# INSTALLING on LOCAL MACHINE
if 'install' in COMMAND_LINE_TARGETS:
	print "scons: INSTALLING LOCALLY"
	shutil.copy('./project/includes/litm.h', '/usr/include')
	shutil.copy('./release/liblitm.so', '/usr/lib/liblitm.so')
	shutil.copy('./debug/liblitm.so', '/usr/lib/liblitm_debug.so')

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

def replace_params(path_src, path_dest, params):
	"""
	Replace the parameters in the target path
	"""
	file = open(path_src,"r")
	contents = file.read()
	file.close()
	
	tpl=Template(contents)
	updated_content = tpl.safe_substitute( **params )
	
	file = open(path_dest, "w")
	file.write(updated_content)
	file.close()
	
	


# BUILDING .deb PACKAGE
# =====================
if 'deb' in COMMAND_LINE_TARGETS:
	print "Preparing .deb package"
	try:
		version = read_version()
		print """scons: building release [%s]""" % version
		
		print """scons: cloning release 'liblitm.so'""" 
		shutil.copy('./release/liblitm.so', "./packages/debian/usr/lib/liblitm-%s.so" % version)
		
		print """scons: cloning debug 'liblitm_debug.so'"""
		shutil.copy('./debug/liblitm.so', './packages/debian/usr/lib/liblitm_debug-%s.so' % version)
		
		print """scons: cloning 'litm.h' & adjust version"""
		replace_params('./project/includes/litm.h', './project/includes/litm.h', {'version':version} )
		shutil.copy('./project/includes/litm.h', './packages/debian/usr/include/litm.h')
		
		print """scons: removing /tmp/litm"""
		shutil.rmtree('/tmp/litm', ignore_errors=True)

		print """scons: updating debian 'control' with version[%s]""" % version
		generate_control(version)
		
		print """scons: cloning ./packages/debian to /tmp/litm"""
		safe_copytree('./packages/debian', '/tmp/litm', skip_dirs=['.svn',], dir_mode=0775, make_dirs=True)
		
		print """scons: adjusting permissions for `dkpg-deb` command-line"""
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

	print "scons: copying [%s] to repo in dists/main/binary-i386" % path
	shutil.copy(path, "../dists/stable/main/binary-i386")
	
	debian_path = "./dists/stable/main/binary-i386/%s" % name
	print "scons: running dpkg-scanpackages  [%s]" % debian_path
	os.system("./do_release")
	




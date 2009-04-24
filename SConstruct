"""
scons build file

@author: Jean-Lou Dupont
"""
import os
import sys
import shutil


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


# TODO move to pyjld
# 
def recursive_chmod(path, mode=0775, debug=False):
	for root, dirs, files in os.walk(path):
		for filename in files:
			this_path = os.path.join(root, filename)
			if debug:
				print "chmod %s %o" % (this_path, mode) 
			os.chmod(this_path, mode)
		for _dir in dirs:
			this_path = os.path.join(root, _dir)
			if debug:
				print "chmod %s %o" % (this_path, mode) 
			os.chmod(this_path, mode)
		

# TODO move to pyjld
# 
def copytree_no_svn(src, dst, symlinks=False, debug=False):
    names = os.listdir(src)
    os.makedirs(dst)
    errors = []
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if symlinks and os.path.islink(srcname):
                linkto = os.readlink(srcname)
                os.symlink(linkto, dstname)
            elif os.path.isdir(srcname):
            	#exclude .svn dirs
            	if (os.path.basename(srcname)!='.svn'):
            		if debug:
            			print "! copying [%s]" % srcname
                	copytree_no_svn(srcname, dstname, symlinks)
            else:
                shutil.copy2(srcname, dstname)
            # XXX What about devices, sockets etc.?
        except (IOError, os.error), why:
            errors.append((srcname, dstname, str(why)))
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except shutil.Error, err:
            errors.extend(err.args[0])
    try:
        shutil.copystat(src, dst)
    except shutil.WindowsError:
        # can't copy file access times on Windows
        pass
    except OSError, why:
        errors.extend((src, dst, str(why)))
    if errors:
        raise shutil.Error, errors

	



# BUILDING .deb PACKAGE
# =====================
if 'deb' in COMMAND_LINE_TARGETS:
	print "Preparing .deb package"
	try:
		print """scons: cloning 'liblitm.a'""" 
		shutil.copy('liblitm.a', 'packages/debian/usr/lib')
		print """scons: cloning 'litm.h'"""
		shutil.copy('project/includes/litm.h', 'packages/debian/usr/include')
		print """scons: removing /tmp/litm"""
		shutil.rmtree('/tmp/litm', ignore_errors=True)
		
		print """scons: cloning /packages/debian to /tmp/litm"""
		copytree_no_svn('packages/debian', '/tmp/litm')
		print """scons: adjusting permissions for `dkpg-deb`"""
		recursive_chmod("/tmp/litm")
		
	except Exception,e:
		print "*** ERROR [%s] ***" % e
	
env.Command("deb", "/tmp/litm", "dpkg-deb --build $SOURCE")

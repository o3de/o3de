#!/usr/bin/env python
import sys
import os
import shutil
import subprocess

def cmake(platform_dir, src_dir, arg):
    if not(os.path.exists(platform_dir)):
        os.mkdir(platform_dir)
    os.chdir(platform_dir)				# go to platform directory
    cmd = "cmake " + arg + " ../" + src_dir
    print("cmd")
    subprocess.call(cmd, shell=True)						# run cmake

def clean(platform_dir):
    if os.path.exists(platform_dir):
        shutil.rmtree(platform_dir)	

if __name__ == '__main__':
    src_dir = '../src/' 					# source code directory
    bin_dir = '../build/' 				# binary directory

    if not(os.path.exists(bin_dir)): 	# binary directory is created if not found
        print("Creating bin directory: " + bin_dir)
        os.mkdir(bin_dir)

    platform = sys.platform
    cmd = ""

    if (platform == 'darwin'): 			# platform directory is created if not found
        platform = 'mac'
        cmd = '-G Xcode' 

    if (platform.startswith('linux')):
        platform = 'linux'
        cmd = '-DCMAKE_BUILD_TYPE=Release' 

    platform_dir = bin_dir + platform

    if len(sys.argv) == 1:
        print("Invalid arguments to run: please specify --cmake or --clean")
        sys.exit(-1)
    else:
        for arg in sys.argv:
            if   (arg == '--cmake'):
                cmake(platform_dir, src_dir, cmd)
            elif (arg == '--clean'):
                clean(platform_dir)

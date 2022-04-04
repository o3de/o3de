#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Python script to create the package necessary for GameLift
# To do this by hand please see the steps provided @ https://www.o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/build-packaging-for-linux/

import os 
import shutil
import re
import argparse
import itertools
import threading
import time
import sys
import stat

def main():        
    script_description = """Create a Linux dedicated server package for GameLift\n
    Prerequisites:
    - You have built your project with the AWS GameLift Gem enabled.
    - You have built the Profile version of your project's server launcher.
    - You have run the Asset Processor and compiled all of the project's assets.
    https://www.o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/build-packaging-for-linux/"""
    parser = argparse.ArgumentParser(description = script_description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--package_dir', help='where to create the linux package', required=True)
    parser.add_argument('--profile_build_dir', help="your game's build profile folder", required=True)
    parser.add_argument('--asset_cache_dir', help="your game's asset cache folder", required=True)

    parser.parse_args()

    package_dir = parser.parse_args().package_dir
    build_bin_folder = parser.parse_args().profile_build_dir
    project_cache_folder = parser.parse_args().asset_cache_dir

    if not project_cache_folder.endswith("Cache") and not project_cache_folder.endswith("Cache/"):
        print("Package cancelled. Invalid asset_cache_dir")
        return

    if not build_bin_folder.endswith("bin/profile") and not build_bin_folder.endswith("bin/profile/"):
        print("Package cancelled. Invalid profile_build_dir")
        return

    if (os.path.exists(package_dir)):
        if len(os.listdir(package_dir)) != 0:
            val = input(f"Path '{package_dir}' already exists, replace? Y/n: ")
            if (val == 'n'):
                print("Package cancelled.")
                return
            elif (val != 'Y'):
                print("Package cancelled. Incorrect user response.")
                return
            else:  # delete everything and start fresh
               shutil.rmtree(package_dir)
               os.mkdir(package_dir)
    else:
        os.mkdir(package_dir)
    
    # this script takes a minute to run, display an animation
    done = False
    animated_str = "loading"
    def animate():
        for c in itertools.cycle(['|', '/', '-', '\\']):
            if done:
                break
            # clear the line and then display animated text
            sys.stdout.write("\033[K")
            print(f'{animated_str} {c}', end="\r")
            time.sleep(0.1)
        sys.stdout.write("\033[K")
        print('Done!', end="\r")

    t = threading.Thread(target=animate)
    t.start()

    # make required sub-folders
    animated_str = "make new folders"
    os.mkdir(os.path.join(package_dir, "assets"))
    os.mkdir(os.path.join(package_dir, "lib64"))

    # copy executables
    for filename in os.listdir(build_bin_folder):
        # skip gamelauncher, editor libraries, and assetprocessor
        if any(map(filename.__contains__, {'.Editor.so', '.GameLauncher', 'libEditor'})):
            continue
        if filename in {'AssetProcessor', 'Editor', 'MaterialEditor', 'o3de', 'libComponentEntityEditorPlugin.so'}:
            continue

        f = os.path.join(build_bin_folder, filename)
        if os.path.isfile(f) and os.access(f, os.X_OK):
            animated_str = f"copying: {filename}"
            target = os.path.join(package_dir, filename)
            shutil.copyfile(f, target)
            # preserve read/write/execute permission. this is lost during the copy.
            st = os.stat(f)
            os.chmod(target, st.st_mode | stat.S_IEXEC)

    # copy registry folder
    animated_str = "copying: registry"
    target = os.path.join(package_dir, "Registry")
    shutil.copytree(os.path.join(build_bin_folder, "Registry"), target)

    # copy asset cache folder
    animated_str = f"copying: asset cache"
    target = os.path.join(package_dir, "assets", "linux")
    shutil.copytree(os.path.join(project_cache_folder, "linux"), target)

    # copy libraries from /usr/lib/x86_64-linux-gnu/ to <package base folder>/lib64/
    regex = re.compile('(libc\+\+.*)|(libxkb.*)|(libxcb.*)|(libX.*)|(libbsd.*)')
    for file in os.listdir("/usr/lib/x86_64-linux-gnu/"):
        if regex.match(file):
            animated_str = f"copying: {file}"
            target = os.path.join(package_dir, 'lib64', file)
            shutil.copyfile(os.path.join('/usr/lib/x86_64-linux-gnu/', file), target)

    # create a build install script for GameLift
    with open(os.open(os.path.join(package_dir, "install.sh"), os.O_CREAT | os.O_WRONLY, 0o777), "w") as f:
        f.write("""#!/bin/sh 
sudo cp -a ./lib64/* /lib64/.
 
if cat /etc/system-release | grep -qFe 'Amazon Linux release 2'
then
 sudo yum groupinstall 'Development Tools' -y
 sudo yum install python3 -y
   
 echo 'Update outdated make package'
 mkdir make && cd make
 wget http://ftp.gnu.org/gnu/make/make-4.2.1.tar.gz
 tar zxvf make-4.2.1.tar.gz
 mkdir make-4.2.1-build make-4.2.1-install
 cd make-4.2.1-build
 /local/game/make/make-4.2.1/configure --prefix='/local/game/make/gmake-4.2.1-install'
 make -j 8
 make -j 8 install
 export PATH=/local/game/make/gmake-4.2.1-install/bin:$PATH
 sudo ln -sf /local/game/make/gmake-4.2.1-install/bin/make /local/game/make/gmake-4.2.1-install/bin/gmake
 cd /local/game/

 echo 'Installing missing libs for AL2'
 mkdir glibc && cd glibc
 wget http://mirror.rit.edu/gnu/libc/glibc-2.29.tar.gz
 tar zxvf glibc-2.29.tar.gz
 mkdir glibc-2.29-build glibc-2.29-install
 cd glibc-2.29-build
 /local/game/glibc/glibc-2.29/configure --prefix='/local/game/glibc/glibc-2.29-install'
 make -j 8
 make -j 8 install
 sudo ln -sf /local/game/glibc/glibc-2.29-install/lib/libm.so.6 /local/game/libm.so.6
cd /local/game/
fi
    
echo 'Install Success'""")

    # create a script to run the server
    with open(os.open(os.path.join(package_dir, "run_server.sh"), os.O_CREAT | os.O_WRONLY, 0o777), "w") as f:
        f.write("""#!/bin/sh
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`
$SCRIPTPATH/UIUX_NetworkDemo.ServerLauncher --engine-path=$SCRIPTPATH --project-path=$SCRIPTPATH --project-cache-path=$SCRIPTPATH/assets -bg_ConnectToAssetProcessor=0""")
    
    done = True


if __name__ == "__main__":
    main()
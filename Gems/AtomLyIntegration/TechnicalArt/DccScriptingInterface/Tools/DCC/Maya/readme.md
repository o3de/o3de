---
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
---

# <u>DCCsi.Tools.DCC.Maya.Readme</u>

"DccScriptingInterface" (aka DCCsi) is a Gem for O3DE to extend and interface with dcc tools in the python ecosystem. Each dcc tool may have it's own specific version of python. Most are some version of py3+. O3DE provides an install of py3+ and manages package dependancies with requirements.txt files and the cmake build system.

However Autodesk Maya 2020 (and earlier) still uses a version of py2.7
so we need an alternate way to deal with package management for some DCC tools.

Maya ships with it's own python interpreter called mayapy.exe

Generally it is located here:
`C:\Program Files\Autodesk\Maya2020\bin\mayapy.exe`

The python install and site-packages are here:
`C:\Program Files\Autodesk\Maya2020\Python\Lib\site-packages`

A general goal of the DCCsi is be self-maintained, and to not taint the users installed applications of environment.

So we boostrap additional access to site-packages in our userSetup.py:
`"C:/Depot/o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/Scripts/userSetup.py"`

We don't want users to have to install or use Python2.7 although with maya and possibly other dcc tools we don't have that control.  Maya 2020 and earlier versions are still on Python2.7, so instead of forcing another install of python we can just use mayapy to manage extensions.

Pip may already be installed, you can check like so (your maya install path may be different):

`C:\Program Files\Autodesk\Maya2020\bin>mayapy -m pip --version`

Another way to check is: `mayapy.exe -m ensurepip`

If pip is not available yet for your mayapy, there are a couple ways to address this.

## Method 1, use get-pip

First find out where the site-packages is located

C:\Program Files\Autodesk\Maya2020\bin>

    mayapy -m site

this will report something like the following

    sys.path = [
        'C:\\Program Files\\Autodesk\\Maya2020\\bin',
        'C:\\Program Files\\Autodesk\\Maya2020\\bin\\python27.zip',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python\\DLLs',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\plat-win',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\lib-tk',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python',
        'C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages',
    ]
    USER_BASE: 'C:\\Users\\< you >\\AppData\\Roaming\\Python' (exists)
    USER_SITE: 'C:\\Users\\< you >\\AppData\\Roaming\\Python\\Python27\\site-packages' (doesn't exist)
    ENABLE_USER_SITE: True

This is the location we are looking for:
`C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages`

download get-pip.py and put into the above ^ directory:
`https://bootstrap.pypa.io/pip/2.7/get-pip.py`

Put that in the root of site-packages:
`C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages\\get-pip.py`

With get-pip module ready, we run it to install pip:
C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m get-pip`

Now you should be able to run the following command and verify pip:
C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m pip --version
pip 20.3.4 from C:\Users\< you >\AppData\Roaming\Python\Python27\site-packages\pip (python 2.7)`

## Method 2, use ensurepip

Note: these steps are pretty similar for any version of Maya with any version of python.
You just need to be careful to make sure you are using the correct requirements.txt
And installing into the correct target folder...

Maya 2020 (Python 2.7.11)

Use a cmd prompt with elevated Admin rights.

    C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

    cd C:\Program Files\Autodesk\Maya2020\bin

This command will ensure that pip is installed

    mayapy -m ensurepip

This command will upgrade pip (for instance if a secutiry patch is released)

    mayapy -m ensurepip --upgrade

Now your local maya install is all set up with pip so you can install additional python packages to use in maya.  (note: not all packages are compatible with maya)

Now you will want to run the following file to finish setup...

We have a requirements.txt file with the extension packages we use in the DCCsi.
You'll need the repo/branch path of your O3DE (aka Lumberyard) install.
And you'll need to know where the DCCsi is located, we will install package dependancies there.

Note: you may need to update the paths below to match your local o3de engine install!

This is py2.7 requirements: DccScriptingInterface\Tools\DCC\Maya\requirements.txt

The following will install those requirements into a sandbox area that we can boostrap in DCC tools running py2.7

C:\Program Files\Autodesk\Maya2020\bin>

    mayapy -m pip install -r C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\requirements.txt -t C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages

![image](https://user-images.githubusercontent.com/23222931/155037696-cab81e13-7910-433d-b038-75a07e6690ad.png)

## Other versions of Maya (2022 and beyond)

Maya 2022 finally brings us to Python3, however to facilitate the transition Maya 2022 also has a backwards compatible implementation of Py2.7 (effectively it has support for both py2.7 and py3.7) 

### Maya 2022 (Python 2.7.11)

Method 2 above should still work, however you'd want to use this mayapy2:
    `C:\Program Files\Autodesk\Maya2022\bin\mayapy2.exe`

open a cmd with elevated admin rights (use windows start, type cmd, right-click on 'command prompt', choose 'Run as administrator')

C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

    cd C:\Program Files\Autodesk\Maya2022\bin

This command will ensure that pip is installed

    C:\Program Files\Autodesk\Maya2020\bin>mayapy2 -m ensurepip

This command will upgrade pip (for instance if a secutiry patch is released)

    C:\Program Files\Autodesk\Maya2020\bin>mayapy2 -m ensurepip --upgrade

The following will install those requirements into a sandbox area that we can boostrap in DCC tools running py2.7

    mayapy2 -m pip install -r C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\requirements.txt -t C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages

![image](https://user-images.githubusercontent.com/23222931/155037710-79bee421-1355-484b-8c96-f672157da40a.png)

### Maya 2022 (Python 3.10.5)

This also is not very different, you just need to modify some of the commands. Also because O3DE is also on a version of py3.10.x, we can re-use the requirements.txt file that is in the root of the DccScriptingInterface gem folder.

    C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

    cd C:\Program Files\Autodesk\Maya2022\bin

This command will ensure that pip is installed

    C:\Program Files\Autodesk\Maya2020\bin>mayapy -m ensurepip

This command will upgrade pip (for instance if a secutiry patch is released)

    C:\Program Files\Autodesk\Maya2020\bin>mayapy -m ensurepip --upgrade

The following will install those requirements into a sandbox area that we can boostrap in DCC tools running py3.10.x 

    mayapy -m pip install -r C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\requirements.txt -t C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\3.x\3.10.x\site-packages

![image](https://user-images.githubusercontent.com/23222931/155037723-8f514a85-194f-46e8-b726-55a04f0860bb.png)

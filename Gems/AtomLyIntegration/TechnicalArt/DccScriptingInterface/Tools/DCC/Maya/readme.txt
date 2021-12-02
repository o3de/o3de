Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
-------------------------------------------------------------------------------

"DccScriptingInterface" aka DCCsi is a Gem for O3DE to extend and interface with dcc tools
in the python ecosystem. Each dcc tool may have it's own specific version of python.
Most are some version of py3+. O3DE provides an install of py3+ and manages package
dependancies with requirements.txt files and the cmake build system.

However Autodesk Maya still uses a version of py2.7 and so we need an alternate way
to deal with package management for this DCC tool.

Maya ships with it's own python interpreter called mayapy.exe

Generally it is located here:
C:\Program Files\Autodesk\Maya2020\bin\mayapy.exe

The python install and site-packages are here:
C:\Program Files\Autodesk\Maya2020\Python\Lib\site-packages

A general goal of the DCCsi is be self-maintained, and to not taint the users installed applications of environment.

So we boostrap additional access to site-packages in our userSetup.py:
"C:\Depot\Lumberyard\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\Maya\Scripts\userSetup.py"

We don't want users to have to install or use Python2.7 although with maya and possibly other dcc tools we don't have that control.  Maya 2020 and earlier versions are still on Python2.7, so instead of forcing another install of python we can just use mayapy to manage extensions.

Pip may already be installed, you can check like so (your maya install path may be different):

C:\Program Files\Autodesk\Maya2020\bin>mayapy -m pip --version

If pip is not available yet for your mayapy.

First find out where th site-packages is located

C:\Program Files\Autodesk\Maya2020\bin>mayapy -m site
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
USER_BASE: 'C:\\Users\\gallowj\\AppData\\Roaming\\Python' (exists)
USER_SITE: 'C:\\Users\\gallowj\\AppData\\Roaming\\Python\\Python27\\site-packages' (doesn't exist)
ENABLE_USER_SITE: True

This is the location we are looking for:
C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages

download get-pip.py and put into the above ^ directory:
https://bootstrap.pypa.io/pip/2.7/get-pip.py

Put that in the root of site-packages:
C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages\\get-pip.py

With get-pip module ready, we run it to install pip:
C:\Program Files\Autodesk\Maya2020\bin>mayapy -m get-pip

Now you should be able to run the following command and verify pip:
C:\Program Files\Autodesk\Maya2020\bin>mayapy -m pip --version
pip 20.3.4 from C:\Users\< you >\AppData\Roaming\Python\Python27\site-packages\pip (python 2.7)

Now your local maya install is all set up with pip so you can install additional python packages to use in maya.  (note: not all packages are compatible with maya)

Now you will want to run the following file to finish setup...
We have a requirements.txt file with the extension packages we use in the DCCsi.
You'll need the repo/branch path of your O3DE (aka Lumberyard) install.
And you'll need to know where the DCCsi is located, we will install package dependancies there.

Note: you may need to update the paths below to match your local o3de engine install!

C:\Program Files\Autodesk\Maya2020\bin>mayapy -m pip install -r C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\Maya\requirements.txt -t C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages

# O3DE DCCsi, DCC Maya

The "DccScriptingInterface" (aka DCCsi) is a Gem for O3DE to extend and interface with dcc tools in the python ecosystem.  This document contains the details of configuration of Maya as a DCC tool to be used with O3DE.  This sets up Maya to be integrated with O3DE in a managed way, via the DCCsi. For more information about the DCCis please see the readme.md at the root of the Gem.

### Status: Prototype

### Version: 0.0.1

### OS: Windows only (for now)

### Support:

- Maya 2023+ w/ Python3 <-- default version
- Maya 2022 w/ Python3 (will not support py2.7)
- Future version of Maya that include Py3+ can be supported with minimal change to the configuration

## Brief

This is an experimental Blender integration with O3DE, it's intent is:

1. Configure and launch Maya (from CLI start.py, or even via O3DE editor menus)
2. Bootstrap O3DE 'Studio Tools', provide shared code access, facilitate python tools and plugins, etc.
3. Soft extension bootstrapping (non-destructive to Users Maya installation)

## Setup

You should enable the DCCsi Gem in your project, this can be done with 'Configure Gems' in the O3DE Project Manager (o3de.exe).  [Adding and Removing Gems in a Project - Open 3D Engine](https://www.o3de.org/docs/user-guide/project-config/add-remove-gems/) This will enable a 'Studio Tools' menu within the O3DE Editor.exe from which some DCC tools can be launched.  The O3DE tools provided with the DCCsi have python package dependencies (via requirements.txt).  However, before launching Maya for O3DE for the first time, you should follow the steps outlined below.

## Configure Maya (TL/DR quick start)

Before the O3DE DCCsi tools will operate correctly in Maya, you will need to install the python package dependencies:

    1. Open a Windows Command Prompt (CMD)

    2. Change directory to:

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

    3. Run the DCCsi `foundation.py` script with a target to the `mayapy.exe` of the vision you want to configure:

```batch
.\python foundation.py -py="C:\Program Files\Autodesk\Maya2023\bin\mayapy.exe""
```

This will install a version of all of the package dependencies into a folder such as the following, where the DCCsi will add them as a site-package based on the DCC tools version of python.

    `DccScriptingInterface\3rdParty\Python\Lib\3.x\3.9.x\site-packages\*`

Since each DCC app, may be on a slightly different version of python, you may find more then one set of installed packages within that 3rdParty location, one for each version of python (the intent here is to maximize compatibility.)

## General Info

What is the DCCsi?

1. A shared development environment for technical art oriented to working with Python across a number of DCC tools.
2. Leverage the existing python ecosystem for technical art.
3. Integrate a DCC app like Substance (or Substance SAT api) from the Python driven VFX and Games ecosystem.
4. Extend O3DE and unlock its potential for content creators, and the Technical Artists that service them.

For general info on the DCCsi:
https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface

For detailed documentation:
https://www.o3de.org/docs/user-guide/< DCC Tools, not stubbed >

## Configure Maya Externally (Optional)

The DCCsi assumes that DCC tools such as Maya are installed within their default install location, for Maya that usually would be:

    `C:\Program Files\Autodesk\Maya2022`    

    `C:\Program Files\Autodesk\Maya2023`

If you'd like to use Maya with O3DE bootstrapped tools externally, outside of the o3de Editor, you can do that also.

There two ways, a windows environment via .bat file, or a `start.py` script

From .bat file, double-click the following file type to start Maya: `C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\win_launch_mayapy_2023.bat`

To start from script:

    1. Open a Windows Command Prompt (CMD)

    2. Change directory to: 

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

    3. Run the Maya `start.py` script:

```batch
.\python Tools\DCC\Maya\start.py
```

The DCCsi defaults to Maya 2023, and the default install location. We understand teams may want to use a specific version of Maya, or may maintain a customized Maya container, or IT managed install location.  If you want to alter the version, or the install location, you'll also want follow these instructions:

The Maya env can be modified by adding this file:`C:\path\tp\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\Env_Dev.bat`

Inside of that you can set/override ENVARsto change the Maya version, it's python version information, as well as your custom install path.  Here is an example of those ENVARs:

```batch
:: Set your preferred defualt Maya and Python version
set MAYA_VERSION=2023
set DCCSI_PY_VERSION_MAJOR=3
set DCCSI_PY_VERSION_MINOR=9
set DCCSI_PY_VERSION_RELEASE=7

:: and you can alter the install path
set MAYA_LOCATION=%ProgramFiles%\Autodesk\Maya%MAYA_VERSION%
```

Additionally, there is additional support to start via scripting.  This is the same setup that the Editor uses to start the external DCC application.  It makes use of a `settings.json` (default settings), and `settings.local.json` (user settings and overrides) within the o3de DCCsi folder for Maya.  These are utilized along with the addition of a `config.py` and `start.py` in the DCC apps folder.  This follows the patterns similar to how Blender can be launched from the O3DE menus, or in a scripted manner rather then legacy windows .bat files.

To generate a `settings.local.json` (which you can then modify with overrides to paths and other settings)::

    1. Open a Windows Command Prompt (CMD)

    2. Change directory to: 

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

    3. Run the Maya` config.py` script:

```batch
python Tools\DCC\Maya\config.py
```

You can now open `settings.local.json` in a text editor and make modifications and resave before starting Maya.

## Additional Advanced Information

Each dcc tool likely has it's own design and architecture that is often not common, and many have it's own specific version of python. Most are some version of py3+. O3DE provides an install of py3+ and manages package dependencies with `requirements.txt` files and the cmake build system.  If you prefer not to use out included `foundation.py` script, the details below will walk you through haw you can manually configure on your own.

Maya ships with it's own python interpreter called `mayapy.exe`

Generally it is located here:

    C:\Program Files\Autodesk\Maya2020\bin\mayapy.exe

The python install and site-packages are here:

    C:\Program Files\Autodesk\Maya2020\Python\Lib\site-packages

A general goal of the DCCsi is be self-maintained, and to not taint the users installed applications of environment.

So as to not directly modify Maya, we bootstrap additional access to site-packages in our `userSetup.py`:

    C:/Depot/o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/Scripts/userSetup.py

We don't want users to have to install or use any additional version of Python, especially use of legacy versions such as python2.7, although with older versions of  Maya and possibly other dcc tools we don't have that control.  Maya 2020 and earlier versions are still on Python2.7, so instead of forcing another install of python we can just use that versions `mayapy.exe` to manage its extensions.

Pip may already be installed, you can check like so (your Maya install path may be different):

```batch
C:\Program Files\Autodesk\Maya2020\bin>'mayapy -m pip --version`
```

Another way to ensure pip exists is: `mayapy.exe -m ensurepip` 

If pip is not available in your maya install for whatever reason, there are a couple ways to go about addressing this.

## Method 1, use get-pip

First find out where the site-packages is located

`C:\Program Files\Autodesk\Maya2020\bin`

```batch
C:\Program Files\Autodesk\Maya2020\bin>mayapy -m site
```

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

    C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages

download get-pip.py and put into the above ^ directory:

    https://bootstrap.pypa.io/pip/2.7/get-pip.py

Put that in the root of site-packages:

    C:\\Program Files\\Autodesk\\Maya2020\\Python\\lib\\site-packages\\get-pip.py

With get-pip module ready, we run it to install pip:

```batch
C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m get-pip`
```

Now you should be able to run the following command and verify pip:

```batch
C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m pip --version
pip 20.3.4 from C:\Users\< you >\AppData\Roaming\Python\Python27\site-packages\pip (python 2.7)`
```

## Method 2, use ensurepip

Note: these steps are pretty similar for any version of Maya with any version of python.
You just need to be careful to make sure you are using the correct requirements.txt
And installing into the correct target folder...

Maya 2020 (Python 2.7.11)

Use a cmd prompt with elevated Admin rights.

    C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

```batch
cd C:\Program Files\Autodesk\Maya2020\bin
```

This command will ensure that pip is installed

```batch
mayapy -m ensurepip
```

This command will upgrade pip (for instance if a security patch is released)

```batch
mayapy -m ensurepip --upgrade
```

Now your local Maya install is all set up with pip so you can install additional python packages to use in Maya.  (note: not all python packages are compatible with Maya)

Now you will want to complete the following instructions to finish setup...

We have a `requirements.txt`file with the extension packages we use in the DCCsi.
You'll need the repo/branch path of your O3DE install.  And you'll need to know where the DCCsi is located, we will install package dependencies in a path such as (based on python version):

    DccScriptingInterface\3rdParty\Python\Lib\3.x\3.9.x\site-packages

Note: you may need to update the paths below to match your local o3de engine install!

This is where the legacy py2.7 version of requirements is stored:                           

    C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\Resources\py27\requirements.txt

The following will install those requirements into a sandbox area that we can bootstrap in DCC tools running py2.7

    C:\Program Files\Autodesk\Maya2020\bin>

```batch
mayapy -m pip install -r C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\requirements.txt -t C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages
```

![image](https://user-images.githubusercontent.com/23222931/155037696-cab81e13-7910-433d-b038-75a07e6690ad.png)

Note: These days most of the DCCsi has be written or refactored for Python3+ syntax, although it is technically possible to write code that is py2.7 and py3 compatible, we have not taken the time to do so (python2.7 is end of life, and Maya 2022+ now comes with python3).  This information is here mainly for historical context, you may be able to modify the code yourself to patch in python 2.7 support if you are working with an older version of Maya.

## Other versions of Maya (2022 and beyond)

Maya 2022 finally brings us to Python3, however to facilitate the transition Maya 2022 also has a backwards compatible implementation of Py2.7 (effectively it has support for both py2.7 and py3.7) 

### Maya 2022 (Python 2.7.11)

Method 2 above should still work, however you'd want to use this mayapy2:

    C:\Program Files\Autodesk\Maya2022\bin\mayapy2.exe

open a cmd with elevated admin rights (use windows start, type cmd, right-click on 'command prompt', choose 'Run as administrator')

    C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

    cd C:\Program Files\Autodesk\Maya2022\bin

This command will ensure that pip is installed

    C:\Program Files\Autodesk\Maya2020\bin>`mayapy2 -m ensurepip`

This command will upgrade pip (for instance if a security patch is released)

    C:\Program Files\Autodesk\Maya2020\bin>`mayapy2 -m ensurepip --upgrade`

The following will install those requirements into a sandbox area that we can bootstrap in DCC tools running py2.7

```batch
mayapy2 -m pip install -r C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\requirements.txt -t C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages
```

![image](https://user-images.githubusercontent.com/23222931/155037710-79bee421-1355-484b-8c96-f672157da40a.png)

### Maya 2022 (Python 3.10.13)

This also is not very different, you just need to modify some of the commands. Also because O3DE is also on a version of py3.10.x, we can re-use the requirements.txt file that is in the root of the DccScriptingInterface gem folder.

    C:\WINDOWS\system32>

This will change directories into Maya's binaries location (where mayapy lives)

    cd C:\Program Files\Autodesk\Maya2022\bin

This command will ensure that pip is installed

    C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m ensurepip`

This command will upgrade pip (for instance if a secutiry patch is released)

    C:\Program Files\Autodesk\Maya2020\bin>`mayapy -m ensurepip --upgrade`

The following will install those requirements into a sandbox area that we can boostrap in DCC tools running py3.10.x

```batch
mayapy -m pip install -r C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\requirements.txt -t C:\Depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\3.x\3.10.x\site-packages
```

---

###### LICENSE INFO

Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

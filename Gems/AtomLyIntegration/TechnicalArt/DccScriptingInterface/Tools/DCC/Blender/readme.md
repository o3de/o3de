# O3DE DCCsi, DCC Blender

The "DccScriptingInterface" (aka DCCsi) is a Gem for O3DE to extend and interface with dcc tools in the python ecosystem.  This document contains the details of configuration of Maya as a DCC tool to be used with O3DE.  This sets up Blender to be integrated with O3DE in a managed way, via the DCCsi. For more information about the DCCis please see the readme.md at the root of the Gem.

### Status: Prototype

### Version: 0.0.1

### Support: Blender 3.x, Currently Windows only

This is currently the first working version of a managed integration of Blender, treat it as an Experimental Preview.

## Brief

This is an experimental Blender integration with O3DE, it's intent is:

1. Configure and launch Blender (from CLI start.py, or even via O3DE editor menus)
2. Bootstrap O3DE 'Studio Tools', provide shared code access, facilitate AddOns, etc.
3. Soft extension bootstrapping (non-destructive to Users Blender installation)

## Setup

Make sure that the DccScriptingInterface Gem is enabled in your project via the Project Manager, then build your project (DCCsi python boostrapping will require the project to be built.) [Adding and Removing Gems in a Project - Open 3D Engine](https://www.o3de.org/docs/user-guide/project-config/add-remove-gems/)  The O3DE tools provided with the DCCsi have python package dependencies (via requirements.txt).  You will need to currently manually configure for Blender before running for the first time.

In the root folder of the DCCsi are a few useful utilities and files to make note of:

****<u>python.cmd</u>**** will launch O3DE's Python runtime/interpreter, which is found in the following location: c:\o3de\python You could  `import foundation.py`  and use the modules interface directly (only recommended if you know what you are doing, read the code.)

**<u>dccsi.cmd</u>** will start a windows cmd configured with an environment to operate the DCCsi utilities. This is the recommend approach for most people.

## Recommended steps:

1. Double-click `dccsi.cmd` this will start the command prompt console (if you run into trouble, you may need select it and right-click and choose 'Run as Administrator')

2. From the cmd run the following command:

3. Example: dccsi>`python foundation.py -py="C:\Program Files\Blender Foundation\Blender 3.1\3.1\python\bin\python.exe"`

4. Notes:
   
   1. If Blender was already running you will need to restart it.
   2. Remember, this is an experimental prototype.  If you encounter errors, make sure you are signed into github and create an O3DE GitHub Issue, log a bug or feature request: [o3de GHI link](https://github.com/o3de/o3de/issues/new/choose) and assign the label [feature/tech-art]([Issues · o3de/o3de · GitHub](https://github.com/o3de/o3de/labels/feature%2Ftech-art)) 

### If you are an End User...

You should be able to now successfully start Blender from within the O3DE Editor 'Studio Tools' menu.

    `O3DE MenuBar > StudioTools > DCC > Blender`

If you'd like to use Blender with O3DE bootstrapped tools externally, outside of the o3de Editor, you can do that also.

There two ways, a windows environment via .bat file, or a `start.py` script

From .bat file, double-click the following file type to start Maya: `C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Blender\win_launch_blender.bat`

To start from script:

    1. Open a Windows Command Prompt (CMD)

    2. Change directory to: 

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

    3. Run the Blender `start.py` script:

```batch
.\python Tools\DCC\Blender\start.py
```

Note: DCCsi is pre-configured for Blender 3.1

IF you want to alter the version of Belnder, or other settings, such as a customer installation path, you can do this via a file: settings.local.json
start.py is the same setup that the Editor uses to start the external DCC application.  It makes use of a `settings.json` (default settings), and `settings.local.json` (user settings and overrides) within the o3de DCCsi folder for Blender.  These are utilized along with the addition of a `config.py` and `start.py` in the DCC apps folder. This follows the patterns similar to how Maya (and other tools) can be launched from the O3DE menus, or in a scripted manner rather then legacy windows .bat files.

To generate a `settings.local.json` (which you can then modify with overrides to paths and other settings)::

    1. Open a Windows Command Prompt (CMD)

    2. Change directory to: 

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

    3. Run the Blender `config.py` script:

```batch
python Tools\DCC\Blender\config.py
```

You can now open `settings.local.json` in a text editor and make modifications and resave before starting Maya.

### If you are a developer...

< to do: add complete instructions >

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

---

###### LICENSE INFO

Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

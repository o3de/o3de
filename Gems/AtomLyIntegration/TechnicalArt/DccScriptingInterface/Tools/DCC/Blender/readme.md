# O3DE Blender DCCsi Integration

This sets up Blender to be integrated with O3DE in a managed way, via the DccScriptingInterface Gem (aka DCCsi). For more information about the DCCis please see the readme.md at the root of the Gem.

###### Status: Prototype

###### Version: 0.0.1

###### Support: Blender 3.x, Currently Windows only

This is currently the first working version of a managed integration of Blender, treat it as an Experimental Preview.

## Brief

This is an experiment Blender integration with O3DE, it's intent is:

1. Configure and launch Blender (from CLI, or even via O3DE editor)
2. Bootstrap O3DE 'Studio Tools', provide shared code access, facilitate AddOns, etc.
3. Soft extension bootstrapping (non-destructive to Users Blender installation)

## Setup

Make sure that the DccScriptingInterface Gem is enabled in your project via the Project Manager, then build your project (DCCsi python boostrapping will require the project to be built.) [Adding and Removing Gems in a Project - Open 3D Engine](https://www.o3de.org/docs/user-guide/project-config/add-remove-gems/)

The O3DE tools provided with the DCCsi have python package dependencies (via requirements.txt).  You will need to currently manually configure for Blender before running for the first time.

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

You should be able to now successfully start Blender from within the O3DE Editor.

`O3DE MenuBar > StudioTools > DCC > Blender`

Note: DCCsi is pre-configured fro Blender 3.1

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

LICENSE INFO

Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

---
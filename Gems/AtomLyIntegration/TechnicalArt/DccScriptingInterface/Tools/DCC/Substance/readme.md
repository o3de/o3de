----

Copyright (c) Contributors to the Open 3D Engine Project.For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

----

# O3DE DCCsi and Substance Designer

This document contains instructions and other information related to using Adobe Substance Designer with Open 3D Engine (O3DE).  O3DE has a Gem called the DccScriptingInterface (DCCsi) that helps user manage the usage of many popular Digital Content Creation tools (DCC) such as Substance.  These are primarily workflow integrations with the intent to provide an improved ease-of-use experience when these tools are used to author source content for O3DE.  The DCCsi helps with aspects such as, configuration and settings, launching tools and boostrapping them with O3DE extensions (generally python scripts using the DCC tools APIs to automate tasks)

## Status:

- Prototype

## Revisions:

- This is the first working Proof of Concept

## Notes:

- Currently only the latest version of Substance Designer installed via Adobe CC has been tested: 12.1.1 build 5825 commit 9adecf0b Release (2022-05-31).
- This should work with other modern Adobe versions in the same Adobe install location:
  - C:\Program Files\Adobe\Adobe Substance 3D Designer\Adobe Substance 3D Designer.exe
- See the Appendix if you are trying to work with older legacy versions.

## Setup

### General Info

The DCCsi utilizes a set of package dependancies defined in a requirements.txt file

[< o3de> / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / requirements.txt](https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/requirements.txt)

When the O3DE engine is built (cmake), these packages are installed into the O3DE python interpretter/runtime, so python scripts utilizing them have access to these packages; i.e. when a python script is executed by the main Editor.exe python runtime.

Many DCC applications such as Substance Designer also come with a managed python interpretter:

- O3DE doesn't use a user or system installed python interpretter, nor do most DCC apps we are aware of.

- The DCC apps python version may be different then O3DE e.g. python 3.7.12 vs 3.9.9.

- O3DE and most DCC apps we are aware of don't make use of virtual enviornments, which is a common way to manage package dependancies for different versions of python.

- The O3DE DCCsi attempts to not directly modify the DCC app installation (e.g. we don't install packages directly into Maya or Substance in the host platforms %ProgramFiles%)

- To facilitate package dependancies the dccsi provides a script called foundation.py which utilizes the DCC apps python.exe and pip (setup tools) to install packages into a target directory which the DCCsi code can access 

### <dccsi>/foundation.py

< To do: document setting up using foundation.py >

## Getting Started

The DCCsi currently manages **<u>Substance Designer</u>** from this location:

[< o3de > / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / Tools / DCC / Substance]([o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance at development · o3de/o3de · GitHub](https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance))

**Windows**: Currently you can start Substance Designer with O3DE extensions using the .bat file in the following location:

[< o3de > / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / Tools / DCC / Substance / windows_launch_substance_designer.bat](https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance/windows_launch_substance_designer.bat)

**Mac**: < to do: not yet implemented or tested >

**Linux**: < to do: not yet implemented or tested > 

### Preferences

< to do: document how project preferences work >

# Appendix

## Env_Dev.bat

< to do: document modifying the .bat launch environment >

## Legacy

< to do: document working with legacy pre-Adobe Allegorithmic versions >

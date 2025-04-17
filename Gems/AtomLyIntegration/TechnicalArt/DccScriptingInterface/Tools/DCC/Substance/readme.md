----

Copyright (c) Contributors to the Open 3D Engine Project.  For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

----

# O3DE DCCsi and Substance Designer

This document contains instructions and other information related to using Adobe Substance Designer with Open 3D Engine (O3DE).  O3DE has a Gem called the DccScriptingInterface (DCCsi) that helps user manage the usage of many popular Digital Content Creation tools (DCC) such as Substance.  These are primarily workflow integrations with the intent to provide an improved ease-of-use experience when these tools are used to author source content for O3DE.  The DCCsi helps with aspects such as, configuration and settings, launching tools and bootstrapping them with O3DE extensions (generally python scripts using the DCC tools APIs to automate tasks)

## Status:  Prototype

### Version: 0.0.1

This iteration is developer focused, such as a Technical Artist.

It does not have end-user artist functionality or creature comforts yet.

## Revision Info:

- This is the first working Proof of Concept

- This version is only the integration patterns for configuration, settings, launch and bootstrapping.  No tooling within Substance is implemented yet.

- Currently only the latest version of Substance Designer installed via Adobe CC has been tested: 12.1.1 build 5825 commit 9adecf0b Release (2022-05-31).

- This should work with other modern Adobe versions in the same Adobe install location:
  
  - C:\Program Files\Adobe\Adobe Substance 3D Designer\Adobe Substance 3D Designer.exe

- See the Appendix if you are trying to work with older legacy versions.

### Getting Started

#### Configuration

1. Make sure that Adobe Substance3D (aka Substance Designer) is installed. As noted above, currently we have only tested the latest version installed via Creative Cloud Desktop and only in the default install location.

2. Run the following command, this will generate default config/settings and install the DCCsi python package dependencies:
   
   1. dccsi > `python.cmd Tools/DCC/Substance/config.py`

### Launch

#### Method 1

From a CMD, this will start up Adobe Substance3D

```batch
> cd < o3de >\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
> python Tools\DCC\Substance\start.py
```

This is basic validation of DCCsi access:

- Substance.exe > Windows > Python Editor

Here is the basic script (azpy is the DCCcsi shared python api):

```python
import azpy
print(azpy)
```

### Method 2

I can start the latest version of Adobe CC Substance Designer with the following .bat file:

https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance/windows_launch_substabce_designer.bat

### General Info

The DCCsi utilizes a set of package dependencies defined in a requirements.txt file

[< o3de> / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / requirements.txt](https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/requirements.txt)

When the O3DE engine is built (cmake), these packages are installed into the O3DE python interpreter/runtime, so python scripts utilizing them have access to these packages; i.e. when a python script is executed by the main Editor.exe python runtime.

Many DCC applications such as Substance Designer also come with a managed python interpreter:

- O3DE doesn't use a user or system installed python interpreter, nor do most DCC apps we are aware of.

- The DCC apps python version may be different then O3DE e.g. python 3.10.13 vs 3.9.9.

- O3DE and most DCC apps we are aware of don't make use of virtual environments, which is a common way to manage package dependencies for different versions of python.

- The O3DE DCCsi attempts to not directly modify the DCC app installation (e.g. we don't install packages directly into Maya or Substance in the host platforms %ProgramFiles%)

- To facilitate package dependencies the dccsi provides a script called **<u>foundation.py</u>** which utilizes the DCC apps python.exe and pip (setup tools) to install packages into a target directory which the DCCsi code can access 

The DCCsi currently manages **<u>Substance Designer</u>** from this location:

[< o3de > / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / Tools / DCC / Substance]([o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance at development · o3de/o3de · GitHub](https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance))

**Windows**: Currently you can start Substance Designer with O3DE extensions using the .bat file in the following location:

[< o3de > / Gems / AtomLyIntegration / TechnicalArt / DccScriptingInterface / Tools / DCC / Substance / windows_launch_substance_designer.bat](https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Substance/windows_launch_substance_designer.bat)

**Mac**: < to do: not yet implemented or tested >

**Linux**: < to do: not yet implemented or tested > 

### Preferences

< (future, not implemented) document how project .setreg configuration works >

# Appendix

## Troubleshooting

If you are encountering issues ...

1) Get on the O3DE Discord:
   
   1) sig-graphic-audio: [Discord, o3de sig-graphics-audio ](https://discordapp.com/channels/805939474655346758/849888482699902977)
   
   2) #technical-artists: [Discord, o3de #Technical Artists](https://discordapp.com/channels/805939474655346758/842110573625081876)

2) Write a GitHub Issue, assign to me

## Env_Dev.bat

If you are a developer working on the DCCsi, and something breaks (start.py, config.py, foundation.py, etc.), you can use the existing .bat file env on windows as a fall back; albiet this is less flexible and more rigid (you'll have to hard-code some envar overrides)

You can set/override any of the envars in this .bat dev environment using a Dev_Env.bat file, for instance:  `< DCCsi >\Tools\Dev\Windows\Env_Dev.bat `

For instance you can explicitly set and override the path to the exe for substance:

```batch
:: path to .exe, "C:\Program Files\Adobe\Adobe Substance 3D Designer\Adobe Substance 3D Designer.exe"
set DCCSI_SUBSTANCE_EXE="C:\Program Files\Adobe\Adobe Substance 3D Designer\Adobe Substance 3D Designer.exe"
```

The launch using the .bat fallback environment:

`< o3de >\Tools\DCC\Substance\win_launch_substance3d.bat`

## Legacy

Currently if you are not using the latest version of Adobe Substance in the default install location, then the method above windows .bat might work.  Or you may similary go about setting envars and path overrides in the following json file:

`< o3de >\Tools\DCC\Substance\settings.local.json`

and override a value like:

```json
{
    "key":""value",
    "DCCSI_SUBSTANCE_EXE": "C:/Program Files/Adobe/Adobe Substance 3D Designer/Adobe Substance 3D Designer.exe",
    "key":""value"
}
```

< (future) further documentation on working with legacy pre-Adobe Allegorithmic versions >

# O3DE (Open 3D Engine)

O3DE (Open 3D Engine) is an open-source, real-time, multi-platform 3D engine that enables developers and content creators to build AAA games, cinema-quality 3D worlds, and high-fidelity simulations without any fees or commercial obligations.

## O3DE DCCsi Dev Windows Enviroments

DccScriptingInterface (DCCsi) is a framework for O3DE extensions, for example:

- Lightweight python integrations with DCC tools like Maya
- O3DE configuration, customization and extensions of tools
- Standalone PySide2/Qt tools
- ^ These might utilize a mix of O3DE and DDC python APIs

The DccScriptingInterface\config.py, procedurally provides a synthetic env context.
This env is a data-driven approach to configuring layered and managed env settings.

This env provides the hooks for DDC apps and/or standalone tools,
to configure acess to O3DE code (for boostrapping), safely retreive known paths, set/get developer flags, etc.

DccScriptingInterface\Tools\Dev\Windows\

This is a .bat file based version of the default env context for development on windows.
This is what we use to boot the default env context such that it is available, when launching a development tool such as a IDE.

This allows a developer to troubleshoot/debug code, like config.py

Other tools, can use config.py to stand up the env context.

What is in this folder ...

### Core env modules

Env_O3DE_Core.bat       : core access to O3DE and DCCsi
Env_O3DE_Python.bat     : access to O3DE python and general py configuration
Env_O3DE_Qt.bat         : access to O3DE Qt .dll files and PySide2

### DCC add on envars

Env_DCC_Maya.bat        : configures Maya with code O3DE/DCCsi access
Env_DCC_Blender.bat     : configures Blender
Env_DCC_Substance.bat   : Configures Substance Designer

### IDE env

Env_IDE_WingIDE.bat     : configures WingIDE for DCCsi development
Env_IDE_VScode.bat      : configures VScode  for DCCsi development
Env_IDE_PyCharm.bat     : configures PyCharm for DCCsi development

### Launchers

Launch_env_Cmd.bat      : Starts a cmd with entire managed env context
                        : ^ allows use to validate env
                        : ^ display all default ENVAR plugs
                        : ^ allows user to test O3DE python + scripts from cmd
Launch_PyMin_Cmd.bat    : Starts minimal cmd with O3DE python access only
                        : ^ for instance, test Dccsi\config.py like this:
                        : {DCCsi prommpt}>python config.py
Launch_Maya_2020.bat    : Starts Maya2020 within managed env context
Launch_WindIDE-7-1.bat  : Starts WingIDE  within managed env context

### Instructions

How to test the synthetic environment and settings externally

1. Run the cmd:   Launch_PyMin_Cmd.bat

        C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface>

2. Run command:>python config.py -dm=True -py=True -qt=True

What this does?

- runs the O3DE python exe
- starts the config.py which begins to procedurally create synthetic/dynamic environment (hooks)
- ^ this starts with DCCsi hooks
- enters 'dev mode'(-dm) and attempts to attach debugger (Wing IDE only for now, others planned)
- enables additional O3DE python hooks and code access
- ^ great for standalone tools, but you don't want that functionality to interfer with other DCC tools python environments (like Maya!)
- enables access to O3DE Qt .dlls and PySide2 python package support(-qt)
- ^ great for standalone PySide2 which can operate outside of the O3DE editor

## Contribute

For information about contributing to Open 3D Engine, visit [https://o3de.org/docs/contributing/](https://o3de.org/docs/contributing/).

## License

For terms please see the LICENSE*.TXT files at the root of this distribution.

---
LICENSE INFO

Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

---

# DccScriptingInterface ( aka DCCsi )

### Status: Prototype

### Version: 0.0.1

### OS: Windows only (for now)

The *DccScriptingInterface* is a Gem for O3DE to extend and interface with dcc tools in the python ecosystem. Each dcc tool may have it's own specific version of python, most are some version of py3+. O3DE provides an install of py3+ and manages package dependencies with `requirements.txt` files and the cmake build system.

## What is the DCCsi?

- A shared Development Environment for technical art.
- Leverage the existing Python Ecosystem for technical art.
- Work with Python across a number of DCC tools as well as O3DE.
- Integrate a DCC app like Substance (or Substance SAT api) from the Python driven VFX and Games ecosystem.
- Extend O3DE and unlock its potential for content creators, and the Technical Artists that service them.

### Tenets:

1. **<u>Interoperability</u>**: Design DCC-agnostic modules and DCC-bespoke modules  to work together along with O3DE efficiently and intuitively.

2. **<u>Encapsulation</u>**:
   
   1. Define a module in terms of its essential features and interface to other components, to facilitate logical layered design and improve maintenance.
   
   2. The DCCsi avoids direct manipulation of the users installation of any DCC tool, instead it attempts to use data-driven configuration and bootstrapping to launch a tool with O3DE extensions.  
   
   3. Tools can run independently and standalone, with minimal dependencies. 

3. **<u>Extensibility</u>**: Design the tool set to be easily extensible with new functionality, along with new utilities and functionality.  Individual pieces should have a generic communication mechanism to allow newly written tools to slot cleanly and transparently into the tool chain.

### What is provided (High Level):

DCC-Agnostic Python Framework (as a modular Gem) related to multiple integrations for:

- O3DE Editor extensibility (python scripting, utils and PySide2 tools)

- DCC applications, O3DE related extensibility using their Python APIs/SDKs

- Custom standalone tools and utils (python based) to improve O3DE workflows

### DCCsi Status: Prototype

The DCCsi is currently still considered experimental and an early prototype, mainly because the functionality is still barebones, and the scaffolding is still being routinely refactored to get core patterns in place. 

The DCCsi uses packages like pathlib and is written in an attempt to be cross-platform, however we mainly develop and test on windows (we'd love help with other platforms)

The DCCsi is still early, it doesn't have unit tests or automated testing for any of it's functionality.

Important Notice: Python 2.7 has reached end of life, we will no longer maintain py2.7 support. Maya 2022 was the first version that shipped with py3+ support, thus versions such as Maya 2020 and earlier we don't expect to work.

## Getting Started

In the root folder of the DCCsi are a few useful utilities and files to make note of:

<b><u>python.cmd</u></b> will launch O3DE's Python runtime/interpreter, which is found in the following location: c:\o3de\python

### Foundation.py

<u><strong>foundation.py</strong></u> is a utility is used to setup a DCC app for use with the DCCsi. At the root of the DCCsi is a `requirements.txt` file, this is the python standard for tracking dependencies. When a cmake build of O3DE is configured, these dependency packages are installed in the the managed Python runtime for O3DE.

Because most DCC apps have their own managed Python and may actually be on a different version of Python, even when a app is given access to the DCCsi the framework and/or APIs (like azpy) will fail - because of the missing dependencies.

`foundation.py` remedies this, it will install the` requirements.txt` into a managed folder within the DCCsi that is also procedurally bootstrapped, this folder is tied to the version of Python for the target application.

Here is a snippet showing that pattern:

```
# the path we want to install packages into
STR_PATH_DCCSI_PYTHON_LIB = str('{0}\\3rdParty\\Python\\Lib\\{1}.x\\{1}.{2}.x\\site-packages')

# the default will be based on the python executable running this module    
# this value should be replaced with the sys,version of the target python
# for example mayapy, or blenders python, etc.
_PATH_DCCSI_PYTHON_LIB = Path(STR_PATH_DCCSI_PYTHON_LIB.format(_PATH_DCCSIG,
                                                               sys.version_info.major,
                                                               sys.version_info.minor))
```

Note: each DCC apps integration folder has a` config.py` ( example: `"Tools\Substance\config.py"` ), these modules have their own CLI to operate the configuration of that app, and by default those enable use of portion of the `foundation.py` to install packages.

However,` foundation.py` was generically written to function against any `python.exe` first, then the plumbing into the `config.py` modules happened later.  The following instructions are for developers who might run `foundation.py` for a DCC application that does not yet have integration folder (allowing package dependencies to be installed for that DCC app, before development for that DCC app has begun.)

You need to pass the target interpreter into `foundation.py`

- First open a windows CMD, then change directories to the DCCsi:

```batch
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
```

- From command line the option `--python_exe` or `-py`

- Examples:
  
  ```batch
  .\python foundation.py -py="C:\Program Files\Blender Foundation\Blender 3.1\3.1\python\bin\python.exe"`
  ```
  
  ```batch
  .\python foundation.py -py="C:\Program Files\Autodesk\Maya2023\bin\mayapy.exe"`
  ```

`foundation.py` can also be used to check the target Python for pip, which is needed to install the `requirements.txt`

- `.\python foundation.py --check_pip=True`

- method: `foundation.check_pip()`

Python3 should come with at least ensurepip if pip is not installed this can be used

- `.\python foundation.py --ensurepip=True`

- method: `foundation.ensurepip()`

And there is a fallback, you can also install pip the oldschool way, which will download get_pip.py and use it to facilitate installation

- `.\python foundation.py --install_pip=True`

- method: `foundation.download_getpip()`

### Config.py

<u>**config.py**</u> is a util that uses dynaconf (dynamic configuration) and procedurally stands up a managed environment context, which bootstraps access to the O3DE core and the DCCsi.  It's generally used as a module in a startup to gain access.

Note: most DCC tools come with their own managed Python, and many are Qt based applications.  Incorrectly configuring the environment a tool is run in can adversely affect it's capabilities; for instance some apps may not operate correctly if PYTHONHOME is set/propagated, or if the wrong Qt based ENVARS are set, i.e. either of these situations can cause Maya to fail booting.

- config.py will always procedurally stand up core access to O3DE
  
  - within the module this is the method `config.init_o3de_core()`

- config.py can optionally bootstrap access to O3DE python
  
  - within the module this is `config.init_o3de_python()`

- config.py can optionally bootstrap access to O3DE Qt/PySide2
  
  - within the module this is `config.init_o3de_pyside2()`

#### Command Line

You can run this config.py from the command line, this allows:

- Allows developers to test changes to the `config.py` module

- Allows developers to run configuration and generate settings for the core of the DCCsi environment

- Allows developers to inspect the various configurations and settings

- Test modal configuration, e.g., test the PySide mode

##### Modes

This command provides basic access to O3DE and DCCsi, this is the default config you will want if you are operating another DCC tool (for interop):

-  `.\python config.py`

< stub an image here >

This command will extend the environment with O3DE Python, this config state will allow you to use O3DEs python and packages to run utils, scripts and automation:

-  `.\python config.py -py=True`

This command will further extend the environment in a Qt specific way, it will give you access to the Python, Qt.dll's, and PySide2 (aka Qt For Python) that ship with O3DE, this is the configuration state you would use to launch a standalone GUI tool.

-  `.\python config.py -py=True -qt=True`

Yes, you can author a Qt app that runs external from the o3de editor.exe, you can validate this by running with this additional flag:

-  `.\python config.py -py=True -qt=True --test-pyside2=True`

That will execute the config to stand up access, then start a PySide2 test, you'll see a standalone panel with a button pop-up.

< stub an image here >

#### Config.py: Settings.json

As mentioned above,` config.py` is built around [dynaconf](https://www.dynaconf.com/)

The ENVARS and settings can be managed via a few mechanisms:

- `settings.json` : this file contains the the default settings for some of the ENVARS

- many ENVARS are procedurally derived and set with the logical code within `config.py`

- developers can add new settings and default values to `settings.json` that persist without altering the code in `config.py`, e.g., implementing a new bool like a 'feature toggle'

- `settings.local.json` allows the user to add new settings or locally override the value of any setting.  When the config.py environment is executed and stood up, this is the last file to load to ensure that local settings are always enabled.
  
  - Note: this file is not provided by default, however there is a `settings.local.json.example` file which you can copy and rename
  - warning: 

`config.py `comes with the ability to export the settings and write them to the `settings.local.json` file, *this is enabled by default* and so should happen the first time `config.py` is accessed (but can also be suppressed at command line.)

- the cli flags are `-cls` or `--cache-local-settings`

-  `.\python config.py -cls=False`  # this would disable 

Additional, the settings can be written to and stored in any user specified location, by using the following cli flag:

    .\python config.py --export-settings="C:\temp\dccs.settings.json"

----

Copyright (c) Contributors to the Open 3D Engine Project.  For complete
copyright and license terms please see the LICENSE at the root of this
distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

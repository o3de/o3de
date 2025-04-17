```yaml
Copyright (c) Contributors to the Open 3D Engine Project.For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
```

# DCCsi PySide2-tools

This readme contains information for developers who would like to use the PySide2-tools with their custom scripting/tools in O3DE.  

Note: we do not install these with O3DE or the DCCsi

Use at your own risk, you are responsible for abiding by licensing and usage rights as defined in the repo:  [pyside2-tools/LICENSE-uic at dev · pyside/pyside2-tools · GitHub](https://github.com/pyside/pyside2-tools/blob/dev/LICENSE-uic)

This guide assumes you are working with the O3DE engine source, and the DccScriptingInterface Gem (aka DCCsi) however this should also work with installed builds with minor modifications to the steps provided below.

# Basic steps:

You will want to clone the following repo:

[GitHub - pyside/pyside2-tools](https://github.com/pyside/pyside2-tools)[GitHub - pyside/pyside2-tools](https://github.com/pyside/pyside2-tools)

These instructions are specific to working within the dccsi, you could modify them for more general use.

## Clone repo into the DCCsi

This location in the DCCsi auto-bootstraps 3rdParty libs:

    *3rdParty\Python\Lib\3.x\3.10.x*

So you can safely clone or install a package here, this location is already configured in the .gitignore so that these packages don't accidentally get committed back to the source repo (but it's still best to also follow the steps after cloning to exclude the sub-repo)

```shell
# change dir to dccsi
> cd c:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface

# clone the pyside2-tools repo
> git clone https://github.com/pyside/pyside2-tools .\3rdParty\Python\Lib\3.x\3.10.x\site-packages\pyside2tools
```

Notes: Python wont allow '-' as a character in a package name for imports, as this is a restricted character.  Be careful the instructions above use '**pyside2tool**' and *not 'pyside2-tools*' as the name of local repo folder.

if you'd like to see how other pkgs are installed into this location for dcc tools, see `< dccsi >/foundation.py`

## Exclude from O3DE engine repo

Location and open the following file in a txt editor

open > `c:/path/to/o3de/.git/info/exclude`

Exclude the pyside2-tools repo clone

```git
# git ls-files --others --exclude-from=.git/info/exclude
# Lines that start with '#' are comments.
# For a project mostly in C, the following would be a good set of
# exclude patterns (uncomment them if you want to use them):
# *.[oa]
# 
# *~
C:/path/to/o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/3rdParty/Python/Lib/3.x/3.10.x/site-packages/pyside2tools
```

## Use the pyside2uic as a Python Package

You will need to make two slight changes to be able to use the pyside2uic as a import package in python tools.

First make an init file in the root:

`3.10.x\site-packages\pyside2tools\__init__.py`

(optional) open that init in an editor and add the following code

```python
__all__ = ['pyside2uic']
```

Now find the following file, copy and rename

From:

```
    site-packages\pyside2tools\pyside2uic\__init__.py.in
```

To:

```
    site-packages\pyside2tools\pyside2uic\__init__.py
```

Now from scripts within the DCCsi, or tools that bootstrap the DCCsi, this should be usable as a Python import package.

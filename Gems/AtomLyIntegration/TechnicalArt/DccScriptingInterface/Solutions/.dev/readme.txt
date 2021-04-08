All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-------------------------------------------------------------------------------

We .gitignore the folder .dev:
DccScriptingInterface\Solutions\.dev

Locally we put two things here foor development.

1. pyi inspection files, this makes it easier to add a single folder
to IDE configurations for api inspection and auto-complete

DccScriptingInterface\Solutions\.dev\pyi
DccScriptingInterface\Solutions\.dev\pyi\maya
DccScriptingInterface\Solutions\.dev\pyi\pymel
DccScriptingInterface\Solutions\.dev\pyi\PySide2
etc

2. Pyside2-tools

DccScriptingInterface\Solutions\.dev\QtForPython\pyside2-tools-dev

These are not currently part of the O3DE QtForPython Gem
But they can be valuable for development.

Ticket to include them in O3DE: https://jira.agscollab.com/browse/ATOM-13861

You can manually clone them from here: https://github.com/pyside/pyside2-tools

Note: if you want to use them as a python package
1. you will need to add as a site-package (PYTHONPATH)
2. you will need to rename a file:
	this:    pyside2-tools/pyside2uic/__init__.py.in
	becomes: pyside2-tools/pyside2uic/__init__.py

Some examples are in: DccScriptingInterface\azpy\shared\ui\templates.py
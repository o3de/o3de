"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

This folder contains the DccScriptingInterface (DCCsi) for O3DE

Notice: The old \\SDK folder is being replaced with \\Tools
The scripts in \\SDK may be out of data (and not run)
When scripts are finished being updated and refactored into \\Tools the \\SDK will be removed

What is the DCCsi?

- A shared development environment for technical art oriented to working with Python across a number of DCC tools.
- Leverage the existing python ecosystem for technical art.
- Integrate a DCC app like Substance (or Substance SAT api) from the Python driven VFX and Games ecosystem.
- Extend O3DE and unlock its potential for content creators, and the Technical Artists that service them.

Tenets:
(1) Interoperability:	Design DCC-agnostic modules and DCC-bespoke modules
						to work together efficiently and intuitively.

(2) Encapsulation:		Define a module in terms of its essential features and interface to other components,
						to facilitate logical layered design and easy maintenance.

(3) Extensibility:		Design the tool set to be easily extensible with new functionality and new tools.
						Individual pieces should have a generic communication mechanism to allow newly written tools to slot cleanly and transparently into the tool chain.

What is provided (High Level):
- DCC-Agnostic Python Framework (as a modular Gem) related to multiple integrations for:
	O3DE Editor (python scripting, utils and PySide2 tools)
	DCC applications and their Python APIs/SDKs
	Custom standalone tools and utils (python based)
		external from cmd line
		external standalone
		integrated to run within O3DE Editor

What is provided (by folder):

\3rdParty:					Allows third party libs/packages to be integrated outside of O3DE
							Example: O3DE is py3, Maya 2020 (and earlier) is py27
							O3DE provides a patterns for Gems to provide a requirements.txt
							See:
								DccScriptingInterface\reqiurements.txt
								^ These packages will be fetched and installed into O3DE python at build time
				
							This means for some applications like Maya we need another way to add the same packages
							See:
								DccScriptingInterface\SDK\Maya\readme.txt
								DccScriptingInterface\SDK\Maya\requirements.txt
								DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages\*

							Packages that reside in 3rdParty are never commited to the repo (only fetched+installed)

\Assets:					All O3DE Gems can maintain an asset folder
							If a Gem contains an \Asset folder, these assets are folded into the projects asset data
							These assets are processed by the Asset Processor for use in the Editor and Runtime
							In the DCCsi the \Assets folder primarily contains TestData

\azpy						Core (shared) API, A pure python Package and Modules

\Code 						Contains the bare bones C++ scaffold to build and integrate the Gem with O3DE
							Notes:	portions of the DCCsi can be utilized outside of O3DE
									thus this Gem doens't have to be enabled and built for some use cases

\Editor						This folder provides an entry point pattern for extending O3DE Editor with python
							When a Gem is enabled ...
							If the following if found, it will be executed when the Editor boots:
								"Editor\Scripts\bootstrap.py"

							This can be used to initialize code access, extend the editor (PySide2), etc.

\Tools						This is where the following is maintained:

\Tools\DCC 					Integration for DCC tools:
								configuration of tool (managed env, etc.)
								bootstrapping, such as providing the tool access to azpy api code
								extensibility, such as adding new functionality or tool to the app

\Tools\DCC\Maya				An example of adding a integration for Autodesk Maya

\Tools\Env\Windows			This provides a .bat file managed env to configure and bootsrap windows apps
\Tools\Launchers\windows	Provides .bat files based tool launchers for windows (accesses env)


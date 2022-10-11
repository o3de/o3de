DccScriptingInterface (DCCsi)

This location can be extended with additional 3rdParty Python Utils, Tools, Packages, etc.

These are not installed or distributed with O3DE

However, there is some stubbed scaffolding in place.

This is a bootstrapped sandbox for installing python libs (version bootstrapped procedurally):
C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\2.x\2.7.x\site-packages
C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\3.x\3.10.x\site-packages

Any libs installed to this location will be accessible (use at your own risk)

For instance, if you want to add py2.7 compatible libs, for apps like Maya2020:
"C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Maya\readme.txt"

These pyside2-tools can be useful, and they are not pip installed, nor distributed.
"C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\pyside2-tools"

pyside2-tools instructions:

1. clone the repo in this location: C:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python
    >git clone https://github.com/pyside/pyside2-tools

2. to use as a python package ...
    find this and copy:
        "< local DCCsi >\3rdParty\Python\pyside2-tools\pyside2uic\__init__.py.in"

    and rename to this:
    	"< local DCCsi >\3rdParty\Python\pyside2-tools\pyside2uic\__init__.py"

3. add to PYTHONPATH: < local DCCsi >\3rdParty\Python
	in .py something like: site.addsitedir(DCCSI_PYSIDE2_TOOLS)

See: "< local DCCsi >\config.py"

Substance Automation Toolkit (SAT) Instructions:
Substance has a licensed python API for automating material data workflows.  By default, their instructions cover installing it to a python 'system interpreter', however if we want to install it for use within the DCCsi (for custom tools) or even potentially to build inter-op and integrations with O3DE editors, we want to install it in a way that is accessible.

Install Substance Automation Toolkit 3rd party library to DCCsi 3rdParty sandbox:

> C:\Depot\o3de\python> pip install "c:\< path to >\SubstanceAutomationToolkit\Python API\Pysbs-2021.2.2-py2.py3-none-win_amd64.whl" --target="C:\Depot\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\3rdParty\Python\Lib\3.x\3.10.x\site-packages"

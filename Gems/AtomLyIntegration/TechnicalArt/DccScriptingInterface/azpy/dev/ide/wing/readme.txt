This particular sub-package is devoted to WingIDE

It is mainly wingide specific extensions

Some notes ...

There are a couple of projects set up for Lumberyard python development with WingIDE

The FIRST is the DCCsi:
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Solutions\.wing\DCCsi_7x.wpr

This provides devs a project to directly work on the DCCsi itself

You can launch wing and directly load this project via the following .bat file:
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Launchers\Windows\Launch_WingIDE-7-1.bat

Note: the data-driven env hooks for the DCCsi are in the Env.bat:
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Launchers\Windows\Env.bat
^ when you launch wing via the .bat file it bootstraps that env first

Additionally, that same env is being transitioned to a python implementation using dynaconf (WIP):
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\.env
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\settings.json
dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\config.py
^ this last file is the root dynaconf config for the DCCsi
This will also allow us to have per-tool, per-project, per-dcc app env extensions and settings in a more nested way

The SECOND is the AtomTechArt Lumberyard project: dev\AtomTechArt
dev\AtomTechArt\DCCsi\envs\AtomTechArt\AtomTechArt.wpr

Note: this is set up as a venv based on the lumberyard python intstall
and thus provides a sandbox silo to develop outside of the lumberyard python, and outside of the DCCsi

This .bat will launch wing and directly load this project:
dev\AtomTechArt\DCCsi\Launch_AtomTechArt_WingIDE-7-1.bat

WingIDE auto-complete with Lumberyard:
Lumberyard when built will generate .pyi files for source analysis, inspection and auto-complete
On a per-project basis they are generated in the cache like:
dev\Cache\AtomTechArt\pc\user\python_symbols\azlmbr

Note: to get this to work unfortunatley each user must configure the wingide prefs to include this path
(there is no shared project / data-driven way that I know of to set this up otherwise)

Note: lumberyard is not currently generating __init__.pyi files in that package structure:
https://jira.agscollab.com/browse/SPEC-3315

The workaround is to create them yourself (they can be empty) and needs to be in the root of each package folder, like this:
dev\Cache\AtomTechArt\pc\user\python_symbols\azlmbr\__init__.pyi
dev\Cache\AtomTechArt\pc\user\python_symbols\azlmbr\materialeditor\__init__.pyi
ETC...

Then to enable do the following in WingIDE

1. (Dialog) WingIDE > Edit > Peferences
2. (Section) Category > Source Analysis > Advanced
3. (Add Path) In the area labeled "Interface File Path", insert a new path and point it to your cache:
Example (mine): g:\depot\JG_PC1_spectrAtom\dev\Cache\AtomTechArt\pc\user\python_symbols

You might need to reboot wing.  Then you should have auto-complete for the lumberyard api
> import azlmbr

Note: the entirety of azlmbr api does not generate .pyi files currently,
all of the "Behaviour Context" based classes do
non-BC modules such as azlmbr.paths currently do not
https://jira.agscollab.com/browse/SPEC-3316
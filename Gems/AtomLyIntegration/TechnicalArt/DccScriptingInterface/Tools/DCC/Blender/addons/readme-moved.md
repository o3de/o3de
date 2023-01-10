**[Impactful Change] DccScriptingInterface, Blender SceneExporter AddOn Moved**

**The change:**
Inside of the DccScriptingInterface Gem, the folder location for Blender addons has moved.  Currently this change is only in the development branch.

We restructured a folder structure in the DCCsi, to change this path:
`DccScriptingInterface\Tools\DCC\Blender\addons\SceneExporter\*`
to this:
`DccScriptingInterface\Tools\DCC\Blender\Scripts\addons\SceneExporter`

**Background info:**
Blender functionality can be extended with addons.  The DccScriptingInterface Gem includes a SceneExporter addon, that helps Users make clean data exports from their Blender scene to O3DE assets.

**Reason for change:**
These addons can live in a custom script location, which can be set via an ENVAR so they are sourced during bootstrapping.  We made a change to include this ENVAR when starting Blender from O3DE in a managed way.  However, Blender expects a particular folder structure, and the /addons folder is expected to be beneath the /scripts folder to be sourced properly.

**Risks:**
The risk is that Users who have already activated the AddOn using an older approach, may think the SceneExporter has been removed, or is broken.  These Users will need to update the path for the SceneExporter, information on activating the SceneExporter from Blender prefs can be found in the tools readme: https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/Scripts/addons/SceneExporter/README.md

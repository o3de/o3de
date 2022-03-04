# Assigning an O3DE Material in a Default Procedural Prefab

The Prefab gem creates default procedural prefab assets for each source scene asset. This process breaks out each mesh data node into a Mesh Group and attaches the mesh data into an entity with a mesh component. The default material is normally assigned, but it is also possible for the FBX to specify an O3DE material using a reserved custom property’s key "o3de.default.material" and assign the value to a relative asset path to the O3DE material.

The easiest way to get the relative asset path from the O3DE Editor’s Asset Browser. In the asset tree view under the FBX file entry there are a number of ```*.material``` entries which have generated ```*.azmaterial``` assets. Right clicking on an .azmaterial file will allow the user to copy its product asset path.

copy_path_to_clipboard.webp

![Asset Browser](/Gems/Prefab/PrefabBuilder/docs/images/copy_path_to_clipboard.webp)

The "Copy Path to Clipboard" menu option will put the full file path name into the clipboard. The actual name of the asset will be right after the project name (such as AutomatedTesting), be in lower case, and use front slashes for the path. Thus, the name of this material will become "materials/minimalblue.material" for the asset path to use with the material override "o3de.default.material" key.

In the DCC tool, assign the key "o3de.default.material" with the string value of the material asset in a user defined property on the mesh data node. 

It important to remember to export the user defined properties (aka custom properties) from the Export FBX menu from the DCC tool.

For example, in the Blender tool the option looks like this:

![Blender Custom Properties](/Gems/Prefab/PrefabBuilder/docs/images/blender_custom_properties.webp)


"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# -------------------------------------------------------------------------

This folder contains the Blender O3DE Scene Exporter Plugin (DCCsi) for O3DE

![SciFiDockSmall](https://user-images.githubusercontent.com/87207603/175056100-d8dc00fa-5795-4a46-b1ab-724c25fb1604.gif)

The Blender O3DE Scene exporter is a convenience tool
for one click exporter from Blender to O3DE.

![image](https://user-images.githubusercontent.com/87207603/175056642-603dd641-328b-4585-b1e6-4928a463a503.png)

This DCCsi tool is non-destructive to your scene,
as it will export selected copies of your mesh and textures,
re-path texture links to your mesh during file export.
You can export selected static meshes or rigged animated
meshes within the capabilities of the .FBX format and O3DE
actor entity.
You have options to export textures with the mesh,
or with the mesh within a sub folder ‘textures’.
This will work for Blender Scenes with or without packed textures.

Hardware and Software Requirements:

Support for Blender 3.+ Windows 10/11 64-bit version
O3DE Stable 21.11 Release Windows 10/11 64-bit version

INSTALL:

Simply Zip-up the SceneExporter folder into the Blender Plugin.
![image](https://user-images.githubusercontent.com/87207603/175057362-a50b99d1-58d0-46d6-b408-cb211d86f529.png)

How to install Blender O3DE Scene Exporter Blender add-ons:
    Locate the in Add-Ons SceneExporter.zip, this should be located in:
    Go to the Add-ons section in the Blender preferences.
    Click Install button.
    Pick the add-on using the File Browser.
    Enable the add-on from the 'Add-ons' section level.
    You can now see the add on in Import-Export or search for O3DE*.
    Make sure its Checked to Active.
    This will add the new O3DE Tab and the new File Export to O3DE option to the Menu.

HOW TO USE:

When your in Blenders Layout Mode you will see a TAB to access the o3de Scene Exporter tools menu:
![image](https://user-images.githubusercontent.com/87207603/175063907-c071c707-42bc-41f8-9458-64888fe09286.png)

Clicking the O3DE Tab will enable the Scene Exporters tools menu:   
![image](https://user-images.githubusercontent.com/87207603/175063345-5c72172b-8b73-4600-abb7-f139aa7128b6.png)

If you have a more complex Project Path, you can also export your first export with the File > Export > Export to O3DE:
![image](https://user-images.githubusercontent.com/87207603/175064741-160e2f35-196a-413d-a621-db70e1da3ac4.png)

If you export this way once, you will add the custom project path into the tool.
![image](https://user-images.githubusercontent.com/87207603/175065046-1d77b771-7466-4e3c-bd40-07928451abcb.png)
This method also has options found on the Scene Exporter Tool menu.

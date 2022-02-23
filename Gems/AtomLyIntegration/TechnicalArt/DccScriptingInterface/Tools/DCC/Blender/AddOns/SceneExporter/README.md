"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

This folder contains the Blender O3DE Scene Exporter Plugin (DCCsi) for O3DE

The Blender O3DE Scene exporter is a convenience tool
for one click exporter from Blender to O3DE.

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

The SceneExporter folder is zipped up into the Blender Plugin.

How to install Blender O3DE Scene Exporter Blender add-ons:
    Locate the in Add-Ons SceneExporter.zip, this should be located in:
    AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\DCC\Blender\AddOns\SceneExporter.zip
    Go to the Add-ons section in the Blender preferences.
    Click Install button.
    Pick the add-on using the File Browser.
    Enable the add-on from the 'Add-ons' section level.
    You can now see the add on in Import-Export or search for O3DE*.
    Make sure its Checked to Active.
    This will add the new O3DE Tab and the new File Export to O3DE option to the Menu.
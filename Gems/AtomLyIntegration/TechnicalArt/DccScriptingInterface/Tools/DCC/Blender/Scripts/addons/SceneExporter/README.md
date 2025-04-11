# O3DE Blender, Scene Exporter AddOn

# Supports: Tested on Blender 3+, Windows 10/11, Linux Ubuntu 22+ 

This folder contains the Blender O3DE Scene Exporter Plugin (DCCsi) for O3DE

![SciFiDockSmall](https://user-images.githubusercontent.com/87207603/175056100-d8dc00fa-5795-4a46-b1ab-724c25fb1604.gif)

The O3DE Scene Exporter is a Blender plugin for Customers using Blender as a 3D Digital Content Creation tool to export assets to O3DE projects with ease in a streamlined method that is non-destructive to source files. Using preferred export settings for static and animated meshes in efforts to help customers quickly add or iterate assets directly from Blender to O3DE.

# Create Export, Iterate Export, and Repeat.

We often don’t hit our targets on the first time. The creative process is very iterative and has lots of repetition of the same steps over and over again until our vision is completed. With Digital Content Creation tools, we can help make this process much smother, speeding steps up or completely removing them, giving time back to be creative.

# Getting Started

Although it is not a hard requirement, you can enable the DccScriptingInterface Gem (DCCsi) with your O3DE Project (using the o3de.exe project manager).  This includes a tool integration with Blender, this provides and out-of-box developer experience for creating python scripts, tools and other addons. Once the DccScriptingInterface Gem is enabled in your project via the Project Manager, then build your project (DCCsi python bootstrapping will require the project to be built.) [Adding and Removing Gems in a Project - Open 3D Engine](https://www.o3de.org/docs/user-guide/project-config/add-remove-gems/)

With the DCCsi enabled, you can launch Blender from the main O3DE Editor, and Blender will start in a managed way (configuration and settings) and this addon can be enabled in the preferences (without requiring a manual install, although you can still do that also)

More information about DCC tool integrations and setup can be found here in the[DccScriptingInterface Readme](https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md)

More information about the DCCsi Blender tool integration, configuration and setup, can be found here in the: [Blender Readme](https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/readme.md)

## Installing the DccScriptingInterface Gem

![image](https://user-images.githubusercontent.com/23222931/200064189-c2d32414-fa62-4281-ae38-b084ee041b45.png)

## Launch Blender from the Main Editor

![Editor_fYF0aCP2k5](https://user-images.githubusercontent.com/23222931/200064272-81b7921a-1909-4836-ac4e-62b6ac919245.gif)

## Enable the AddOn via Blender Prefs

![blender_T6xHTOTS7k](https://user-images.githubusercontent.com/23222931/200065171-a9e1d802-7a94-4273-bd00-03421fcbfd17.gif)

## Accessing the O3DE Scene Exporter Tool

![blender_uZhsuDbmuU](https://user-images.githubusercontent.com/23222931/200066094-63be4232-d92c-41e5-a9ef-a226c74d2b77.gif)

# Project Asset Organization with O3DE Projects and Custom Project Paths.

Asset management organization is always a challenge. Projects paths from O3DEs manifest are automatic added for you, and if you wish to create customized paths to store in the project list, these paths can be added and saved for future use.

![image](https://user-images.githubusercontent.com/87207603/191541955-998566a0-29a1-4023-83e4-38aa64174856.png)

# 3D and Texture asset exports

In this example we select our office items, select our O3DE Project and Export options and click EXPORT TO O3DE.

![image](https://user-images.githubusercontent.com/87207603/191542135-665383eb-cd34-416f-9928-73dc801d1979.png)
![image](https://user-images.githubusercontent.com/87207603/191542245-ce126078-f8dd-4406-9ee7-b2900e0bb741.png)

Selected .FBX(s) are Exported with updated Texture Paths. Ready to use in your Selected O3DE Project.

![image](https://user-images.githubusercontent.com/87207603/191542364-6096621b-d69e-433a-b07d-c079cd99c2f9.png)

# O3DE User Defined Properties

O3DE User Defined Properties meta data in our DCCs tools helps speeds up the iterative processes of exporting assets with Level of Detail and Physics Colliders and more.

![image](https://user-images.githubusercontent.com/87207603/191542498-1272b606-944d-4d5d-821e-29e5d73d7d48.png)

Above, An Example of a Tree exported with Multiple Levels of UDP LODs o3de_atom_lod

More on O3DE User Defined Properties https://docs.o3de.org/blog/posts/blog-udp/

# O3DE Actor, Skin Attachments, and Animation Exports

Selected your mesh, Animation Options, and EXPORT TO O3DE! The plugin will look at the whole connected tree and export your Rig with Animation to o3de. This speeds up the iterative tweaks that are needed to have Skin Weights and Animation Motions look their best.

![image](https://user-images.githubusercontent.com/87207603/191542958-53242cc0-8da7-443d-b75a-9ea0bf80b7e1.png)

When it comes to Actor Skin Attachments, it’s just as simple, select your Actors Skin Attachment and Skin Attachment Mesh with Rig Animation Option, Export.

![image](https://user-images.githubusercontent.com/87207603/191543074-8ed75ce1-324c-4e1e-9a0a-c22bf8e6e7f5.png)

Bring it all together, an Actor that is Animating in O3DE with Skin Attachments we also can Iterate Creatively on.

![Base_Planet_3](https://user-images.githubusercontent.com/87207603/191543321-92168c58-bc68-40b4-b7a0-bdfbe42ea129.gif)

# Deep Dive into the feature details.

The O3DE Tools Panel will have an easy ABC order of workflow. 

![image](https://user-images.githubusercontent.com/87207603/191543676-05688ac1-d689-40cb-b02f-22cf6f8901d8.png)

Also providing an Export tool on the File Menu as an optional method, this will automatically add the export Path to the Projects Paths as well.

![image](https://user-images.githubusercontent.com/87207603/191543749-22231944-758b-4a5b-944d-895e95a12da8.png)

# A: HELP / INFORMATION

![image](https://user-images.githubusercontent.com/87207603/191543969-e424d646-eb0e-45b8-b8eb-4c20ab2660f6.png)

**O3DE Tools Wiki:**

This page.

**CURRENT SELECTED:**  

This area lets you know Current Selected information, what Project your are currently in, and what type of export options you have currently picked.  
If you have an animated mesh, more animation related information appears.

# B: USER DEFINED (UDP)

![image](https://user-images.githubusercontent.com/87207603/191544089-eb40b0ff-3860-4652-b2fb-0a76a2040249.png)

**O3DE Projects:**

The tool will look at your O3DE Projects and automatically add their assets paths to the tool. However if you would like to add more custom paths for your project assets to the O3DE Projects list, you can add them with the Add Custom Project Path tool.

![image](https://user-images.githubusercontent.com/87207603/191544191-bb04863b-4846-48e6-a5e1-15a82d097934.png)

**Add Custom Project Path:**  

Select which O3DE Project to export assets to. You can add/save custom paths to this list with the Add Custom Project Path tool using your OS File Browser.

# C: USER DEFINED (UDP)

![image](https://user-images.githubusercontent.com/87207603/191544333-166dc6a3-a1fb-445c-b6b0-0733eb2c649d.png)

These are User Defined Properties you can add to your mesh. Currently, you will have to select the Duplicate mesh option for these to show up in O3DE, but this is the groundwork for creating O3DE UDPs in a DCCs tool. You can add a PhysX Collider or add LODs to your selected mesh.

![image](https://user-images.githubusercontent.com/87207603/191544443-9d50a642-9794-499e-b3a3-80cb8decf6f1.png)

# D: EXPORT OPTIONS

![image](https://user-images.githubusercontent.com/87207603/191544553-ff546c1c-edbd-4e56-bf9a-43f2359ae683.png)

**Texture Export Options**:  

Use to selected assets textures and how to handle them when exporting.

![image](https://user-images.githubusercontent.com/87207603/191544634-31ff0d46-8f03-4959-bf7a-aa6795606b29.png)

**Animation Export Options:**  

Use to select how to handle the animation. Please note, you only have to click on the a mesh attached to a rig to export an actor, the plugin will do the rest!

![image](https://user-images.githubusercontent.com/87207603/191544732-a84d1230-2f54-4a87-a91c-1ff3f0e6b485.png)

![image](https://user-images.githubusercontent.com/87207603/191544794-0c6eef93-d812-47ef-81b6-4bddfb0b5c9c.png)

**Export a Single or Multi-Files:**

This option will show up when selecting more than one mesh. You can export a single FBX or an FBX for each selection.

![image](https://user-images.githubusercontent.com/87207603/191544877-00750283-693d-4915-b125-b04c5cf9955e.png)

**Export as Quads or Triangles:**

This option will export your FBX in Quads or Triangles. This is non-destructive to your Blender Model. This is good when **NGONs** might be an issue.

# E: EXPORT FILE

When all the requirements are met for an export, the export button will be unlocked, click this will send your assets to the selected o3de project path. A custom export filename for the asset. If you wish to export multi-able files at once, they will use the assets object name when exported.

![image](https://user-images.githubusercontent.com/87207603/191545003-1296a19b-e66d-4d02-a88f-396ec16478d3.png)

# THE PREFLIGHT REPORT CARD

When exporting your selected model, the In the scene export Preflight tool will give you a quick diagnostic of your file and common issues it may have when importing into O3DE.

![image](https://user-images.githubusercontent.com/87207603/191545091-fd4f59a9-fb16-497d-8863-9af7244e55ad.png)

**Report Card Currently checks for issues:**

**Transforms have not been Applied (Frozen)**

**Model(s) not at world orgin (0.0, -4.2, 0)**

**Model(s) Are missing UV's**

**Have Invalid Bone Names for O3DE. Contain Invalid Characters <>[\]~.`**

**Warning, Your Scene is set to Feet. However O3DE units are in Meters.**

# INSTALL PLUGIN MANUALLY:

To install, simply zip the SceneExporter folder and import as a Blender Plugin. This is the same process of any Blender Plugin.
![image](https://user-images.githubusercontent.com/87207603/191545265-91b1cbff-fd23-4bfc-be3b-5357f4c77449.png)

![InstallPlugin](https://user-images.githubusercontent.com/87207603/191545430-1033c1e5-b195-4cd0-8c1a-864d91ebe487.gif)

----

Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

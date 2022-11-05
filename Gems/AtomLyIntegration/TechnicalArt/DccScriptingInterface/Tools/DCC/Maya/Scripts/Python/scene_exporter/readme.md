# O3DE Scene Exporter Tool



## What does this tool do?

This purpose of the Scene Exporter Tool is to export the content of Autodesk Maya scene files into assets that can be read by the O3DE Editor. This tool will save you time when assets are ready to bring into O3DE on several fronts. It will help you to distribute all files associated with your Maya scene into a properly formatted destination directory within your O3DE project location. It generates an FBX file that contains the user-specified scene elements for transfer, and it will also generate O3DE-specific ".material" files that correspond to your assigned real-time shaders (StingrayPBS or Arnold AIStandardSurface), setting relative paths for O3DE consumption. No matter what size scene you need to carry over, the heavy lifting can all be handled in a couple clicks.



## Tool Interface

The tool is separated by a handful of different settings, and can be adjusted depending on what operations are needed. 



#### Task

This tool can be used for both exporting assets, as well as to generate an audit of all the materials and connected texture files present in the selected scope (scope is explained below). By selecting "Query Material Attributes", no files are created- only a dictionary is logged into the Maya output window that indicates the elements of the scene that will be exported if "Convert to O3DE Materials" is selected.



#### Export Object Settings

There are two options that are available for the way that the tool handles elements that are marked for export in the scene.

###### Preserve Transform Values

Because this tool provides the option of exporting single objects, users may or may not want to preserve transform values (where the object is positioned in relation to the origin of the Maya scene). When this option is checked, the object will retain this position in relation to the O3DE origin, and without this option checked all assets marked for export will be positioned at the center of the origin.

###### Preserve Grouped Objects

When exporting assets you may want to retain hierarchical relationships as they exist in the Maya file. This is not always ideal, but may be desired for a number of reasons. Without this option checked, the elements of the exported objects are flattened to a single level.

#### Scope

The scope setting describes what elements are marked for export, and there are several options for how the export is carried out.

**Selected**

Only those objects selected in the viewport will be exported

**By Name**

Only the object corresponding to the name entered will be exported

**Scene**

All mesh elements in the scene will be exported

**Directory**

All Maya files contained inside the specified directory will have their contents exported. Each Maya scene will generate its own fbx file, and all material files will be grouped into a single textures directory.

#### Export Path

The destination path for converted assets are sent to this location. The proper place to set this directory is within the Project directory that the assets will be used, inside the "Objects" directory. By setting this location as it relates to your project, you will ensure that the assets can be found and textures applied automatically by the texture assignment tool inside O3DE.

#### Output Window

Once a scene has been processed, this window can be used to see all assets that have been exported, along with their set attributes/texture assignments. The combobox can be used to access each of the various exported elements.

---

Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

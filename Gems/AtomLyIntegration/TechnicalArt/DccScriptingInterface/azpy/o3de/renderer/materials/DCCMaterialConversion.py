# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""
+++++++++++++++++++++
All Requests Needed:
+++++++++++++++++++++

-> Convert Legacy Material to Current Material (low)
-> Convert DCC Material to O3DE Material (single, file, batch files)
------> Export material menu item in Maya (currently selected) all the way to all in scene
-> Material Validation (generate all properties, starting with StandardPBR and EnhancedPBR) (chris?)
-> Update existing materials (with new values)
-> Reformat DCC Materials to Compatible Materials (i.e. Blinn to Arnold in Maya)
-> Get DCC Scene Material Info
-> FBX Object Export

Stretch
-> Export to Substance Node Graph
-> Support Legacy Conversion
-> Automatic material assignments (via Asset Registry - what I was working on for Carbonated)


++++++++++++++++++++++++
How Requests Processed:
++++++++++++++++++++++++

Level One:
Point to object/file/files

1. Scene Profiler (Modular Support of Individual DCC Packages, starting with Maya and Blender)
Build to support audit across multiple aspects (i.e. Poly count, Materials, Scene Objects (lights, cameras, etc)
- Component needed for script is Materials

- Start as an internal DCC script that will process a single material (Stingray and Arnold, Blender BRDF) and extract its values

2. With Scene profiled, offer a way to process one or more objects/materials.
- If materials aren't supported for conversion (Blinn, Lambert, etc.) the script can offer to convert to supported
material type (as best it can).



- Existing materials may be compatible, or may need to be converted INTO compatible materials for conversion



Temp directory that can be processed by AP
Tools/Maya/---tools to make accessible in Maya

The base shared material api is
azpy\o3de\renderer\materials\.
azpy\o3de\renderer\materials\generator
azpy\o3de\renderer\materials\class (read template, modify, write new source .material)
Mayas
azpy\dcc\maya\materials\.
azpy\dcc\maya\materials\conversion_map (map from StandardSurface to StandardPBR)
azpy\dcc\maya\materials\export_selected (get_selected, write_srouce)
Blender
azpy\dcc\blender\materials\.
azpy\dcc\blender\materials\conversion_maps
azpy\dcc\blender\materials\export_selected
DCCsi\Tools\DCC\Maya
DCCsi\Tools\DCC\Maya\Scripts\o3de\render\materials\export_action
^ this area adds to the o3de menu in maya, to add an action
^^ that action calls the azpy\dcc\maya\materials\export_selected (edited) 



"""
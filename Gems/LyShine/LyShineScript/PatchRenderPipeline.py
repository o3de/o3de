"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import os
import sys

# Parse arguments
if len(sys.argv) != 3:
    print('Incorrect number of args')
    exit()

engine_path = sys.argv[1]
if not os.path.exists(engine_path):
    print(f'Given path {engine_path} does not exist')
    exit()

project_path = sys.argv[2]
if not os.path.exists(project_path):
    print(f'Given path {project_path} does not exist')
    exit()

sys.path.insert(0, os.path.join(engine_path, 'Gems/Atom/RPI/Tools/'))

from atom_rpi_tools.pass_data import PassTemplate
import atom_rpi_tools.utils as utils

# Folder of this py file
dir_name = os.path.dirname(os.path.realpath(__file__))

# Patch render pipeline to insert a custom LyShine parent pass

# Gem::Atom_Feature_Common gem's path since default render pipeline is comming from this gem
gem_assets_path = os.path.join(engine_path,'Gems/Atom/feature/Common/Assets/')

pipeline_relatvie_path = 'Passes/MainPipeline.pass'
srcRenderPipeline = os.path.join(gem_assets_path, pipeline_relatvie_path)
destRenderPipeline = os.path.join(project_path, pipeline_relatvie_path)
# If the project doesn't have a customized main pipeline
# copy the default render pipeline from Atom_Common_Feature gem to same path in project folder
utils.find_or_copy_file(destRenderPipeline, srcRenderPipeline)

# Load project render pipeline
renderPipeline = PassTemplate(destRenderPipeline)

# Skip if LyShinePass already exist
newPassName = 'LyShinePass'
if renderPipeline.find_pass(newPassName)>-1:
    print('Skip merging. LyShinePass already exists')
    exit()
 
# Insert LyShinePass between DebugOverlayPass and UIPass
refPass = 'DebugOverlayPass'
# The data file for new pass request is in the same folder of the py file
newPassRequestFilePath = os.path.join(dir_name, 'LyShinePass.data') 
newPassRequestData = utils.load_json_file(newPassRequestFilePath)
insertIndex = renderPipeline.find_pass(refPass) + 1
if insertIndex>-1:
    renderPipeline.insert_pass_request(insertIndex, newPassRequestData)
else:
    print('Failed to find ', refPass)
    exit()
 
# Update attachment references for the passes following LyShinePass
renderPipeline.replace_references_after(newPassName, 'DebugOverlayPass', 'InputOutput', 'LyShinePass', 'ColorInputOutput')

# Save the updated render pipeline
renderPipeline.save()

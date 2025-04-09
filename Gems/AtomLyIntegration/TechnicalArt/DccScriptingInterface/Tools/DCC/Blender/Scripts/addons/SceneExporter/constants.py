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
from pathlib import Path
# -------------------------------------------------------------------------

EXPORT_LIST_OPTIONS = ( ( ('0', 'Selected with in texture folder',
    'Export selected meshes with textures in a texture folder.'),
    ('1', 'Selected with Textures in same folder',
        'Export selected meshes with textures in the same folder'),
    ('2', 'Only Meshes', 'Only export the selected meshes, no textures')
    ))
NO_ANIMATION = 'No Animation'
KEY_FRAME_ANIMATION = 'Keyframe Animation'
MESH_AND_RIG = 'Just Mesh with Rig'
SKIN_ATTACHMENT = 'Skin Attachment Mesh with Rig'
ANIMATION_LIST_OPTIONS = ( (
    ('0', NO_ANIMATION, 'Export with no keyframe Animation.'),
    ('1', KEY_FRAME_ANIMATION, 'Mesh needs to be parented to Armature with weights in order for O3DE to detect Entity as an Actor.'),
    ('2', MESH_AND_RIG, 'Key All Bones, Force exporting at least one key of animation for all bones'),
    ('3', SKIN_ATTACHMENT, 'Export a mesh with the Armature bones for use as a O3DE Skin Attachment.'),
    ))

UDP = {'o3de_atom_lod' : '_lod',  'o3de_atom_phys' : '_phys'}
TAG_O3DE = '.o3de'
IMAGE_EXT = ('', '.jpg', '.png', '.JPG', '.PNG')
USER_HOME = Path.home()
DEFAULT_SDK_MANIFEST_PATH = Path.home().joinpath(f'{TAG_O3DE}','o3de_manifest.json')
WIKI_URL = 'https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/AddOns/SceneExporter/README.md'
PLUGIN_VERSION = '1.5'

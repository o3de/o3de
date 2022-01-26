# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from pathlib import Path
# -------------------------------------------------------------------------

EXPORT_LIST_OPTIONS = ( ( ('0', 'Selected with in texture folder',
    'Export selected meshes with textures in a texture folder.'),
    ('1', 'Selected with Textures in same folder',
        'Export selected meshes with textures in the same folder'),
    ('2', 'Only Meshes', 'Only export the selected meshes, no textures')
    ))
TAG_O3DE = '.o3de'
USER_HOME = Path.home()
DEFAULT_SDK_MANIFEST_PATH = Path.home().joinpath(f'{TAG_O3DE}','o3de_manifest.json')
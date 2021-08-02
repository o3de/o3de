"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# from maya_mapping import MayaMapping
# from max_mapping import MaxMapping
# from blender_mapping import BlenderMapping

# def get_dcc_mapping(app):
#     app = app.lower()
#     if app == 'maya':
#         return MayaMapping()
#     elif app == '3dsmax':
#         return MaxMapping()
#     elif app == 'blender':
#         return BlenderMapping()
#     else:
#         return NullMapping(app)

__all__ = ['blender_materials',
           'dcc_material_mapping',
           'drag_and_drop',
           'main',
           'materials_export',
           'max_materials',
           'maya_materials',
           'model']

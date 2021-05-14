"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import WhiteBoxInit as init

import argparse
import azlmbr.bus as bus
import azlmbr.math
import azlmbr.whitebox.api as api

# usage: pyRunFile path/to/file/CsgBoolean.py [union|subtraction|intersection]
#
# Demonstrates Api::MeshBoolean by creating two overlapping unit cubes and applying
# a CSG boolean operation. The first cube is modified in place; on success the
# second (cutter) entity is deleted so only the result remains in the scene.

# values match WhiteBox::Api::BooleanOperation (also reflected as
# BOOLEAN_UNION/BOOLEAN_SUBTRACTION/BOOLEAN_INTERSECTION enum properties)
BOOLEAN_OPERATIONS = {
    'union': 0,
    'subtraction': 1,
    'intersection': 2,
}


def create_cube_entity(name):
    entity = init.create_white_box_entity(name)
    component = init.create_white_box_component(entity)
    white_box = init.create_white_box_handle(component)
    white_box.InitializeAsUnitCube()
    return entity, component, white_box


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Applies a CSG boolean between two unit cubes.')
    parser.add_argument(
        'operation', nargs='?', default='subtraction', choices=sorted(BOOLEAN_OPERATIONS),
        help='which boolean operation to apply')
    args = parser.parse_args()

    entity_a, component_a, white_box_a = create_cube_entity('WhiteBox-Boolean-A')
    entity_b, component_b, white_box_b = create_cube_entity('WhiteBox-Boolean-B')

    # position cube B relative to cube A so the two cubes overlap by a quarter cube
    operand_transform = azlmbr.math.Transform_CreateTranslation(azlmbr.math.Vector3(0.5, 0.5, 0.5))

    # cube A becomes the result of (A boolOp B)
    if white_box_a.MeshBoolean(white_box_b, operand_transform, BOOLEAN_OPERATIONS[args.operation]):
        init.update_white_box(white_box_a, component_a)
        # TrenchBroom-style: consume the cutter entity on success so only the
        # result remains in the scene (otherwise entity_b fills the hole in A
        # and the two objects look like an unmodified cube).
        import azlmbr.editor as editor
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityById', entity_b)
        print(f'CsgBoolean: {args.operation} succeeded')
    else:
        print(f'CsgBoolean: {args.operation} failed (do the meshes overlap?)')

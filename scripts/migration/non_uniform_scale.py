#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import azlmbr
from pathlib import Path

def fixup_current_level(threshold):
    nonUniformScaleComponentId = azlmbr.editor.EditorNonUniformScaleComponentTypeId

    # iterate over all entities in the level
    entityIdList = azlmbr.entity.SearchBus(azlmbr.bus.Broadcast, 'SearchEntities', azlmbr.entity.SearchFilter())
    for entityId in entityIdList:
        name = azlmbr.editor.EditorEntityInfoRequestBus(azlmbr.bus.Event, 'GetName', entityId)
        local = azlmbr.components.TransformBus(azlmbr.bus.Event, 'GetLocalScale', entityId)

        # only process entities where the non-uniformity is greater than the threshold
        local_max = max(local.x, local.y, local.z)
        local_min = min(local.x, local.y, local.z)
        if local_max / local_min > 1 + threshold:

            # check if there is already a Non-uniform Scale component
            getComponentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', entityId, nonUniformScaleComponentId)
            if getComponentOutcome.IsSuccess():
                print(f"skipping {name} as it already has a Non-uniform Scale component")

            else:
                # add Non-uniform Scale component and set it to the non-uniform part of the local scale
                azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast,'AddComponentsOfType', entityId, [nonUniformScaleComponentId])
                vec = azlmbr.math.Vector3(local.x / local_max, local.y / local_max, local.z / local_max)
                azlmbr.entity.NonUniformScaleRequestBus(azlmbr.bus.Event, 'SetScale', entityId, vec)
                print(f"added non-uniform scale component for {name}: {local.x}, {local.y}, {local.z}")

if __name__ == '__main__':
    # handle the arguments manually since argparse causes problems when run through EditorPythonBindings
    process_all_levels = "--all" in sys.argv

    # ignore entities where the relative difference between the min and max scale values is less than this threshold
    threshold = 0.001
    for i in range(len(sys.argv) - 1):
        if sys.argv[i] == "--threshold":
            try:
                threshold = float(sys.argv[i + 1])
            except ValueError:
                print(f"invalid threshold value {sys.argv[i + 1]}, using default value {threshold}")
                pass

    if process_all_levels:
        game_folder = Path(azlmbr.legacy.general.get_game_folder())
        level_folder = game_folder / 'Levels'
        levels = [str(level) for level in level_folder.rglob('*.ly')] + [str(level) for level in level_folder.rglob('*.cry')]
        for level in levels:
            if "_savebackup" not in level:
                print(f'loading level {level}')
                azlmbr.legacy.general.open_level_no_prompt(level)
                azlmbr.legacy.general.idle_wait(2.0)
                fixup_current_level(threshold)
                azlmbr.legacy.general.save_level()
    else:
        fixup_current_level(threshold)

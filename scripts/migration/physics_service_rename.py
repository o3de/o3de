#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
from pathlib import Path

'''
Renames services provided by PhysX components from PhysX...Service to Physics...Service.
The service names were originally chosen to make a distinction from the legacy physics
components, but the legacy components have been removed, and the new naming better reflects
the modular design and intent to support other potential physics Gems.
This function searches all folders below the O3DE root folder for .cpp, .h or .inl files, and
replaces any instances of the old naming convention.
'''
def rename_physics_services():
    renamed_services = {
        "\"PhysXCharacterControllerService\"": "\"PhysicsCharacterControllerService\"",
        "\"PhysXCharacterGameplayService\"": "\"PhysicsCharacterGameplayService\"",
        "\"PhysXColliderService\"": "\"PhysicsColliderService\"",
        "\"PhysXEditorService\"": "\"PhysicsEditorService\"",
        "\"PhysXHeightfieldColliderService\"": "\"PhysicsHeightfieldColliderService\"",
        "\"PhysXJointService\"": "\"PhysicsJointService\"",
        "\"PhysXRagdollService\"": "\"PhysicsRagdollService\"",
        "\"PhysXRigidBodyService\"": "\"PhysicsRigidBodyService\"",
        "\"PhysXService\"": "\"PhysicsService\"",
        "\"PhysXShapeColliderService\"": "\"PhysicsShapeColliderService\"",
        "\"PhysXTriggerService\"": "\"PhysicsTriggerService\""
    }

    o3de_root = Path(__file__).parents[2]
    for root, dirs, files in os.walk(o3de_root):
        for filename in files:
            extension = os.path.splitext(filename)[1]
            if extension in [".h", ".cpp", ".inl"]:
                full_path = os.path.join(root, filename)
                file = open(full_path)
                lines = file.readlines()
                file.close()
                lines_changed = False
                for line_index in range(len(lines)):
                    for old_service, new_service in renamed_services.items():
                        lines_changed = lines_changed or old_service in lines[line_index]
                        lines[line_index] = lines[line_index].replace(old_service, new_service)
                if lines_changed:
                    file = open(full_path, 'w')
                    file.writelines(lines)
                    file.close()

if __name__ == "__main__":
    rename_physics_services()
import os
from pathlib import Path

if __name__ == "__main__":
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

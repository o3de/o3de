"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    new_entity_created = (
        "Successfully created new entity",
        "Failed to create new entity"
    )
    emitter_disabled_before_mesh = (
        "Mesh Surface Tag Emitter is disabled without a Mesh component",
        "Mesh Surface Tag Emitter is unexpectedly enabled without a Mesh component"
    )
    emitter_enabled_after_mesh = (
        "Mesh Surface Tag Emitter is enabled after adding a Mesh component",
        "Mesh Surface Tag Emitter is unexpectedly disabled after adding a Mesh component"
    )


def MeshSurfaceTagEmitter_DependentOnMeshComponent():
    """
    Summary:
    A New level is loaded. A New entity is created with component "Mesh Surface Tag Emitter". Adding a component
    "Mesh" to the same entity.

    Expected Behavior:
    Mesh Surface Tag Emitter is disabled until the required Mesh component is added to the entity.

    Test Steps:
     1) Open level
     2) Create a new entity with component "Mesh Surface Tag Emitter"
     3) Make sure Mesh Surface Tag Emitter is disabled
     4) Add Mesh to the same entity
     5) Make sure Mesh Surface Tag Emitter is enabled after adding Mesh

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as EntityId
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def is_component_enabled(EntityComponentIdPair):
        return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", EntityComponentIdPair)

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create a new entity with component "Mesh Surface Tag Emitter"
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    component_to_add = "Mesh Surface Tag Emitter"
    entity_id = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
    )
    meshentity = hydra.Entity("meshentity", entity_id)
    meshentity.components = []
    meshentity.components.append(hydra.add_component(component_to_add, entity_id))
    Report.critical_result(Tests.new_entity_created, entity_id.IsValid())

    # 3) Make sure Mesh Surface Tag Emitter is disabled
    is_enabled = is_component_enabled(meshentity.components[0])
    Report.result(Tests.emitter_disabled_before_mesh, not is_enabled)

    # 4) Add Mesh to the same entity
    component = "Mesh"
    meshentity.components.append(hydra.add_component(component, entity_id))

    # 5) Make sure Mesh Surface Tag Emitter is enabled after adding Mesh
    is_enabled = is_component_enabled(meshentity.components[0])
    Report.result(Tests.emitter_enabled_after_mesh, is_enabled)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(MeshSurfaceTagEmitter_DependentOnMeshComponent)

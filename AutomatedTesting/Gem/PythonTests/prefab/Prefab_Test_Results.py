"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Results():
    create_entity_succeeded = "'CreateNewEntity' passed"
    create_entity_failed = "'CreateNewEntity' failed"
    create_prefab_succeeded = "'CreatePrefab' passed"
    create_prefab_failed = "'CreatePrefab' failed"
    instantiate_prefab_succeeded = "'InstantiatePrefab' passed"
    instantiate_prefab_failed = "'InstantiatePrefab' failed"
    instantiated_prefab_at_expected_position = "instantiated prefab's position is at the expected position"
    instantiated_prefab_not_at_expected_position = "instantiated prefab's position is *not* at the expected position"
    entity_has_expected_children_count = "target entity owns exactly given count of children"
    entity_not_have_expected_children_count = "target entity does *not* owns given count of children"
    delete_prefab_succeeded = "'DeleteEntitiesAndAllDescendantsInInstance' passed"
    delete_prefab_failed = "'DeleteEntitiesAndAllDescendantsInInstance' failed"
    delete_prefab_entities_and_descendants_succeeded = "Entities and all the descendants in target prefab are deleted"
    delete_prefab_entities_and_descendants_failed = "Entities and all the descendants in target prefab are *not* deleted"
    reparent_prefab_succeeded = "Successfully reparented target prefab instance"
    reparent_prefab_cyclical_dependency_detected = "Cyclical dependency detected while reparenting prefab"
    reparent_prefab_parent_not_own_prefab = "Target parent entity does *not* own reparented prefab's container entity"
    reparent_prefab_not_under_expected_parent = "Prefab reparented is *not* under the expected parent entity"
    entity_not_have_unique_name = "This test entity should have a unique name"


UnexpectedResults = [
    Results.create_entity_failed,
    Results.create_prefab_failed,
    Results.instantiate_prefab_failed,
    Results.instantiated_prefab_not_at_expected_position,
    Results.entity_not_have_expected_children_count,
    Results.delete_prefab_failed,
    Results.delete_prefab_entities_and_descendants_failed,
    Results.reparent_prefab_cyclical_dependency_detected,
    Results.reparent_prefab_parent_not_own_prefab,
    Results.reparent_prefab_not_under_expected_parent,
    Results.entity_not_have_unique_name,
]

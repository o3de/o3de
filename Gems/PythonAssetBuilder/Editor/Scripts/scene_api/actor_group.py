#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import json
import uuid
import os, sys
import scene_api.physics_data
from scene_api.common_rules import RuleEncoder, BaseRule, SceneNodeSelectionList, CommentRule, CoordinateSystemRule

class ActorGroup():
    """
    Configure actor data exporting.

    Attributes
    ----------
    name:
        Name for the group. This name will also be used as the name for the generated file.

    selectedRootBone:
        The root bone of the animation that will be exported.

    rules: `list` of actor rules (derived from BaseRule)
        modifiers to fine-tune the export process.

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary

    add_rule(rule)
        Adds a rule into the internal rules container
        Returns True if the rule was added to the internal rules container

    create_rule(rule)
        Helper method to add and return the rule

    remove_rule(type)
        Removes the rule from the internal rules container

    to_dict()
        Converts the contents to as a Python dictionary

    to_json()
        Converts the contents to a JSON string

    """
    def __init__(self):
        self.typename = 'ActorGroup'
        self.name = ''
        self.selectedRootBone = ''
        self.id = uuid.uuid4()
        self.rules = set()

    def add_rule(self, rule) -> bool:
        if (rule not in self.rules):
            self.rules.add(rule)
            return True
        return False

    def create_rule(self, rule) -> any:
        if (self.add_rule(rule)):
            return rule
        return None

    def remove_rule(self, type) -> None:
        self.rules.discard(rule)

    def to_dict(self) -> dict:
        out = {}
        out['$type'] = self.typename
        out['name'] = self.name
        out['selectedRootBone'] = self.selectedRootBone
        out['id'] = f"{{{str(self.id)}}}"
        # convert the rules
        ruleList = []
        for rule in self.rules:
            jsonStr = json.dumps(rule, cls=RuleEncoder)
            jsonDict = json.loads(jsonStr)
            ruleList.append(jsonDict)
        out['rules'] = ruleList
        return out

    def to_json(self, i = 0) -> any:
        jsonDOM = self.to_dict()
        return json.dumps(jsonDOM, cls=RuleEncoder, indent=i)


class LodNodeSelectionList(SceneNodeSelectionList):
    """
    Level of Detail node selection list

    The selected nodes should be joints with BoneData
    derived from SceneNodeSelectionList
    see also LodRule

    Attributes
    ----------
    lodLevel: int
        the level of detail to target where 0 is nearest level of detail
        up to 5 being the farthest level of detail range for 6 levels maximum

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__()
        self.typename = 'LodNodeSelectionList'
        self.lodLevel = 0

    def to_dict(self):
        data = super().to_dict()
        data['lodLevel'] = self.lodLevel
        return data

class PhysicsAnimationConfiguration():
    """
    Configuration for animated physics structures which are more detailed than the character controller.
    For example, ragdoll or hit detection configurations.
    See also 'class Physics::AnimationConfiguration'

    Attributes
    ----------
    hitDetectionConfig: CharacterColliderConfiguration
        for hit detection

    ragdollConfig: RagdollConfiguration
        to set up physics properties

    clothConfig: CharacterColliderConfiguration
        for cloth physics

    simulatedObjectColliderConfig: CharacterColliderConfiguration
        for simulation physics

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.hitDetectionConfig = scene_api.physics_data.CharacterColliderConfiguration()
        self.ragdollConfig = scene_api.physics_data.RagdollConfiguration()
        self.clothConfig = scene_api.physics_data.CharacterColliderConfiguration()
        self.simulatedObjectColliderConfig = scene_api.physics_data.CharacterColliderConfiguration()

    def to_dict(self):
        data = {}
        data["hitDetectionConfig"] = self.hitDetectionConfig.to_dict()
        data["ragdollConfig"] = self.ragdollConfig.to_dict()
        data["clothConfig"] = self.clothConfig.to_dict()
        data["simulatedObjectColliderConfig"] = self.simulatedObjectColliderConfig.to_dict()
        return data

class EMotionFXPhysicsSetup():
    """
    Physics setup properties
    See also 'class EMotionFX::PhysicsSetup'

    Attributes
    ----------
    config: PhysicsAnimationConfiguration
        Configuration to setup physics properties

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        self.typename = 'PhysicsSetup'
        self.config = PhysicsAnimationConfiguration()

    def to_dict(self):
        return {
            "config" : self.config.to_dict()
        }

class ActorPhysicsSetupRule(BaseRule):
    """
    Physics setup properties

    Attributes
    ----------
    data: EMotionFXPhysicsSetup
        Data to setup physics properties

    Methods
    -------

    set_hit_detection_config(self, hitDetectionConfig)
        Simple helper function to assign the hit detection configuration

    set_ragdoll_config(self, ragdollConfig)
        Simple helper function to assign the ragdoll configuration

    set_cloth_config(self, clothConfig)
        Simple helper function to assign the cloth configuration

    set_simulated_object_collider_config(self, simulatedObjectColliderConfig)
        Simple helper function to assign the assign simulated object collider configuration

    to_dict()
        Converts contents to a Python dictionary

    """
    def __init__(self):
        super().__init__('ActorPhysicsSetupRule')
        self.data = EMotionFXPhysicsSetup()

    def set_hit_detection_config(self, hitDetectionConfig) -> None:
        self.data.config.hitDetectionConfig = hitDetectionConfig

    def set_ragdoll_config(self, ragdollConfig) -> None:
        self.data.config.ragdollConfig = ragdollConfig

    def set_cloth_config(self, clothConfig) -> None:
        self.data.config.clothConfig = clothConfig

    def set_simulated_object_collider_config(self, simulatedObjectColliderConfig) -> None:
        self.data.config.simulatedObjectColliderConfig = simulatedObjectColliderConfig

    def to_dict(self):
        data = super().to_dict()
        data["data"] = self.data.to_dict()
        return data

class ActorScaleRule(BaseRule):
    """
    Scale the actor

    Attributes
    ----------
    scaleFactor: float
        Set the multiplier to scale geometry.

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary

    """
    def __init__(self):
        super().__init__('ActorScaleRule')
        self.scaleFactor = 1.0

class SkeletonOptimizationRule(BaseRule):
    """
    Advanced skeleton optimization rule.

    Attributes
    ----------
    autoSkeletonLOD: bool
        Client side skeleton LOD based on skinning information and critical bones list.

    serverSkeletonOptimization: bool
        Server side skeleton optimization based on hit detections and critical bones list.

    criticalBonesList: `list` of SceneNodeSelectionList
        Bones in this list will not be optimized out.

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('SkeletonOptimizationRule')
        self.autoSkeletonLOD = True
        self.serverSkeletonOptimization = False
        self.criticalBonesList = SceneNodeSelectionList()

    def to_dict(self):
        data = super().to_dict()
        self.criticalBonesList.convert_selection(data, 'criticalBonesList')
        return data

class LodRule(BaseRule):
    """
    Set up the level of detail for the meshes in this group.

    The engine supports 6 total lods.
    1 for the base model then 5 more lods.
    The rule only captures lods past level 0 so this is set to 5.

    Attributes
    ----------
    nodeSelectionList: `list` of LodNodeSelectionList
        Select the meshes to assign to each level of detail.

    Methods
    -------

    add_lod_level(lodLevel, selectedNodes=None, unselectedNodes=None)
        A helper function to add selected nodes (list of node names) and unselected nodes (list of node names)
        This creates a LodNodeSelectionList and adds it to the node selection list
        returns the LodNodeSelectionList that was created

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('{3CB103B3-CEAF-49D7-A9DC-5A31E2DF15E4} LodRule')
        self.nodeSelectionList = [] # list of LodNodeSelectionList

    def add_lod_level(self, lodLevel, selectedNodes=None, unselectedNodes=None) -> LodNodeSelectionList:
        lodNodeSelection = LodNodeSelectionList()
        lodNodeSelection.selectedNodes = selectedNodes
        lodNodeSelection.unselectedNodes = unselectedNodes
        lodNodeSelection.lodLevel = lodLevel
        self.nodeSelectionList.append(lodNodeSelection)
        return lodNodeSelection

    def to_dict(self):
        data = super().to_dict()
        selectionListList = data.pop('nodeSelectionList')
        data['nodeSelectionList'] = []
        for nodeList in selectionListList:
            data['nodeSelectionList'].append(nodeList.to_dict())
        return data

class MorphTargetRule(BaseRule):
    """
    Select morph targets for actor.

    Attributes
    ----------
    targets: `list` of SceneNodeSelectionList
        Select 1 or more meshes to include in the actor as morph targets.

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('MorphTargetRule')
        self.targets = SceneNodeSelectionList()

    def to_dict(self):
        data = super().to_dict()
        self.targets.convert_selection(data, 'targets')
        return data


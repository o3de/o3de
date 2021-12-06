"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import uuid
import os, sys

sys.path.append(os.path.dirname(__file__))
import physics_data

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

    def to_json(self) -> any:
        jsonDOM = self.to_dict()
        return json.dumps(jsonDOM, cls=RuleEncoder, indent=1)

class RuleEncoder(json.JSONEncoder):
    """
    A helper class to encode the Python classes with to a Python dictionary

    Methods
    -------

    encode(obj)
        Converts contents to a Python dictionary for the JSONEncoder

    """
    def encode(self, obj):
        chunk = obj
        if isinstance(obj, BaseRule):
            chunk = obj.to_dict()
        elif isinstance(obj, dict):
            chunk = obj
        elif hasattr(obj, 'to_dict'):
            chunk = obj.to_dict()
        else:
            chunk = obj.__dict__
        
        return super().encode(chunk)

class BaseRule():
    """
    Base class of the actor rules to help encode the type name of abstract rules

    Parameters
    ----------
    typename : str
        A typename the $type will be in the JSON chunk object

    Attributes
    ----------
    typename: str
        The type name of the abstract classes to be serialized

    id: UUID
        a unique ID for the rule

    Methods
    -------
    to_dict()
        Converts contents to a Python dictionary
        Adds the '$type' member
        Adds a random 'id' member
    """
    def __init__(self, typename):
        self.typename = typename
        self.id = uuid.uuid4()
        
    def __eq__(self, other):
        return self.id.__eq__(other.id)

    def __ne__(self, other):
        return self.__eq__(other) is False

    def __hash__(self):
        return self.id.__hash__()
    
    def to_dict(self):
        data = self.__dict__
        data['id'] = f"{{{str(self.id)}}}"
        # rename 'typename' to '$type'
        data['$type'] = self.typename
        data.pop('typename')
        return data

class SceneNodeSelectionList(BaseRule):
    """
    Contains a list of node names to include (selectedNodes) and to exclude (unselectedNodes)

    Attributes
    ----------
    selectedNodes: `list` of str
        The node names to include for this group rule
        
    unselectedNodes: `list` of str
        The node names to exclude for this group rule

    Methods
    -------
    convert_selection(self, container, key):
        this adds its contents to an existing dictionary container at a key position

    select_targets(self, selectedList, allNodesList:list)
        helper function to include a small list of node names from list of all the node names

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('SceneNodeSelectionList')
        self.selectedNodes = []
        self.unselectedNodes = []

    def convert_selection(self, container, key):
        container[key] = self.to_dict()
        
    def select_targets(self, selectedList, allNodesList:list):
        self.selectedNodes = selectedList
        self.unselectedNodes = allNodesList.copy()
        for node in selectedList:
            if node in self.unselectedNodes:
                self.unselectedNodes.remove(node)

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
        self.hitDetectionConfig = physics_data.CharacterColliderConfiguration()
        self.ragdollConfig = physics_data.RagdollConfiguration()
        self.clothConfig = physics_data.CharacterColliderConfiguration()
        self.simulatedObjectColliderConfig = physics_data.CharacterColliderConfiguration()
        
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

class CoordinateSystemRule(BaseRule):
    """
    Modify the target coordinate system, applying a transformation to all data (transforms and vertex data if it exists).

    Attributes
    ----------
    targetCoordinateSystem: int
        Change the direction the actor/motion will face by applying a post transformation to the data.

    useAdvancedData: bool
        If True, use advanced settings

    originNodeName: str
        Select a Node from the scene as the origin for this export.

    rotation: [float, float, float, float]
        Sets the orientation offset of the processed mesh in degrees. Rotates (yaw, pitch, roll) the group after translation.

    translation: [float, float, float] 
        Moves the group along the given vector3.

    scale: float
        Sets the scale offset of the processed mesh.

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('CoordinateSystemRule')
        self.targetCoordinateSystem = 0
        self.useAdvancedData = False
        self.originNodeName = ''
        self.rotation = [0.0, 0.0, 0.0, 1.0]
        self.translation = [0.0, 0.0, 0.0]
        self.scale = 1.0

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

class CommentRule(BaseRule):
    """
    Add an optional comment to the asset's properties.

    Attributes
    ----------
    text: str
        Text for the comment.

    Methods
    -------

    to_dict()
        Converts contents to a Python dictionary
    """
    def __init__(self):
        super().__init__('CommentRule')
        self.text = ''

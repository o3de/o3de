#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json, logging

def log_exception_traceback():
    """
    Outputs an exception stacktrace.
    """
    data = traceback.format_exc()
    logger = logging.getLogger('python')
    logger.error(data)


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
        Note: Override this method if a derviced class needs to return a custom dictionary

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
        data = vars(self)
        data['id'] = f"{{{str(self.id)}}}"
        # rename 'typename' to '$type'
        data['$type'] = self.typename
        data.pop('typename')
        return data

def convert_rule_to_json(rule:BaseRule, indentValue=0):
    """
    Helper function to convert a BaseRule into a JSON string

    Parameters
    ----------
    obj : any
        The object to convert to a JSON string as long as the obj class has an *to_dict* method

    indentValue : int
        The number of spaces to indent between each JSON block/value
    """
    return json.dumps(rule.to_dict(), indent=indentValue, cls=RuleEncoder)

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

    """
    def __init__(self):
        super().__init__('CoordinateSystemRule')
        self.targetCoordinateSystem = 0
        self.useAdvancedData = False
        self.originNodeName = ''
        self.rotation = [0.0, 0.0, 0.0, 1.0]
        self.translation = [0.0, 0.0, 0.0]
        self.scale = 1.0

class TypeId():
    """
    Wraps a UUID that represents a AZ::TypeId from O3DE

    Attributes
    ----------
    valud: uuid.Uuid
        A unique ID that defaults to AZ::TypeId::CreateNull()
    """
    def __init__(self):
        self.value = uuid.UUID('{00000000-0000-0000-0000-000000000000}')

    def __str__(self):
        return f"{{{str(self.value)}}}"


class RuleEncoder(json.JSONEncoder):
    """
    A helper class to encode the Python classes with to a Python dictionary

    Methods
    -------

    default(obj)
        Converts a single object to a JSON value that can be stored with a key

    encode(obj)
        Converts contents to a Python dictionary for the JSONEncoder

    """
    def default(self, obj):
        if (isinstance(obj,TypeId)):
            return str(obj)
        elif hasattr(obj, 'to_json_value'):
            return obj.to_json_value()
        return super().default(obj)

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

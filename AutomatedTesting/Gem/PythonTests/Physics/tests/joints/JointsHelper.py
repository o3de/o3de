"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from editor_python_test_tools.utils import Report
import azlmbr.legacy.general as general
import azlmbr.bus

def vector3SmallerThanScalar(vec3Value, scalarValue):
    return (vec3Value.x < scalarValue and 
        vec3Value.y < scalarValue and 
        vec3Value.z < scalarValue)

def vector3LargerThanScalar(vec3Value, scalarValue):
    return (vec3Value.x > scalarValue and 
        vec3Value.y > scalarValue and 
        vec3Value.z > scalarValue)

def getRelativeVector(vecA, vecB):
    relativeVec = vecA
    relativeVec.x = relativeVec.x - vecB.x
    relativeVec.y = relativeVec.y - vecB.y
    relativeVec.z = relativeVec.z - vecB.z
    return relativeVec


# Entity class for joints tests
class JointEntity:
    def criticalEntityFound(self): # For overriding in sub-classes so that can report if entities are found using their own dictionary of entities
        pass

    def __init__(self, name):
        self.id = general.find_game_entity(name)
        self.name = name
        self.criticalEntityFound()

    @property
    def position(self):
        # type () -> Vector3
        return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

# Entity class that sets a flag when an instance receives collision events.
class JointEntityCollisionAware(JointEntity):
    def on_collision_begin(self, args):
        self.collided = True
    
    def on_collision_persist(self, args):
        self.collided = True

    def on_collision_end(self, args):
        self.collided = True

    def __init__(self, name):
        self.id = general.find_game_entity(name)
        self.name = name
        self.criticalEntityFound()

        self.collided = False
        # Set up collision notification handler
        self.handler = azlmbr.physics.CollisionNotificationBusHandler()
        self.handler.connect(self.id)
        self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)
        self.handler.add_callback("OnCollisionPresist", self.on_collision_persist)
        self.handler.add_callback("OnCollisionEnd", self.on_collision_end)

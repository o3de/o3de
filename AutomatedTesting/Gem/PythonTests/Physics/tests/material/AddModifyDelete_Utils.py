"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.components
import azlmbr.physics

class Box:
    def __init__(self, name):
        self.name = name
        self.distances = []

    def find(self):
        self.id = general.find_game_entity(self.name)
        self.start_position = self.position
        return self.id.IsValid()

    @property
    def position(self):
        return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    def is_stationary(self):
        velocity = azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetLinearVelocity", self.id)
        return velocity.IsZero()

    def push(self, impulse):
        azlmbr.physics.RigidBodyRequestBus(bus.Event, "ApplyLinearImpulse", self.id, impulse)
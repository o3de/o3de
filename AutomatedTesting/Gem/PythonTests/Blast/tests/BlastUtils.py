"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr
from enum import Enum

class Constants():
    DAMAGE_MIN_RADIUS = 1.0
    DAMAGE_MAX_RADIUS = 1.0
    DAMAGE_AMOUNT = 10.0  # Enough to break a `brittle` object


class CollisionHandler:
    def __init__(self, id, target_id):
        self.id = id
        self.target_id = target_id
        self.collision_happened = False
        self.create_collision_handler()

    def on_collision_begin(self, args):
        if args[0].Equal(self.target_id):
            self.collision_happened = True

    def create_collision_handler(self):
        self.handler = azlmbr.physics.CollisionNotificationBusHandler()
        self.handler.connect(self.id)
        self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)


class BlastNotificationHandler:
    def __init__(self, id):
        self.id = id
        self.actors_destroyed = 0
        self.actors_created = 0
        self.create_collision_handler()

    def on_actor_created(self, args):
        self.actors_created += 1

    def on_actor_destroyed(self, args):
        self.actors_destroyed += 1

    def create_collision_handler(self):
        self.handler = azlmbr.destruction.BlastFamilyComponentNotificationBusHandler()
        self.handler.connect(self.id)
        self.handler.add_callback("On Actor Created", self.on_actor_created)
        self.handler.add_callback("On Actor Destroyed", self.on_actor_destroyed)
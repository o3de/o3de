"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# does some work
# 
import sys
import os
import azlmbr
import azlmbrtest

def print_entity_id(entityId):
    print ('entity_id {} {}'.format(entityId.id, entityId.isValid()))

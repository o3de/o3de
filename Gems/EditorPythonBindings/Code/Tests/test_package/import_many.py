"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# testing mulitple test modules with azlmbr
# 
import azlmbr
import azlmbrtest
import do_work

def test_many_entity_id():
    do_work.print_entity_id(azlmbrtest.EntityId(101))

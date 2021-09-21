"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def Blast_ActorSplitsAfterCapsuleDamage():
    from BaseDamageTest import base_run as internal_run
    from BlastUtils import Constants

    def CapsuleDamage(target_id, position0):
        position1 = azlmbr.object.construct('Vector3', position0.x + 1.0, position0.y, position0.z)
        azlmbr.destruction.BlastFamilyDamageRequestBus(azlmbr.bus.Event, "Capsule Damage", target_id,
                                                       position0, position1,
                                                       Constants.DAMAGE_MIN_RADIUS,
                                                       Constants.DAMAGE_MAX_RADIUS, Constants.DAMAGE_AMOUNT)

    internal_run(CapsuleDamage)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Blast_ActorSplitsAfterCapsuleDamage)

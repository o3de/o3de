"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def Blast_ActorSplitsAfterTriangleDamage():
    from BaseDamageTest import base_run as internal_run
    from BlastUtils import Constants

    def TriangleDamage(target_id, position):
        # Some points that form a triangle that contains the given position
        position0 = azlmbr.object.construct('Vector3', position.x, position.y + 1.0, position.z)
        position1 = azlmbr.object.construct('Vector3', position.x + 1.0, position.y - 1.0, position.z)
        position2 = azlmbr.object.construct('Vector3', position.x - 1.0, position.y - 1.0, position.z)
        azlmbr.destruction.BlastFamilyDamageRequestBus(azlmbr.bus.Event, "Triangle Damage", target_id,
                                                       position0, position1, position2,
                                                       Constants.DAMAGE_AMOUNT)

    internal_run(TriangleDamage)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Blast_ActorSplitsAfterTriangleDamage)
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def Blast_ActorSplitsAfterStressDamage():
    from BaseDamageTest import base_run as internal_run
    from BlastUtils import Constants

    def StressDamage(target_id, position):
        force = azlmbr.object.construct('Vector3', 0.0, 0.0, -100.0)  # Should be enough to break `brittle` objects
        azlmbr.destruction.BlastFamilyDamageRequestBus(azlmbr.bus.Event, "Stress Damage", target_id,
                                                       position, force)

    internal_run(StressDamage)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Blast_ActorSplitsAfterStressDamage)

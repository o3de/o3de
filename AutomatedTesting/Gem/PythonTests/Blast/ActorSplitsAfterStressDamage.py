"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from ActorSplitsAfterDamage import Tests

def ActorSplitsAfterStressDamage():
    from ActorSplitsAfterDamage import base_run as internal_run
    from BlastUtils import Constants

    def StressDamage(target_id, position):
        force = azlmbr.object.construct('Vector3', 0.0, 0.0, -100.0)  # Should be enough to break `brittle` objects
        azlmbr.destruction.BlastFamilyDamageRequestBus(azlmbr.bus.Event, "Stress Damage", target_id,
                                                       position, force)

    internal_run(StressDamage)


if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(ActorSplitsAfterStressDamage)

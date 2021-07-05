"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from ActorSplitsAfterDamage import Tests

def ActorSplitsAfterImpactSpreadDamage():
    from ActorSplitsAfterDamage import base_run as internal_run
    from BlastUtils import Constants

    def ImpactSpreadDamage(target_id, position):
        azlmbr.destruction.BlastFamilyDamageRequestBus(azlmbr.bus.Event, "Impact Spread Damage", target_id,
                                                       position, Constants.DAMAGE_MIN_RADIUS,
                                                       Constants.DAMAGE_MAX_RADIUS, Constants.DAMAGE_AMOUNT)

    internal_run(ImpactSpreadDamage)


if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(ActorSplitsAfterImpactSpreadDamage)

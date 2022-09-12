"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.bus as bus
import azlmbr.atomtools as atomtools


def set_pane_visibility(pane_name, value):
    atomtools.AtomToolsMainWindowRequestBus(bus.Broadcast, "SetDockWidgetVisible", pane_name, value)


def is_pane_visible(pane_name):
    """
    :return: bool
    """
    return atomtools.AtomToolsMainWindowRequestBus(bus.Broadcast, "IsDockWidgetVisible", pane_name)

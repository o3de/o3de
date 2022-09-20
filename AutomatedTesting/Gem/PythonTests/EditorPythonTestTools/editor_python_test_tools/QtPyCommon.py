"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Base class for QtPy classes. Contains commonly used constants and generic behavior used on multiple QtPy classes
"""

import pyside_utils
from PySide2 import QtWidgets, QtTest, QtCore


class QtPyCommon:
    """
    """
    def __init__(self):
        self.var = None


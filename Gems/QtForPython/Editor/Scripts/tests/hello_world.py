"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# a simple test to make sure PySide2 widgets can be used
from PySide2 import QtWidgets
hello = QtWidgets.QPushButton("Hello world!")
hello.resize(200, 60)
hello.show()

# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""retreive the O3DE python path"""
import sys
from pathlib import Path
py_exe = Path(sys.executable)
py_dir = py_exe.parents[0]
print(py_dir.resolve())
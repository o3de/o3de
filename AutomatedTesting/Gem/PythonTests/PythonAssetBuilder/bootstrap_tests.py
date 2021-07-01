"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import sys
try:
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import mock_asset_builder
except:
    print ('skipping asset builder testing via mock_asset_builder')

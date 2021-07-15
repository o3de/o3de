"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Stores LyTT variables configured via pytest CLI plugin, for later access without dependency on fixtures
# these values should only be modified during initialization, or patched during unit tests
build_directory = None
output_path = None

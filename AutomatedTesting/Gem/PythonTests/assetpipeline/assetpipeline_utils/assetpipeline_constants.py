"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from re import compile
from typing import Pattern

"""
General Constants
"""

TIMEOUT_5: int = 5

"""
Asset Cache Server Constants
"""

ASSET_CACHE_INVALID_SERVER_ADDRESS: str = \
    '--regset="/O3DE/AssetProcessor/Settings/Server/cacheServerAddress=InvalidAddress"'
INVALID_SERVER_ADDRESS: str = "InvalidAddress"
ASSET_CACHE_SERVER_MODE: str = '--regset="/O3DE/AssetProcessor/Settings/Server/assetCacheServerMode=Server"'
ASSET_CACHE_CLIENT_MODE: str = '--regset="/O3DE/AssetProcessor/Settings/Server/assetCacheServerMode=Client"'

"""
Scene Test Constants
"""

DECIMAL_DIGITS_TO_PRESERVE: int = 3
FLOATING_POINT_REGEX: Pattern[str] = compile(f"(.*?[0-9]+\\.[0-9]{{{DECIMAL_DIGITS_TO_PRESERVE}}})[0-9]+(.*)")
XML_NUMBER_REGEX: Pattern[str] = \
    compile('.*<Class name="AZ::u64" field="m_data" value="([0-9]*)" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>')
HASH_REGEX: Pattern[str] = compile("(.*: )([0-9]*)")

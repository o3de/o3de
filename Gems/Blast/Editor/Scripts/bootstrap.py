"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
try:
    import azlmbr.asset
    import azlmbr.asset.entity
    import azlmbr.asset.builder
    import blast_asset_builder
except:
    # this script only runs in an asset processing environment
    # like the AssetProcessor or an AssetBuilder
    # plus the Blast gem needs to be enabled for the project
    pass

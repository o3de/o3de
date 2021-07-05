/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace AzFramework
{
    const char* SimpleAssetTypeGetName(const AZ::Data::AssetType& nameUuid)
    {
        char varName[SimpleAssetReferenceBase::kMaxVariableNameLength] = { 0 };
        azsnprintf(varName, SimpleAssetReferenceBase::kMaxVariableNameLength, "assetname%s", nameUuid.ToString<AZStd::string>().c_str());
        auto variable = AZ::Environment::FindVariable<AssetInfoString>(varName);
        if (variable)
        {
            return (*variable).c_str();
        }

        return "";
    }

    const char* SimpleAssetTypeGetFileFilter(const AZ::Data::AssetType& nameUuid)
    {
        char varName[SimpleAssetReferenceBase::kMaxVariableNameLength] = { 0 };
        azsnprintf(varName, SimpleAssetReferenceBase::kMaxVariableNameLength, "assetfilter%s", nameUuid.ToString<AZStd::string>().c_str());
        auto variable = AZ::Environment::FindVariable<AssetInfoString>(varName);
        if (variable)
        {
            return (*variable).c_str();
        }

        return "";
    }
} // namespace AzFramework

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

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

#include "LmbrCentral_precompiled.h"
#include "DbaAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType dbaAssetType("{511562BE-65A5-4538-A5F1-AC685366243E}");

    DbaAssetTypeInfo::~DbaAssetTypeInfo()
    {
        Unregister();
    }

    void DbaAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(dbaAssetType);
    }

    void DbaAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(dbaAssetType);
    }

    AZ::Data::AssetType DbaAssetTypeInfo::GetAssetType() const
    {
        return dbaAssetType;
    }

    const char* DbaAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Animation Database";
    }

    const char* DbaAssetTypeInfo::GetGroup() const
    {
        return "Animation";
    }
} // namespace LmbrCentral

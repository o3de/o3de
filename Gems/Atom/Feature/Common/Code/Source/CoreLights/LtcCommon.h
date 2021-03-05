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

#pragma once

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ::Render
{
    class ILtcCommon
    {
    public:
        AZ_RTTI(ILtcCommon, "{62AB02D4-A26A-4290-9302-96859E9F46CE}");
        virtual void LoadMatricesForSrg(Data::Instance<RPI::ShaderResourceGroup> srg) = 0;

        virtual ~ILtcCommon() = default;
    };

    // Class that handles basic setup for light types that use linearly transformed cosines.
    class LtcCommon
        : public ILtcCommon
    {
    public:
        AZ_RTTI(AZ::Render::LtcCommon, "{7BB6BD58-A517-4D68-87EC-CAA2AADAAC02}", AZ::Render::ILtcCommon);

        LtcCommon();
        ~LtcCommon();

        // ILtcCommon overides...
        void LoadMatricesForSrg(Data::Instance<RPI::ShaderResourceGroup> srg) override;

    private:
        AZStd::map<AZ::Uuid, AZStd::vector<RPI::AssetUtils::AsyncAssetLoader>> m_assetLoaders;

    };
}


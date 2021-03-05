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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

class CShader;

namespace Terrain
{
    class TerrainDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual float GetHeightSynchronous(float x, float y) = 0;
        virtual AZ::Vector3 GetNormalSynchronous(float x, float y) = 0;

        virtual CShader* GetTerrainHeightGeneratorShader() const = 0;
        virtual CShader* GetTerrainMaterialCompositingShader() const = 0;
    };
    using TerrainDataRequestBus = AZ::EBus<TerrainDataRequests>;

    class TerrainShaderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual void RefreshShader(const AZStd::string_view name, CShader* shader) = 0;
        virtual void ReleaseShader(CShader* shader) const = 0;
    };
    using TerrainShaderRequestBus = AZ::EBus<TerrainShaderRequests>;
}

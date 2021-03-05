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

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/EBus/EBus.h>

#include "HeightmapDataBus.h"

class CShader;

namespace Terrain
{
    // This interface defines how the renderer can access the terrain system to set up state and gather information before rendering height maps
    class TerrainProviderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // world properties
        virtual AZ::Vector3 GetWorldSize() = 0;
        virtual AZ::Vector3 GetRegionSize() = 0;
        virtual AZ::Vector3 GetWorldOrigin() = 0;
        virtual AZ::Vector2 GetHeightRange() = 0;

        // utility
        virtual void GetRegionIndex(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, int& regionIndexX, int& regionIndexY) = 0;

        virtual float GetHeightAtIndexedPosition([[maybe_unused]] int ix, [[maybe_unused]] int iy) { return 64.0f; }
        virtual float GetHeightAtWorldPosition([[maybe_unused]] float fx, [[maybe_unused]] float fy) { return 64.0f; }
        virtual unsigned char GetSurfaceTypeAtIndexedPosition([[maybe_unused]] int ix, [[maybe_unused]] int iy) { return 0; }
    };
    using TerrainProviderRequestBus = AZ::EBus<TerrainProviderRequests>;

    // This class exists for the terrain system to inject data into the renderer for generating the GPU-side terrain height map
    struct CRETerrainContext
    {
        // Tract map
        virtual void OnTractVersionUpdate() = 0;

        CShader* m_currentShader = nullptr;
    };

    class TerrainProviderNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // interface to be implemented by the game, invoked by the terrain render element

        // pull settings from the world cache, so the next accessors are accurate
        virtual void SynchronizeSettings(CRETerrainContext* context) = 0;
    };
    using TerrainProviderNotificationBus = AZ::EBus<TerrainProviderNotifications>;
}

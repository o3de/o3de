/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace Vegetation
{
    using InstanceId = AZ::u64;
    static const InstanceId MaxInstanceId = std::numeric_limits<InstanceId>::max() - 1;
    static const InstanceId InvalidInstanceId = std::numeric_limits<InstanceId>::max();

    using InstancePtr = void*;

    struct InstanceData;

    /**
    * Base class for anything that can be spawned by the Vegetation system.
    */
    class InstanceSpawner
    {
    public:
        AZ_RTTI(InstanceSpawner, "{01AD0758-B04A-4B43-BC2B-BDCD77F4EF6A}");
        AZ_CLASS_ALLOCATOR(InstanceSpawner, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        InstanceSpawner() = default;
        virtual ~InstanceSpawner() = default;

        //! Start loading any assets that the spawner will need.
        virtual void LoadAssets() = 0;

        //! Unload any assets that the spawner loaded.
        virtual void UnloadAssets() = 0;

        //! Perform any extra initialization needed at the point of registering with the vegetation system.
        virtual void OnRegisterUniqueDescriptor() = 0;

        //! Perform any extra cleanup needed at the point of unregistering with the vegetation system.
        virtual void OnReleaseUniqueDescriptor() = 0;

        //! Does this exist but have empty asset references?
        virtual bool HasEmptyAssetReferences() const = 0;

        //! Has this finished loading any assets that are needed?
        virtual bool IsLoaded() const = 0;

        //! Are the assets loaded, initialized, and spawnable?
        virtual bool IsSpawnable() const = 0;

        //! Does this spawner have the capability to provide radius data?
        virtual bool HasRadiusData() const { return false; }

        //! Radius of the instances that will be spawned, used by the Distance Between filter.
        virtual float GetRadius() const { return 0.0f; }

        //! Display name of the instances that will be spawned.
        virtual AZStd::string GetName() const = 0;

        //! Create a single instance.
        virtual InstancePtr CreateInstance(const InstanceData& instanceData) = 0;

        //! Destroy a single instance.
        virtual void DestroyInstance(InstanceId id, InstancePtr instance) = 0;

        //! Check for data equivalency.  Subclasses are expected to implement this.
        bool operator==(const InstanceSpawner& rhs) const { return DataIsEquivalent(rhs); };

    protected:

        //! Subclasses are expected to provide a comparison for data equivalency.
        virtual bool DataIsEquivalent(const InstanceSpawner& rhs) const = 0;

        //! Subclasses are expected to call this whenever assets have loaded / reloaded
        void NotifyOnAssetsLoaded()
        {
            // Note that because InstanceSpawners can be shared between Descriptors, this might actually notify
            // listeners for multiple Descriptors.
            DescriptorNotificationBus::Event(this, &DescriptorNotificationBus::Events::OnDescriptorAssetsLoaded);
        }

        //! Subclasses are expected to call this whenever assets have unloaded
        void NotifyOnAssetsUnloaded()
        {
            // Note that because InstanceSpawners can be shared between Descriptors, this might actually notify
            // listeners for multiple Descriptors.
            DescriptorNotificationBus::Event(this, &DescriptorNotificationBus::Events::OnDescriptorAssetsUnloaded);
        }
    };

} // namespace Vegetation

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTIMacros.h>


namespace TestAssetBuilder
{
    // An asset that has just an int as its data and is referenced by other assets.
    constexpr const char* const s_TestGenericAssetTypeId = "{B0CAA8CE-EEE0-4EEE-BAD5-AC88B2C8F3B8}";
    class TestGenericAssetRef : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(TestGenericAssetRef, s_TestGenericAssetTypeId, AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(TestGenericAssetRef, AZ::SystemAllocator);

        TestGenericAssetRef() = default;
        virtual ~TestGenericAssetRef() = default;

        int m_someField = 0;
    };

    // A generic asset that references other assets.
    class TestGenericAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(TestGenericAsset, "{63B3EA1F-8AFC-4C40-8F59-0B2E6A9CA191}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(TestGenericAsset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        TestGenericAsset() = default;
        virtual ~TestGenericAsset() = default;

        AZ::Data::Asset<TestGenericAssetRef> m_referencedAsset_Preload{ AZ::Data::AssetLoadBehavior::PreLoad };
        AZ::Data::Asset<TestGenericAssetRef> m_referencedAsset_NoLoad{ AZ::Data::AssetLoadBehavior::NoLoad };
        AZ::Data::AssetId m_referencedAssetId; // an assetId that is used instead of an asset ref.
    };

    //! TestGenericAssetBuilderComponent handles the lifecycle of the generic asset registration.
    //! note that this is NOT a builder, but a system component whos ONLY job is to register the
    //! asset type.
    class TestGenericAssetBuilderComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestGenericAssetBuilderComponent, "{C0B9177F-3701-445A-BEDE-3415738F0EA1}");

        TestGenericAssetBuilderComponent() = default;
        ~TestGenericAssetBuilderComponent() override = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    private:
    };

    
} // namespace TestAssetBuilder

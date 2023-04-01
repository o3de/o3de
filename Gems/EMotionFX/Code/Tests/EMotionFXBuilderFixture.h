/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <Tests/SystemComponentFixture.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>

namespace EMotionFX
{
    class EMotionFXTest_MockCatalog
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EMotionFXTest_MockCatalog, AZ::SystemAllocator);

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        MOCK_METHOD1(GetAssetInfoById, AZ::Data::AssetInfo(const AZ::Data::AssetId& id));
        //////////////////////////////////////////////////////////////////////////

        MOCK_METHOD2(GetStreamInfoForLoad, AZ::Data::AssetStreamInfo(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type));
    };

    class BuilderMockComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderMockComponent, "{6FD64C20-B0D1-41EC-9563-7B57C019300C}");

        BuilderMockComponent() = default;
        ~BuilderMockComponent() override = default;
        BuilderMockComponent(const BuilderMockComponent&) = delete;
        BuilderMockComponent(BuilderMockComponent&&) = default;
        BuilderMockComponent& operator=(const BuilderMockComponent&) = delete;
        BuilderMockComponent& operator=(BuilderMockComponent&&) = default;

        static void ReflectAnimGraphAndMotionSet(AZ::ReflectContext* context);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler> > m_assetHandlers;
        AZStd::unique_ptr<EMotionFXTest_MockCatalog> m_assetCatalog;
    };

    using EMotionFXBuilderFixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        BuilderMockComponent
    >;
}

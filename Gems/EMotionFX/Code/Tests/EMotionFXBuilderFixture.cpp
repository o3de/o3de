/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFXBuilderFixture.h"

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

using ::testing::Return;
using ::testing::_;

namespace EMotionFX
{
    void BuilderMockComponent::Activate()
    {
        ASSERT_TRUE(MCore::Initializer::Init());
        ASSERT_TRUE(EMotionFX::Initializer::Init());

        // Initialize asset handlers.
        m_assetHandlers.emplace_back(aznew EMotionFX::Integration::MotionSetAssetBuilderHandler);
        m_assetHandlers.emplace_back(aznew EMotionFX::Integration::AnimGraphAssetBuilderHandler);

        // Initialize an AssetCatalog AssetStreamInfo that will appear valid.
        AZ::Data::AssetStreamInfo mockAssetStreamInfo;
        mockAssetStreamInfo.m_streamFlags = AZ::IO::OpenMode::ModeRead;
        mockAssetStreamInfo.m_streamName = "test";

        m_assetCatalog.reset(aznew EMotionFXTest_MockCatalog());
        EXPECT_CALL(*(m_assetCatalog.get()), GetAssetInfoById(_)).WillRepeatedly(Return(AZ::Data::AssetInfo()));
        EXPECT_CALL(*(m_assetCatalog.get()), GetStreamInfoForLoad(_, _)).WillRepeatedly(Return(mockAssetStreamInfo));

        AZ::Data::AssetManager::Instance().RegisterCatalog(m_assetCatalog.get(), azrtti_typeid<EMotionFX::Integration::MotionSetAsset>());
        AZ::Data::AssetManager::Instance().RegisterCatalog(m_assetCatalog.get(), azrtti_typeid<EMotionFX::Integration::AnimGraphAsset>());
    }

    void BuilderMockComponent::Deactivate()
    {
        m_assetCatalog->DisableCatalog();
        m_assetCatalog.reset();
        m_assetHandlers.clear();

        EMotionFX::Initializer::Shutdown();
        MCore::Initializer::Shutdown();
    }

    void BuilderMockComponent::ReflectAnimGraphAndMotionSet(AZ::ReflectContext* context)
    {
        // Motion set
        EMotionFX::MotionSet::Reflect(context);
        EMotionFX::MotionSet::MotionEntry::Reflect(context);

        // Base AnimGraph objects
        EMotionFX::AnimGraphObject::Reflect(context);
        EMotionFX::AnimGraph::Reflect(context);
        EMotionFX::AnimGraphNodeGroup::Reflect(context);

        // Anim graph objects
        EMotionFX::AnimGraphObjectFactory::ReflectTypes(context);

        // Anim graph's parameters
        EMotionFX::ParameterFactory::ReflectParameterTypes(context);
    }

    void BuilderMockComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectAnimGraphAndMotionSet(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BuilderMockComponent, AZ::Component>()
                ->Version(1);
        }
    }
}

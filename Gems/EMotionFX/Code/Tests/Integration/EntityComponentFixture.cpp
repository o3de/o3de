/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <Tests/Integration/EntityComponentFixture.h>

namespace EMotionFX
{
    void EntityComponentFixture::SetUp()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        SystemComponentFixture::SetUp();

        m_app.RegisterComponentDescriptor(Integration::ActorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::AnimGraphComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::SimpleMotionComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());
    }

    void EntityComponentFixture::TearDown()
    {
        SystemComponentFixture::TearDown();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }
} // namespace EMotionFX

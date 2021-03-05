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

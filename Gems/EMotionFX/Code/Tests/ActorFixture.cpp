/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActorFixture.h"
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>

#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    void ActorFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        m_actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        m_actorInstance = ActorInstance::Create(m_actor.get());
    }

    void ActorFixture::TearDown()
    {
        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
            m_actorInstance = nullptr;
        }

        SystemComponentFixture::TearDown();
    }

    AZStd::string ActorFixture::SerializePhysicsSetup(const Actor* actor) const
    {
        if (!actor)
        {
            return AZStd::string();
        }

        return MCore::ReflectionSerializer::Serialize(actor->GetPhysicsSetup().get()).GetValue();
    }

    AZStd::string ActorFixture::SerializeSimulatedObjectSetup(const Actor* actor) const
    {
        if (!actor)
        {
            return AZStd::string();
        }

        return MCore::ReflectionSerializer::Serialize(actor->GetSimulatedObjectSetup().get()).GetValue();
    }


    SimulatedObjectSetup* ActorFixture::DeserializeSimulatedObjectSetup(const AZStd::string& data) const
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        AZ::ObjectStream::FilterDescriptor loadFilter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        SimulatedObjectSetup* setup = AZ::Utils::LoadObjectFromBuffer<EMotionFX::SimulatedObjectSetup>(data.data(), data.size(), serializeContext, loadFilter);
        setup->InitAfterLoad(m_actor.get());
        return setup;
    }


    AZStd::vector<AZStd::string> ActorFixture::GetTestJointNames() const
    {
         return { "Bip01__pelvis", "l_upLeg", "l_loLeg", "l_ankle" };
    }
} // namespace EMotionFX

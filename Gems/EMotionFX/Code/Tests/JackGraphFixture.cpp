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

#include "JackGraphFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/ActorFactory.h>


namespace EMotionFX
{
    void JackGraphFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        m_actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        OnPostActorCreated();

        m_actorInstance = ActorInstance::Create(m_actor.get());

        m_motionSet = aznew MotionSet("motionSet");

        ConstructGraph();
        m_animGraph->InitAfterLoading();

        m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance, m_motionSet);
        m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
        m_animGraphInstance->IncreaseReferenceCount(); // Two owners now, the test and the actor instance
        m_animGraphInstance->RecursiveInvalidateUniqueDatas();
    }

    void JackGraphFixture::ConstructGraph()
    {
        m_animGraph = AnimGraphFactory::Create<EmptyAnimGraph>();
    }

    void JackGraphFixture::TearDown()
    {
        if (m_animGraphInstance)
        {
            m_animGraphInstance->Destroy();
            m_animGraphInstance = nullptr;
        }

        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
            m_actorInstance = nullptr;
        }

        delete m_motionSet;
        m_motionSet = nullptr;

        m_animGraph.reset();

        SystemComponentFixture::TearDown();
    }

    void JackGraphFixture::Evaluate(float timeDelta)
    {
        GetEMotionFX().Update(timeDelta);
    }

    void JackGraphFixture::AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name)
    {
        Parameter* parameter = ParameterFactory::Create(typeId);
        parameter->SetName(name);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();
    }
} // namespace EMotionFX

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Framework/ScriptCanvasTestVerify.h>

#include "ScriptCanvasTestingSystemComponent.h"

#include <Nodes/BehaviorContextObjectTestNode.h>

namespace ScriptCanvasTesting
{
    void ScriptCanvasTestingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptCanvasTestingSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ScriptCanvasTestingSystemComponent>("ScriptCanvasTesting", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        ScriptCanvasTestingNodes::BehaviorContextObjectTest::Reflect(context);
    }

    void ScriptCanvasTestingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasTestingService", 0xd2b424fc));
    }

    void ScriptCanvasTestingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ScriptCanvasTestingService", 0xd2b424fc));
    }

    void ScriptCanvasTestingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ScriptCanvasTestingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ScriptCanvasTestingSystemComponent::Init()
    {
    }

    void ScriptCanvasTestingSystemComponent::Activate()
    {
        ScriptCanvasEditor::UnitTestVerificationBus::Handler::BusConnect();
    }

    void ScriptCanvasTestingSystemComponent::Deactivate()
    {
        ScriptCanvasEditor::UnitTestVerificationBus::Handler::BusDisconnect();
    }

    ScriptCanvasEditor::UnitTestResult ScriptCanvasTestingSystemComponent::Verify(ScriptCanvasEditor::Reporter reporter)
    {
        return ScriptCanvasTests::VerifyReporterEditor(reporter);
    }
}

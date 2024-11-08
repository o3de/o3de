/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "ScriptCanvasTestingSystemComponent.h"
#include "ScriptCanvasTestBus.h"
#include "Framework/ScriptCanvasTestVerify.h"
#include "Nodes/BehaviorContextObjectTestNode.h"

#include "Nodes/Nodeables/SharedDataSlotExample.h"
#include "Nodes/Nodeables/ValuePointerReferenceExample.h"

#include "Nodes/TestAutoGenFunctions.h"

namespace ScriptCanvasTestingNodes
{
    void BehaviorContextObjectTest::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BehaviorContextObjectTest>()
                ->Version(0)
                ->Field("String", &BehaviorContextObjectTest::m_string)
                ->Field("Name", &BehaviorContextObjectTest::m_name)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BehaviorContextObjectTest>("Behavior Context Object Test", "An Object that lives within Behavior Context exclusively for testing")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Tests/Behavior Context")
                    ->Attribute(AZ::Edit::Attributes::CategoryStyle, ".method")
                    ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TestingNodeTitlePalette")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorContextObjectTest::m_string, "String", "")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BehaviorContextObjectTest>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Tests/Behavior Context")
                ->Method("SetString", &BehaviorContextObjectTest::SetString)
                ->Method("GetString", &BehaviorContextObjectTest::GetString)
                ->Property("Name", BehaviorValueProperty(&BehaviorContextObjectTest::m_name))
                ->Constant("Always24", BehaviorConstant(24))
                ;
        }
    }
}

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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        ScriptCanvasTestingNodes::BehaviorContextObjectTest::Reflect(context);
        ScriptCanvasTesting::Reflect(context);
    }

    void ScriptCanvasTestingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasTestingService"));
    }

    void ScriptCanvasTestingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ScriptCanvasTestingService"));
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

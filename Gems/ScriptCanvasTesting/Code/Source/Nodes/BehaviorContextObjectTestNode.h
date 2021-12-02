/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Attributes.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvasTestingNodes
{
    //! This object is used to test the use of BehaviorContext classes
    class BehaviorContextObjectTest
    {
    public:
        AZ_RTTI(BehaviorContextObjectTest, "{FF687BD7-42AA-44C4-B3AB-79353E8C6CCF}");
        AZ_CLASS_ALLOCATOR(BehaviorContextObjectTest, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context)
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
                    ->Attribute(AZ::Script::Attributes::Category, "Tests/Behavior Context")
                    ->Method("SetString", &BehaviorContextObjectTest::SetString)
                    ->Method("GetString", &BehaviorContextObjectTest::GetString)
                    ->Property("Name", BehaviorValueProperty(&BehaviorContextObjectTest::m_name))
                    ->Constant("Always24", BehaviorConstant(24))
                    ;
            }
        }

        BehaviorContextObjectTest()
        {
        }

        virtual ~BehaviorContextObjectTest() {}

        void SetString(AZStd::string string)
        {
            m_string = string;
        }

        AZStd::string GetString() const
        {
            return m_string;
        }

    private:
        AZStd::string m_name;
        AZStd::string m_string;

    };
}

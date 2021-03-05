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

#pragma once

#include <ScriptCanvas/Core/Attributes.h>

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
                    ->Field("StringName", &BehaviorContextObjectTest::m_string)
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
        
        AZStd::string m_string;

    };
}
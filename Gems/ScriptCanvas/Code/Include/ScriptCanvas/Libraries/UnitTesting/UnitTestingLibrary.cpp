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

#ifndef _RELEASE

#include "UnitTestingLibrary.h"

#include <ScriptCanvas/Libraries/Libraries.h>


namespace ScriptCanvas
{
    namespace Library
    {
        void UnitTesting::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<UnitTesting, LibraryDefinition>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<UnitTesting>("Unit Testing", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/UnitTesting.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".method")->
                        Attribute(AZ::Edit::Attributes::Category, "Utilities/Unit Testing")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TestingNodeTitlePalette")
                        ;
                }
            }

            ScriptCanvas::UnitTesting::Auxiliary::StringConversion::Reflect(reflection);
            ScriptCanvas::UnitTesting::Auxiliary::EBusTraits::Reflect(reflection);
            ScriptCanvas::UnitTesting::Auxiliary::TypeExposition::Reflect(reflection);
            ScriptCanvas::UnitTesting::Auxiliary::ProduceOutcome::Reflect(reflection);
        }

        void UnitTesting::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::UnitTesting;
            AddNodeToRegistry<UnitTesting, MarkComplete>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, AddFailure>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, AddSuccess>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, Checkpoint>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectEqual>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectFalse>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectGreaterThan>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectGreaterThanEqual>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectLessThan>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectLessThanEqual>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectNotEqual>(nodeRegistry);
            AddNodeToRegistry<UnitTesting, ExpectTrue>(nodeRegistry);

            // Removing the auxiliary node since they seem to be very specific nodes that shouldn't be released to the public.
            //ScriptCanvas::UnitTesting::Auxiliary::Registrar::AddToRegistry<UnitTesting>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> UnitTesting::GetComponentDescriptors()
        {
            using namespace ScriptCanvas::Nodes::UnitTesting;
            AZStd::vector<AZ::ComponentDescriptor*> descriptors = AZStd::vector<AZ::ComponentDescriptor*>({
                MarkComplete::CreateDescriptor(),
                AddFailure::CreateDescriptor(),
                AddSuccess::CreateDescriptor(),
                Checkpoint::CreateDescriptor(),
                ExpectEqual::CreateDescriptor(),
                ExpectFalse::CreateDescriptor(),
                ExpectGreaterThan::CreateDescriptor(),
                ExpectGreaterThanEqual::CreateDescriptor(),
                ExpectLessThan::CreateDescriptor(),
                ExpectLessThanEqual::CreateDescriptor(),
                ExpectNotEqual::CreateDescriptor(),
                ExpectTrue::CreateDescriptor(),
            });

            ScriptCanvas::UnitTesting::Auxiliary::Registrar::AddDescriptors(descriptors);
            return descriptors;
        }

    } // namespace Library
} // namespace ScriptCanvas


#endif
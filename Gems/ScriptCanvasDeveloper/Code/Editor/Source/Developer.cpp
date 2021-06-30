/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"
#include <ScriptCanvasDeveloperEditor/Developer.h>
#include <ScriptCanvasDeveloperEditor/Mock.h>
#include <ScriptCanvasDeveloperEditor/WrapperMock.h>
#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvasDeveloper
{
    namespace Libraries
    {
        void Developer::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Developer, ScriptCanvas::Library::LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Developer>("Developer", "Library of Developer only nodes")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ScriptCanvas.png")
                        ;
                }
            }
        }

        void Developer::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
        {
            ScriptCanvas::Library::AddNodeToRegistry<Developer, Nodes::Mock>(nodeRegistry);
            ScriptCanvas::Library::AddNodeToRegistry<Developer, Nodes::WrapperMock>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Developer::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                Nodes::Mock::CreateDescriptor(),
                Nodes::WrapperMock::CreateDescriptor()
            });
        }
    }
}

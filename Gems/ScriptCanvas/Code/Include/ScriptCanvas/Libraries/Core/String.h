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

#include <ScriptCanvas/Core/NativeDatumNode.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {

            class String 
                : public NativeDatumNode<String, Data::StringType>
            {
            public:
                using ParentType = NativeDatumNode<String, Data::StringType>;
                AZ_COMPONENT(String, "{9C9B9D96-1838-4493-A14D-40C77B4DB5DD}", ParentType);

                static void Reflect(AZ::ReflectContext* context)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                    {
                        serializeContext->Class<String, PureData>()
                            ->Version(3)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<String>("String", "Holds a string value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/String.png")
                                ;
                        }
                    }
                }                

            };
        }
    }
}

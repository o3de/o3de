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
        namespace Math
        {
            class Number
                : public NativeDatumNode<Number, Data::NumberType>
            {
            public:
                using ParentType = NativeDatumNode<Number, Data::NumberType>;
                AZ_COMPONENT(Number, "{E1E746E9-2761-431E-9D06-717381F9822D}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Number, PureData>()
                            ->Version(3)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Number>("Number", "A numeric value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Number.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

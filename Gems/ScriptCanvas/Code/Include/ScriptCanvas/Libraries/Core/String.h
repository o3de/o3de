/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/String.png")
                                ;
                        }
                    }
                }                

            };
        }
    }
}

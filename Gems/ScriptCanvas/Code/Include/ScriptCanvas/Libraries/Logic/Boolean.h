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
        namespace Logic
        {
            class Boolean 
                : public NativeDatumNode<Boolean, bool>
            {
            public:
                using ParentType = NativeDatumNode<Boolean, bool>;
                AZ_COMPONENT(Boolean, "{263E8CAE-9F20-4198-A937-14761A46D996}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Boolean, ParentType>()
                            ->Version(4)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Boolean>("Boolean", "A boolean value (true/false)")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Boolean.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

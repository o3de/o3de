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
        namespace Math
        {
            class Matrix4x4
                : public NativeDatumNode<Matrix4x4, AZ::Matrix4x4>
            {
            public:
                using ParentType = NativeDatumNode<Matrix4x4, AZ::Matrix4x4>;
                AZ_COMPONENT(Matrix4x4, "{CF059648-8BE5-4CC6-B909-4D3EBD945071}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Matrix4x4, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Matrix4x4>("Matrix4x4", "A 4x4 matrix value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Matrix4x4.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

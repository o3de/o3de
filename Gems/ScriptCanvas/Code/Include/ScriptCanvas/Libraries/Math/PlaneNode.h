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
            class Plane
                : public NativeDatumNode<Plane, Data::PlaneType>
            {
            public:
                using ParentType = NativeDatumNode<Plane, Data::PlaneType>;
                AZ_COMPONENT(Plane, "{A756E230-B4DC-42E7-A160-20A803CC8FA1}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Plane, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Plane>("Plane", "A plane value according to the equation: Ax + By + Cz + D = 0")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Plane.png")
                                ;
                        }
                    }
                }
                
            };
        }
    }
}

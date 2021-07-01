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
            class Quaternion
                : public NativeDatumNode<Quaternion, Data::QuaternionType>
            {
            public:
                using ParentType = NativeDatumNode<Quaternion, Data::QuaternionType>;
                AZ_COMPONENT(Quaternion, "{E17FE11D-69F2-4746-B582-778B48D0BF47}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Quaternion, PureData>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Quaternion>("Quaternion", "imaginary(X, Y, Z), real W")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Quaternion.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

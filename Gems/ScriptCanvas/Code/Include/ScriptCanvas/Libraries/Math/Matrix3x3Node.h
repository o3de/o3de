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
#include <AzCore/Math/Matrix3x3.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Matrix3x3
                : public NativeDatumNode<Matrix3x3, AZ::Matrix3x3>
            {
            public:
                using ParentType = NativeDatumNode<Matrix3x3, AZ::Matrix3x3>;
                AZ_COMPONENT(Matrix3x3, "{9FDA1949-A74F-4D27-BBD0-8E6F165291FE}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Matrix3x3, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Matrix3x3>("Matrix3x3", "A 3x3 matrix value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Matrix3x3.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

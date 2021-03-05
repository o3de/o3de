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
            class Vector2
                : public NativeDatumNode<Vector2, AZ::Vector2>
            {
            public:
                using ParentType = NativeDatumNode<Vector2, AZ::Vector2>;
                AZ_COMPONENT(Vector2, "{EB647398-7F56-4727-9C7C-277593DB1F11}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector2, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector2>("Vector2", "A 2D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ;
                        }
                    }
                }

            };

            class Vector3
                : public NativeDatumNode<Vector3, AZ::Vector3>
            {
            public:
                using ParentType = NativeDatumNode<Vector3, AZ::Vector3>;
                AZ_COMPONENT(Vector3, "{95A12BDE-D4B4-47E8-A917-3E42F678E7FA}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector3, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector3>("Vector3", "A 3D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ;
                        }
                    }
                }

            };

            class Vector4
                : public NativeDatumNode<Vector4, AZ::Vector4>
            {
            public:
                using ParentType = NativeDatumNode<Vector4, AZ::Vector4>;
                AZ_COMPONENT(Vector4, "{9CAE50A1-C575-4DFC-95C5-FA0A12DABCBD}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector4, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector4>("Vector4", "A 4D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ;
                        }
                    }
                }

            };
        }
    }
}

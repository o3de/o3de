#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace SceneData
        {
            class StaticMeshAdvancedRule
                : public DataTypes::IMeshAdvancedRule
            {
            public:
                AZ_RTTI(StaticMeshAdvancedRule, "{AE82749D-A68A-4FE7-A8BA-0F4CE67607AC}", DataTypes::IMeshAdvancedRule);
                AZ_CLASS_ALLOCATOR_DECL

                StaticMeshAdvancedRule();
                ~StaticMeshAdvancedRule() override = default;

                void SetUse32bitVertices(bool value);
                bool Use32bitVertices() const override;

                void SetMergeMeshes(bool value);
                bool MergeMeshes() const override;

                void SetUseCustomNormals(bool value);
                bool UseCustomNormals() const override;

                void SetVertexColorStreamName(const AZStd::string& name);
                void SetVertexColorStreamName(AZStd::string&& name);
                const AZStd::string& GetVertexColorStreamName() const override;
                bool IsVertexColorStreamDisabled() const override;
                                
                static void Reflect(ReflectContext* context);
                
            protected:
                AZStd::string m_vertexColorStreamName;
                bool m_use32bitVertices;
                bool m_mergeMeshes;
                bool m_useCustomNormals;
            };
        } // SceneData
    } // SceneAPI
} // AZ

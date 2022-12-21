/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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

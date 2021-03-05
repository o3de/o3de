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
#include <AzCore/std/string/string.h>
#include <SceneAPIExt/Rules/IMeshRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class MeshRule
                : public IMeshRule
            {
            public:
                AZ_RTTI(MeshRule, "{7F115A73-28A2-4E35-8C87-1A1982773034}", IMeshRule);
                AZ_CLASS_ALLOCATOR_DECL

                MeshRule();
                ~MeshRule() override = default;

                VertexColorMode GetVertexColorMode() const override;
                const AZStd::string& GetVertexColorStreamName() const override;
                bool IsVertexColorsDisabled() const override;
                void DisableVertexColors() override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode);

                VertexColorMode m_vertexColorMode = VertexColorMode::Precision_32;
                AZStd::string m_vertexColorStreamName;

                static const char* m_disabledVertexColorsName;
            };
        } // Rule
    } // Pipeline
} // EMotionFX

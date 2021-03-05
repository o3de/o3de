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

#include "./UseTextureFunctor.h"
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace Render
    {
        void UseTextureFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<UseTextureFunctor, RPI::MaterialFunctor>()
                    ->Version(5)
                    ->Field("texturePropertyIndex", &UseTextureFunctor::m_texturePropertyIndex)
                    ->Field("useTexturePropertyIndex", &UseTextureFunctor::m_useTexturePropertyIndex)
                    ->Field("dependentPropertyIndexes", &UseTextureFunctor::m_dependentPropertyIndexes)
                    ->Field("shaderTags", &UseTextureFunctor::m_shaderTags)
                    ->Field("useTextureOptionIndices", &UseTextureFunctor::m_useTextureOptionIndices)
                    ;
            }
        }

        void UseTextureFunctor::Process(RuntimeContext& context)
        {
            using namespace RPI;

            auto texture = context.GetMaterialPropertyValue<Data::Instance<Image>>(m_texturePropertyIndex);
            bool useTextureFlag = context.GetMaterialPropertyValue<bool>(m_useTexturePropertyIndex);

            ShaderOptionValue useTexture{useTextureFlag && nullptr != texture};

            for (const auto& shaderTag : m_shaderTags)
            {
                context.SetShaderOptionValue(shaderTag, m_useTextureOptionIndices[shaderTag], useTexture);
            }
        }

        void UseTextureFunctor::Process(EditorContext& context)
        {
            const bool useTextureFlag = context.GetMaterialPropertyValue<bool>(m_useTexturePropertyIndex);
            Data::Instance<RPI::Image> image = context.GetMaterialPropertyValue<Data::Instance<RPI::Image>>(m_texturePropertyIndex);

            context.SetMaterialPropertyVisibility(
                m_useTexturePropertyIndex,
                nullptr != image ? RPI::MaterialPropertyVisibility::Enabled : RPI::MaterialPropertyVisibility::Hidden
            );

            RPI::MaterialPropertyVisibility dependentVisibility = RPI::MaterialPropertyVisibility::Enabled;
            if (nullptr == image)
            {
                dependentVisibility = RPI::MaterialPropertyVisibility::Hidden;
            }
            else if (!useTextureFlag)
            {
                dependentVisibility = RPI::MaterialPropertyVisibility::Disabled;
            }

            // These properties are enabled only when the texture is actually going to be sampled by the shader.
            for (RPI::MaterialPropertyIndex index : m_dependentPropertyIndexes)
            {

                context.SetMaterialPropertyVisibility(index, dependentVisibility);
            }
        }

    } // namespace Render
} // namespace AZ

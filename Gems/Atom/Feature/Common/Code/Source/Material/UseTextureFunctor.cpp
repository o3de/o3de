/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                    ->Field("useTextureOptionName", &UseTextureFunctor::m_useTextureOptionName)
                    ;
            }
        }

        void UseTextureFunctor::Process(RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            using namespace RPI;

            auto texture = context.GetMaterialPropertyValue<Data::Instance<Image>>(m_texturePropertyIndex);
            bool useTextureFlag = context.GetMaterialPropertyValue<bool>(m_useTexturePropertyIndex);

            ShaderOptionValue useTexture{useTextureFlag && nullptr != texture};

            context.SetShaderOptionValue(m_useTextureOptionName, useTexture);
        }

        void UseTextureFunctor::Process(RPI::MaterialFunctorAPI::EditorContext& context)
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

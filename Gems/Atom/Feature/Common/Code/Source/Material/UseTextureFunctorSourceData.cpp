/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./UseTextureFunctorSourceData.h"
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void UseTextureFunctorSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<UseTextureFunctorSourceData>()
                    ->Version(6)
                    ->Field("textureProperty", &UseTextureFunctorSourceData::m_texturePropertyName)
                    ->Field("useTextureProperty", &UseTextureFunctorSourceData::m_useTexturePropertyName)
                    ->Field("dependentProperties", &UseTextureFunctorSourceData::m_dependentProperties)
                    ->Field("shaderTags", &UseTextureFunctorSourceData::m_shaderTags)
                    ->Field("shaderOption", &UseTextureFunctorSourceData::m_useTextureOptionName)
                    ;
            }
        }

        AZStd::vector<AZ::Name> UseTextureFunctorSourceData::GetShaderOptionDependencies() const
        {
            AZStd::vector<AZ::Name> options;
            options.push_back(m_useTextureOptionName);
            return options;
        }

        RPI::MaterialFunctorSourceData::FunctorResult UseTextureFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            using namespace RPI;

            RPI::Ptr<UseTextureFunctor> functor = aznew UseTextureFunctor;

            functor->m_texturePropertyIndex = context.FindMaterialPropertyIndex(m_texturePropertyName);
            if (functor->m_texturePropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_texturePropertyIndex);

            for (const Name& name : m_dependentProperties)
            {
                MaterialPropertyIndex index = context.FindMaterialPropertyIndex(name);
                if (index.IsNull())
                {
                    return Failure();
                }
                functor->m_dependentPropertyIndexes.push_back(index);
            }

            functor->m_useTexturePropertyIndex = context.FindMaterialPropertyIndex(m_useTexturePropertyName);
            if (functor->m_useTexturePropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_useTexturePropertyIndex);

            auto attachShaderOption = [&](const AZ::Name& shaderTag, bool reportOptionNotFoundError)
            {
                RPI::ShaderOptionIndex optionIndex = context.FindShaderOptionIndex(shaderTag, m_useTextureOptionName, reportOptionNotFoundError);
                if (optionIndex.IsNull())
                {
                    return false;
                }

                functor->m_shaderTags.push_back(shaderTag);
                functor->m_useTextureOptionIndices[shaderTag] = optionIndex;

                return true;
            };

            if (m_shaderTags.empty())
            {
                bool shaderOptionFound = false;

                for (const auto& shaderTag : context.GetShaderTags())
                {
                    if (attachShaderOption(shaderTag, false))
                    {
                        shaderOptionFound = true;
                    }
                }

                if (!shaderOptionFound)
                {
                    AZ_Error("UseTextureFunctorSourceData", false, "Could not find shader option '%s' in any of the available shaders.", m_useTextureOptionName.GetCStr());
                    return Failure();
                }
            }
            else
            {
                for (const auto& shaderTag : m_shaderTags)
                {
                    if (!attachShaderOption(shaderTag, true))
                    {
                        return Failure();
                    }
                }
            }

            return Success(Ptr<MaterialFunctor>(functor));
        }

        RPI::MaterialFunctorSourceData::FunctorResult UseTextureFunctorSourceData::CreateFunctor(const EditorContext& context) const
        {
            using namespace RPI;

            RPI::Ptr<UseTextureFunctor> functor = aznew UseTextureFunctor;

            functor->m_texturePropertyIndex = context.FindMaterialPropertyIndex(m_texturePropertyName);
            if (functor->m_texturePropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_texturePropertyIndex);

            for (const Name& name : m_dependentProperties)
            {
                MaterialPropertyIndex index = context.FindMaterialPropertyIndex(name);
                if (index.IsNull())
                {
                    return Failure();
                }
                functor->m_dependentPropertyIndexes.push_back(index);
            }

            functor->m_useTexturePropertyIndex = context.FindMaterialPropertyIndex(m_useTexturePropertyName);
            if (functor->m_useTexturePropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_useTexturePropertyIndex);

            return Success(Ptr<MaterialFunctor>(functor));
        }

    } // namespace Render
} // namespace AZ

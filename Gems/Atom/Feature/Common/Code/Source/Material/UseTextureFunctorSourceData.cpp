/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./UseTextureFunctorSourceData.h"
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>

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

            functor->m_useTextureOptionName = m_useTextureOptionName;
            context.GetNameContext()->ContextualizeShaderOption(functor->m_useTextureOptionName);

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

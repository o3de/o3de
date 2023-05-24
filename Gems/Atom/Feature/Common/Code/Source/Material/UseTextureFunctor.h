/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace Render
    {
        //! Materials can use this functor to control whether a specific texture property will be sampled. 
        //! Sampling will be disabled if no texture is bound or if the useTexture flag is disabled.
        class UseTextureFunctor final
            : public RPI::MaterialFunctor
        {
            friend class UseTextureFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(UseTextureFunctor , SystemAllocator)
            AZ_RTTI(UseTextureFunctor, "{CFAC6159-840A-4696-8699-D3850D8A3930}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            void Process(RPI::MaterialFunctorAPI::EditorContext& context) override;

        private:
            // Material property inputs...
            RPI::MaterialPropertyIndex m_texturePropertyIndex;     //!< material property for a texture
            RPI::MaterialPropertyIndex m_useTexturePropertyIndex;  //!< material property for a bool that indicates whether to use the texture

            //! material properties that relate to the texture, which will be enabled only when the texture map is enabled.
            AZStd::vector<RPI::MaterialPropertyIndex> m_dependentPropertyIndexes;

            // Shader option output...
            Name m_useTextureOptionName;
        };

    } // namespace Render
} // namespace AZ

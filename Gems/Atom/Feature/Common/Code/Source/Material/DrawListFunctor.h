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

namespace AZ
{
    namespace Render
    {
        //! Materials can use this functor to overwrite the draw list for a shader.
        class DrawListFunctor final
            : public RPI::MaterialFunctor
        {
            friend class DrawListFunctorSourceData;
        public:
            AZ_RTTI(AZ::Render::DrawListFunctor, "{C8A18ADE-FFD4-43CF-9262-E38849B86BC0}", RPI::MaterialFunctor);
            AZ_CLASS_ALLOCATOR(DrawListFunctor, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RuntimeContext& context) override;

        private:
            RPI::MaterialPropertyIndex m_triggerPropertyIndex; //!< The index of the property that triggers the change to the draw list.
            RPI::MaterialPropertyValue m_triggerValue;         //!< The value of the property that triggers the change.
            uint32_t m_shaderItemIndex;                        //!< Index into the material's list of shader items.
            AZ::Name m_drawListName;                           //!< The intended draw list for the shader item indexed if the enable property is true.
        };
    } // namespace Render
} // namespace AZ

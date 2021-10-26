/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "DrawListFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>

namespace AZ
{
    namespace Render
    {
        //! Builds a DrawListFunctor.
        //! Materials can use this functor to overwrite draw list for a shader.
        class DrawListFunctorSourceData final
            : public RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(AZ::Render::DrawListFunctorSourceData, "{1DF1E75F-8C6F-4CED-8CC7-73A8C1E9E9ED}", RPI::MaterialFunctorSourceData);
            AZ_CLASS_ALLOCATOR(DrawListFunctorSourceData, AZ::SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override;

        private:
            AZStd::string m_triggerPropertyName;                          //!< The name of the property to trigger the changes to the draw list.
            RPI::MaterialPropertyValueSourceData m_triggerPropertyValue;  //!< The value of the property to trigger the changes to the draw list.

            uint32_t m_shaderItemIndex;                       //!< Index into the material's list of shader items.
            AZ::Name m_drawListName;                          //!< The intended draw list for the shader item indexed if the enable property is true.
        };

    } // namespace Render
} // namespace AZ

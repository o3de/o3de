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
#include <Atom/Feature/CoreLights/PhotometricValue.h>

namespace AZ
{
    namespace Render
    {
        //! The functor can be used to convert between different emissive light unit
        //! Only support Ev100 and Lux
        class ConvertEmissiveUnitFunctor final
            : public AZ::RPI::MaterialFunctor
        {
            friend class ConvertEmissiveUnitFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(ConvertEmissiveUnitFunctor, SystemAllocator)
            AZ_RTTI(ConvertEmissiveUnitFunctor, "{F272CFAB-FD71-4E78-AA47-D0D2E88CE30C}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            void Process(RPI::MaterialFunctorAPI::EditorContext& context) override;
            float GetProcessedValue(float originalEmissiveIntensity, uint32_t lightUnitIndex) const;

        private:

            AZ::RPI::MaterialPropertyIndex m_intensityPropertyIndex;
            AZ::RPI::MaterialPropertyIndex m_lightUnitPropertyIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputIndex;

            uint32_t m_ev100Index;
            uint32_t m_nitIndex;

            float m_ev100Min = 0.0f;
            float m_ev100Max = 0.0f;
            float m_nitMin = 0.0f;
            float m_nitMax = 0.0f;
        };
    }
}

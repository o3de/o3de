/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Math/Vector2.h>

namespace AZ
{
    namespace Render
    {
        class ConvertEmissiveUnitFunctorSourceData;

        //! Builds a ConvertEmissiveUnitFunctor
        //! Please add it into the shaderInputFunctors list in material file
        class ConvertEmissiveUnitFunctorSourceData final
            : public AZ::RPI::MaterialFunctorSourceData
        {
        public:
            AZ_CLASS_ALLOCATOR(ConvertEmissiveUnitFunctorSourceData, AZ::SystemAllocator)
            AZ_RTTI(ConvertEmissiveUnitFunctorSourceData, "{B476A346-C5E0-4DB9-BCFD-B2AFA8587D24}", AZ::RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;
            FunctorResult CreateFunctor(const EditorContext& context) const override;

        private:
            // Name of the intensity property in material
            AZStd::string m_intensityPropertyName;

            // Name of the light unit property in material
            AZStd::string m_lightUnitPropertyName;

            // Name of the srg constant input in shader
            AZStd::string m_shaderInputName;

            // The indices of photometric units in the dropdown list
            uint32_t m_ev100Index = 0;
            uint32_t m_nitIndex = 1;

            // Minimum and Maximum value for different photometric units
            AZ::Vector2 m_ev100MinMax;
            AZ::Vector2 m_nitMinMax;
        };
    }
}

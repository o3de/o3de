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

#pragma once


#include "./ConvertEmissiveUnitFunctor.h"
#include <AzCore/Math/Vector2.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>

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
            uint32_t m_ev100Index;
            uint32_t m_nitIndex;

            // Minimum and Maximum value for different photometric units
            AZ::Vector2 m_ev100MinMax;
            AZ::Vector2 m_nitMinMax;
        };
    }
}

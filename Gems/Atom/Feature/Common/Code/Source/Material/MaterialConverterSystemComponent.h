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

#include <Atom/Feature/Automation/AtomAutomationBus.h>
#include <Atom/RPI.Edit/Material/MaterialConverterBus.h>
#include <AzCore/Component/Component.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>

namespace AZ
{
    namespace Render
    {
        //! Atom's implementation of converting SceneAPI data into Atom's default material: StandardPBR
        class MaterialConverterSystemComponent final
            : public AZ::Component
            , public RPI::MaterialConverterBus::Handler
        {
        public:
            AZ_COMPONENT(MaterialConverterSystemComponent, "{C2338D45-6456-4521-B469-B000A13F2493}");

            static void Reflect(AZ::ReflectContext* context);

            void Activate() override;
            void Deactivate() override;

            // MaterialConverterBus overrides ...
            bool ConvertMaterial(const AZ::SceneAPI::DataTypes::IMaterialData& materialData, RPI::MaterialSourceData& out) override;
            const char* GetMaterialTypePath() const override;
        };
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        struct MaterialConverterSettings
        {
            AZ_TYPE_INFO(MaterialConverterSettings, "{8D91601D-570A-4557-99C8-631DB4928040}");
            
            static void Reflect(AZ::ReflectContext* context);

            bool m_enable = true;
            AZStd::string m_defaultMaterial;
            //! Sets whether to include material property names when generating material assets. If this
            //! setting is true, material property name resolution and validation is deferred into load
            //! time rather than at build time, allowing to break some dependencies (e.g. fbx files will no
            //! longer need to be dependent on materialtype files).
            bool m_includeMaterialPropertyNames = true;
        };

        //! Atom's implementation of converting SceneAPI data into Atom's default material: StandardPBR
        class MaterialConverterSystemComponent final
            : public AZ::Component
            , public RPI::MaterialConverterBus::Handler
        {
        public:
            AZ_COMPONENT(MaterialConverterSystemComponent, "{C2338D45-6456-4521-B469-B000A13F2493}");

            static void Reflect(AZ::ReflectContext* context);
            
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            void Activate() override;
            void Deactivate() override;

            // MaterialConverterBus overrides ...
            bool IsEnabled() const override;
            bool ShouldIncludeMaterialPropertyNames() const override;
            bool ConvertMaterial(const AZ::SceneAPI::DataTypes::IMaterialData& materialData, RPI::MaterialSourceData& out) override;
            AZStd::string GetMaterialTypePath() const override;
            AZStd::string GetDefaultMaterialPath() const override;

        private:
            MaterialConverterSettings m_settings;
        };
    }
}

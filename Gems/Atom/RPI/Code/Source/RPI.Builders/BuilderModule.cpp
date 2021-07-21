/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <Model/ModelExporterComponent.h>
#include <Model/ModelAssetBuilderComponent.h>
#include <Model/MaterialAssetBuilderComponent.h>
#include <BuilderComponent.h>

namespace AZ
{
    namespace RPI
    {
        /**
         * @class BuilderModule
         * @brief Exposes Atom Building components to the Asset Processor
         */
        class BuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(BuilderModule, "{CA15BD7F-01B4-4959-BEF2-81FA3AD2C834}", AZ::Module);

            BuilderModule()
            {
                m_descriptors.push_back(ModelExporterComponent::CreateDescriptor());
                m_descriptors.push_back(ModelAssetBuilderComponent::CreateDescriptor());
                m_descriptors.push_back(MaterialAssetBuilderComponent::CreateDescriptor());
                m_descriptors.push_back(MaterialAssetDependenciesComponent::CreateDescriptor());
                m_descriptors.push_back(BuilderComponent::CreateDescriptor());
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    } // namespace RPI
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RPI_Edit_Builders, AZ::RPI::BuilderModule);

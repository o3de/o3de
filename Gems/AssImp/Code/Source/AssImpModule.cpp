/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <Source/AssImpImporterComponent.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpAnimationImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBitangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBlendShapeImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBoneImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpCustomPropertyImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpSkinImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpSkinWeightsImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpTransformImporter.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpUvMapImporter.h>

namespace AZ
{
    namespace AssImp
    {
        class AssImpModule
            : public Module
        {
        public:
            AZ_RTTI(AssImpModule, "{C527442E-A29D-48E5-8D87-6B453F48BD53}", Module);

            AssImpModule()
                : Module()
            {
                m_descriptors.insert(m_descriptors.end(),
                {
                    AssImpImporterComponent::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpAnimationImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpBitangentStreamImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpBlendShapeImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpBoneImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpColorStreamImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpCustomPropertyImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpMaterialImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpMeshImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpSkinImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpSkinWeightsImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpTangentStreamImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpTransformImporter::CreateDescriptor(),
                    SceneAPI::SceneBuilder::AssImpUvMapImporter::CreateDescriptor(),
                });

                // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
                // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
                AZStd::vector<AZ::Uuid> typeIds;
                typeIds.reserve(m_descriptors.size());
                for (AZ::ComponentDescriptor* descriptor : m_descriptors)
                {
                    typeIds.emplace_back(descriptor->GetUuid());
                }
                AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(&AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, typeIds);
            }
        };
    } // namespace AssImp
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AZ::AssImp::AssImpModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AssImp_Editor, AZ::AssImp::AssImpModule)
#endif

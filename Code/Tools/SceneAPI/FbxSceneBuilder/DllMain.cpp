/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Module/Environment.h>
#include <SceneAPI/FbxSceneBuilder/FbxImportRequestHandler.h>

#include <SceneAPI/FbxSceneBuilder/FbxImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpBitangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTransformImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpUvMapImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpSkinImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpSkinWeightsImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpBoneImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpAnimationImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpBlendShapeImporter.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;

            void Reflect(AZ::SerializeContext* /*context*/)
            {
                // Descriptor registration is done in Reflect instead of Initialize because the ResourceCompilerScene initializes the libraries before
                // there's an application.
                using namespace AZ::SceneAPI;
                using namespace AZ::SceneAPI::FbxSceneBuilder;

                if (g_componentDescriptors.empty())
                {
                    // Global importer and behavior
                    g_componentDescriptors.push_back(FbxSceneBuilder::FbxImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxSceneImporter::FbxImportRequestHandler::CreateDescriptor());

                    // Node and attribute importers
                    g_componentDescriptors.push_back(AssImpBitangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpColorStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMaterialImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMeshImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpTangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpTransformImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpUvMapImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpSkinImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpSkinWeightsImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpBoneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpAnimationImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpBlendShapeImporter::CreateDescriptor());

                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
                    }
                }
            }

            void ReflectBehavior([[maybe_unused]] AZ::BehaviorContext* context)
            {
                // stub in until LYN-1284 is done
            }

            void Activate()
            {
            }
            
            void Deactivate()
            {
            }

            void Uninitialize()
            {
                if (!g_componentDescriptors.empty())
                {
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        descriptor->ReleaseDescriptor();
                    }
                    g_componentDescriptors.clear();
                    g_componentDescriptors.shrink_to_fit();
                }
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
}
extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    AZ::SceneAPI::FbxSceneBuilder::Reflect(context);
}
extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext* context)
{
    AZ::SceneAPI::FbxSceneBuilder::ReflectBehavior(context);
}
extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    AZ::SceneAPI::FbxSceneBuilder::Uninitialize();
    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)

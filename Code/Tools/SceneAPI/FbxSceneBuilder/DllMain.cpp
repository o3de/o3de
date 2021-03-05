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

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Module/Environment.h>
#include <SceneAPI/FbxSceneBuilder/FbxImportRequestHandler.h>

#include <SceneAPI/FbxSceneBuilder/FbxImporter.h>
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTransformImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpUvMapImporter.h>
#endif
#include <SceneAPI/FbxSceneBuilder/Importers/FbxAnimationImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBlendShapeImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBoneImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxTangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBitangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxMaterialImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxMeshImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxSkinImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxSkinWeightsImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxTransformImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxUvMapImporter.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            static AZ::SceneAPI::FbxSceneImporter::FbxImportRequestHandler* g_fbxImporter = nullptr;
            static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;

            void Initialize()
            {
                // Currently it's still needed to explicitly create an instance of this instead of letting
                //      it be a normal component. This is because ResourceCompilerScene needs to return
                //      the list of available extensions before it can start the application.
                if (!g_fbxImporter)
                {
                    g_fbxImporter = aznew AZ::SceneAPI::FbxSceneImporter::FbxImportRequestHandler();
                    g_fbxImporter->Activate();
                }
            }

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

                    // Node and attribute importers
                    g_componentDescriptors.push_back(FbxAnimationImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBlendShapeImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBoneImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxColorStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxMaterialImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxMeshImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxSkinImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxSkinWeightsImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxTransformImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxUvMapImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxTangentStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(FbxBitangentStreamImporter::CreateDescriptor());
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
                    g_componentDescriptors.push_back(AssImpColorStreamImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMaterialImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpMeshImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpTransformImporter::CreateDescriptor());
                    g_componentDescriptors.push_back(AssImpUvMapImporter::CreateDescriptor());
#endif
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

                if (g_fbxImporter)
                {
                    g_fbxImporter->Deactivate();
                    delete g_fbxImporter;
                    g_fbxImporter = nullptr;
                }
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
    AZ::SceneAPI::FbxSceneBuilder::Initialize();
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

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

#include <AssetProcessor/AssetBuilderSDK/AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <SceneAPI/FbxSceneBuilder/FbxImportRequestHandler.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneImporter
        {
            void SceneImporterSettings::Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext)
                {
                    serializeContext->Class<SceneImporterSettings>()
                                    ->Version(1)
                                    ->Field("SupportedFileTypeExtensions", &SceneImporterSettings::m_supportedFileTypeExtensions);
                }
            }

            void FbxImportRequestHandler::Activate()
            {
                auto settingsRegistry = AZ::SettingsRegistry::Get();
                
                if (settingsRegistry)
                {
                    settingsRegistry->GetObject(m_settings, "/O3DE/SceneAPI/AssetImporter");
                }

                BusConnect();
            }

            void FbxImportRequestHandler::Deactivate()
            {
                BusDisconnect();
            }

            void FbxImportRequestHandler::Reflect(ReflectContext* context)
            {
                SceneImporterSettings::Reflect(context);

                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxImportRequestHandler, SceneCore::BehaviorComponent>()->Version(1)->Attribute(
                        AZ::Edit::Attributes::SystemComponentTags,
                        AZStd::vector<AZ::Crc32>({AssetBuilderSDK::ComponentTags::AssetBuilder}));
                    
                }
            }

            void FbxImportRequestHandler::GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions)
            {
                extensions.insert(m_settings.m_supportedFileTypeExtensions.begin(), m_settings.m_supportedFileTypeExtensions.end());
            }

            Events::LoadingResult FbxImportRequestHandler::LoadAsset(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid, [[maybe_unused]] RequestingApplication requester)
            {
                AZStd::string extension;
                StringFunc::Path::GetExtension(path.c_str(), extension);
                
                if (!m_settings.m_supportedFileTypeExtensions.contains(extension))
                {
                    return Events::LoadingResult::Ignored;
                }

                scene.SetSource(path, guid);

                // Push contexts
                Events::ProcessingResultCombiner contextResult;
                contextResult += Events::Process<Events::PreImportEventContext>(path);
                contextResult += Events::Process<Events::ImportEventContext>(path, scene);
                contextResult += Events::Process<Events::PostImportEventContext>(scene);

                if (contextResult.GetResult() == Events::ProcessingResult::Success)
                {
                    return Events::LoadingResult::AssetLoaded;
                }
                else
                {
                    return Events::LoadingResult::AssetFailure;
                }
            }

            void FbxImportRequestHandler::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.emplace_back(AZ_CRC_CE("AssetImportRequestHandler"));
            }
        } // namespace Import
    } // namespace SceneAPI
} // namespace AZ

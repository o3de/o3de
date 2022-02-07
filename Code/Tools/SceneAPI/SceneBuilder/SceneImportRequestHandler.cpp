/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetProcessor/AssetBuilderSDK/AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <SceneAPI/SceneBuilder/SceneImportRequestHandler.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        void SceneImporterSettings::Reflect(AZ::ReflectContext* context)
        {
            using namespace AzToolsFramework::AssetUtils;
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext)
            {
                serializeContext->Class<SceneImporterSettings>()
                                ->Version(2)
                                ->Field(AssetImporterSupportedFileTypeKey, &SceneImporterSettings::m_supportedFileTypeExtensions);
            }
        }

        void SceneImportRequestHandler::Activate()
        {
            using namespace AzToolsFramework::AssetUtils;
            if (auto* settingsRegistry = AZ::SettingsRegistry::Get())
            {
                settingsRegistry->GetObject(m_settings, AssetImporterSettingsKey);
            }

            BusConnect();
        }

        void SceneImportRequestHandler::Deactivate()
        {
            BusDisconnect();
        }

        void SceneImportRequestHandler::Reflect(ReflectContext* context)
        {
            SceneImporterSettings::Reflect(context);

            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SceneImportRequestHandler, AZ::Component>()->Version(1)->Attribute(
                    AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>(
                        {AssetBuilderSDK::ComponentTags::AssetBuilder,
                        AssetImportRequest::GetAssetImportRequestComponentTag()}));
                    
            }
        }

        void SceneImportRequestHandler::GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions)
        {
            extensions.insert(m_settings.m_supportedFileTypeExtensions.begin(), m_settings.m_supportedFileTypeExtensions.end());
        }

        Events::LoadingResult SceneImportRequestHandler::LoadAsset(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid, [[maybe_unused]] RequestingApplication requester)
        {
            AZStd::string extension;
            StringFunc::Path::GetExtension(path.c_str(), extension);
            AZStd::to_lower(extension.begin(), extension.end());
                
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

        void SceneImportRequestHandler::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.emplace_back(AZ_CRC_CE("AssetImportRequestHandler"));
        }
    } // namespace SceneAPI
} // namespace AZ

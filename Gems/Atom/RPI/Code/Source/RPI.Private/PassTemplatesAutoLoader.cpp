/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RPI.Private/PassTemplatesAutoLoader.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Gem/GemInfo.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        void PassTemplatesAutoLoader::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<PassTemplatesAutoLoader, Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<PassTemplatesAutoLoader>("PassTemplatesAutoLoader", "A service that loads PassTemplates.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void PassTemplatesAutoLoader::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void PassTemplatesAutoLoader::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PassTemplatesAutoLoader"));
        }

        void PassTemplatesAutoLoader::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PassTemplatesAutoLoader"));
        }

        void PassTemplatesAutoLoader::Activate()
        {
            // Register the event handler.
            m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
                [&]()
                {
                    LoadPassTemplates();
                });
            AZ::RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);
        }

        void PassTemplatesAutoLoader::Deactivate()
        {

        }

        void PassTemplatesAutoLoader::LoadPassTemplates()
        {
            auto* settingsRegistry = AZ::SettingsRegistry::Get();
            AZStd::vector<AzFramework::GemInfo> gemInfoList;
            if (!AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
            {
                AZ_Warning(LogWindow, false, "%s Failed to get Gems info.\n", __FUNCTION__);
                return;
            }

            auto* passSystemInterface = AZ::RPI::PassSystemInterface::Get();
            AZStd::unordered_set<AZStd::string> loadedTemplates; // See CAVEAT below.
            auto loadFunc = [&](const AZStd::string& assetPath)
            {
                if (loadedTemplates.count(assetPath) > 0)
                {
                    // CAVEAT: Most of the times Game Projects contain a Gem of the same name
                    // inside of them, this is why we check first with @loadedTemplates before attempting to load
                    // the PassTemplate asset located at <PROJECT_ROOT>/Passes/<PROJECT_NAME>/AutoLoadPassTemplates.azasset
                    return;
                }
                const auto assetId = AssetUtils::GetAssetIdForProductPath(assetPath.c_str(), AssetUtils::TraceLevel::None);
                if (!assetId.IsValid())
                {
                    // This is the most common scenario.
                    return;
                }
                if (!passSystemInterface->LoadPassTemplateMappings(assetPath))
                {
                    AZ_Error(LogWindow, false, "Failed to load PassTemplates at '%s'.\n", assetPath.c_str());
                    return;
                }
                AZ_Printf(LogWindow, "Successfully load PassTemplates from '%s'.\n", assetPath.c_str());
                loadedTemplates.emplace(AZStd::move(assetPath));
            };

            for (const auto& gemInfo : gemInfoList)
            {
                AZStd::string assetPath = AZStd::string::format("Passes/%s/AutoLoadPassTemplates.azasset", gemInfo.m_gemName.c_str());
                loadFunc(assetPath);
            }

            // Besides the Gems, a Game Project can also provide PassTemplates.
            // <PROJECT_ROOT>/Assets/Passes/<PROJECT_NAME>/AutoLoadPassTemplates.azasset
            // <PROJECT_ROOT>/Passes/<PROJECT_NAME>/AutoLoadPassTemplates.azasset
            const auto projectName = AZ::Utils::GetProjectName(settingsRegistry);
            if (!projectName.empty())
            {
                {
                    AZStd::string assetPath = AZStd::string::format("Passes/%s/AutoLoadPassTemplates.azasset", projectName.c_str());
                    loadFunc(assetPath);
                }

                {
                    AZStd::string assetPath = AZStd::string::format("Assets/Passes/%s/AutoLoadPassTemplates.azasset", projectName.c_str());
                    loadFunc(assetPath);
                }
            }
        }

    } // namespace RPI
} // namespace AZ

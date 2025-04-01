/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Script/LuaEditorSystemComponent.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QIcon>

namespace AzToolsFramework
{
    namespace Script
    {
        void LuaEditorSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<LuaEditorSystemComponent, AZ::Component>();
            }
        }

        void LuaEditorSystemComponent::Activate()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler::BusConnect(LuaComponentScriptBusId);
        }

        void LuaEditorSystemComponent::Deactivate()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler::BusDisconnect(LuaComponentScriptBusId);
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void LuaEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LuaEditorSystemComponent"));
        }

        void LuaEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ScriptService"));
        }

        void LuaEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LuaEditorSystemComponent"));
        }

        void LuaEditorSystemComponent::AddSourceFileCreators(
            [[maybe_unused]] const char* fullSourceFolderName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            auto luaAssetCreator = [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
            {
                const AZStd::string_view defaultScriptName = "NewScript";

                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(
                    fullSourceFolderNameInCallback.c_str(),
                    defaultScriptName.data(),
                    LuaExtension,
                    fullFilepath);

                MakeFilenameUnique(fullSourceFolderNameInCallback, defaultScriptName, fullFilepath);

                const auto outcome = SaveLuaScriptFile(fullFilepath, "");
                if (outcome.IsSuccess())
                {
                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                        &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                        fullFilepath,
                        AZ::Crc32(),
                        true);
                }
                else
                {
                    AZ_Error(LogName, false, outcome.GetError().c_str());
                }
            };

            creators.push_back({ "Lua_creator", "Lua Script", QIcon(), luaAssetCreator });

            auto luaComponentAssetCreator = [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
            {
                const AZStd::string_view defaultScriptName = "NewComponent";

                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback.c_str(),
                    defaultScriptName.data(),
                    LuaExtension,
                    fullFilepath);

                MakeFilenameUnique(fullSourceFolderNameInCallback, defaultScriptName, fullFilepath);

                const AZStd::string scriptBoilerplate = GenerateLuaComponentBoilerplate(AZ::IO::Path(fullFilepath).Stem().Native());

                const auto outcome = SaveLuaScriptFile(fullFilepath, scriptBoilerplate);
                if (outcome.IsSuccess())
                {
                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                        &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                        fullFilepath,
                        LuaComponentScriptBusId,
                        true);
                }
                else
                {
                    AZ_Error(LogName, false, outcome.GetError().c_str());
                }
            };

            creators.push_back({ "LuaComponent_creator", "Lua Component Script", QIcon(), luaComponentAssetCreator });
        }

        void LuaEditorSystemComponent::AddSourceFileOpeners(
            const char* fullSourceFileName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            if (AZ::IO::Path(fullSourceFileName).Extension() == LuaExtension)
            {
                const auto luaScriptOpener = [](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid&)
                {
                    AzToolsFramework::EditorRequestBus::Broadcast(
                        &AzToolsFramework::EditorRequests::LaunchLuaEditor, fullSourceFileNameInCallback);
                };

                openers.push_back({ "O3DE_LUA_Editor", "Open in Open 3D Engine LUA Editor...", QIcon(), luaScriptOpener });
            }
        }

        void LuaEditorSystemComponent::HandleInitialFilenameChange(const AZStd::string_view fullFilepath)
        {
            const AZ::IO::Path filepath = AZ::IO::Path(fullFilepath);
            if (filepath.Extension() == LuaExtension)
            {
                const AZStd::string scriptBoilerplate = GenerateLuaComponentBoilerplate(filepath.Stem().Native());

                const auto outcome = SaveLuaScriptFile(fullFilepath, scriptBoilerplate);
                if (!outcome.IsSuccess())
                {
                    AZ_Error(LogName, false, outcome.GetError().c_str());
                }
            }
        }

        AZStd::string LuaEditorSystemComponent::GenerateLuaComponentBoilerplate(const AZStd::string& componentName)
        {
            constexpr const char* namePlaceholder = "$SCRIPT_NAME";
            AZStd::string luaComponentBoilerplate = R"LUA(-- $SCRIPT_NAME.lua

local $SCRIPT_NAME = 
{
    Properties =
    {
        -- Property definitions
    }
}

function $SCRIPT_NAME:OnActivate()
    -- Activation Code
end

function $SCRIPT_NAME:OnDeactivate()
    -- Deactivation Code
end

return $SCRIPT_NAME)LUA";

            AZ::StringFunc::Replace(luaComponentBoilerplate, namePlaceholder, componentName.c_str());
            return luaComponentBoilerplate;
        }

        void LuaEditorSystemComponent::MakeFilenameUnique(
            const AZStd::string& directoryPath,
            const AZStd::string& filename,
            AZStd::string& outFullFilepath) const
        {
            int fileCounter = 0;
            while (AZ::IO::FileIOBase::GetInstance()->Exists(outFullFilepath.c_str()))
            {
                fileCounter++;
                const AZStd::string incrementalFilename = filename + AZStd::to_string(fileCounter);

                AZ::StringFunc::Path::ConstructFull(directoryPath.c_str(),
                    incrementalFilename.c_str(),
                    LuaExtension,
                    outFullFilepath);
            }
        }

        AZ::Outcome<void, AZStd::string> LuaEditorSystemComponent::SaveLuaScriptFile(
            const AZStd::string& fullFilepath,
            const AZStd::string& fileContents) const
        {
            const AZStd::string correctedFilepath = AZ::IO::Path(fullFilepath).MakePreferred().Native();

            AZ::IO::SystemFile scriptFile = AZ::IO::SystemFile();
            int openMode = AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY;
            scriptFile.Open(correctedFilepath.c_str(), openMode);

            if (scriptFile.IsOpen())
            {
                auto bytesWritten = scriptFile.Write(fileContents.c_str(), strlen(fileContents.c_str()));

                scriptFile.Close();

                if (bytesWritten == strlen(fileContents.c_str()))
                {
                    return AZ::Success();
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Failed to write contents to Lua file: %s", correctedFilepath.c_str()));
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Failed to open file when writing Lua script: %s", correctedFilepath.c_str()));
            }
        }

    } // namespace AzComponents
} // namespace AzToolsFramework

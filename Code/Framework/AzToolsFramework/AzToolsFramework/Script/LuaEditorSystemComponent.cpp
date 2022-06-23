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
        }

        void LuaEditorSystemComponent::Deactivate()
        {
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
            auto luaAssetCreator = [&](const char* fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
            {
                AZStd::string defaultScriptName = "NewScript";

                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback,
                    defaultScriptName.c_str(),
                    m_luaExtension,
                    fullFilepath);

                MakeFilenameUnique(fullSourceFolderNameInCallback, defaultScriptName, fullFilepath);

                auto outcome = SaveLuaScriptFile(fullFilepath, "");
                if (outcome.IsSuccess())
                {
                    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                        &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::NotifyAssetWasCreatedInEditor, fullFilepath);
                }
                else
                {
                    AZ_Error(LogName, false, outcome.GetError().c_str());
                }
            };

            creators.push_back({ "Lua_creator", "Lua Script", QIcon(), luaAssetCreator });

            auto luaComponentAssetCreator = [&](const char* fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
            {
                AZStd::string defaultScriptName = "NewComponent";

                AZStd::string fullFilepath;
                AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback,
                    defaultScriptName.c_str(),
                    m_luaExtension,
                    fullFilepath);

                MakeFilenameUnique(fullSourceFolderNameInCallback, defaultScriptName, fullFilepath);

                AZStd::string scriptBoilerplate = GenerateLuaComponentBoilerplate(defaultScriptName);

                auto outcome = SaveLuaScriptFile(fullFilepath, scriptBoilerplate);
                if (outcome.IsSuccess())
                {
                    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                        &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::NotifyAssetWasCreatedInEditor, fullFilepath);
                }
                else
                {
                    AZ_Error(LogName, false, outcome.GetError().c_str());
                }
            };

            creators.push_back({ "LuaComponent_creator", "Lua Component Script", QIcon(), luaComponentAssetCreator });
        }

        void LuaEditorSystemComponent::AddSourceFileOpeners(
            [[maybe_unused]] const char* fullSourceFileName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            [[maybe_unused]] AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            if (AZ::IO::Path(fullSourceFileName).Extension() == m_luaExtension)
            {
                auto luaScriptOpener = [](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid&)
                {
                    AzToolsFramework::EditorRequestBus::Broadcast(
                        &AzToolsFramework::EditorRequests::LaunchLuaEditor, fullSourceFileNameInCallback);
                };

                openers.push_back({ "O3DE_LUA_Editor", "Open in Open 3D Engine LUA Editor...", QIcon(), luaScriptOpener });
            }
        }

        AZStd::string LuaEditorSystemComponent::GenerateLuaComponentBoilerplate(const AZStd::string& filename)
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

            AZ::StringFunc::Replace(luaComponentBoilerplate, namePlaceholder, filename.c_str());
            return luaComponentBoilerplate;
        }

        void LuaEditorSystemComponent::MakeFilenameUnique(
            const AZStd::string& directoryPath,
            const AZStd::string& filename,
            AZStd::string& outFullFilepath)
        {
            int fileCounter = 0;
            while (AZ::IO::FileIOBase::GetInstance()->Exists(outFullFilepath.c_str()))
            {
                fileCounter++;
                AZStd::string filenameDigit = AZStd::to_string(fileCounter);

                AZ::StringFunc::Path::ConstructFull(directoryPath.c_str(),
                    (filename + filenameDigit).c_str(),
                    m_luaExtension,
                    outFullFilepath);
            }
        }

        AZ::Outcome<void, AZStd::string> LuaEditorSystemComponent::SaveLuaScriptFile(
            const AZStd::string& fullFilepath,
            const AZStd::string& fileContents)
        {
            AZ::IO::SystemFile scriptFile = AZ::IO::SystemFile();
            scriptFile.Open(fullFilepath.c_str(), static_cast<int>(AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText));
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
                    return AZ::Failure(AZStd::string::format("Failed to write contents to Lua file: %s", fullFilepath.c_str()));
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Failed to open file when writing Lua script: %s", fullFilepath.c_str()));
            }
        }

    } // namespace AzComponents
} // namespace AzToolsFramework

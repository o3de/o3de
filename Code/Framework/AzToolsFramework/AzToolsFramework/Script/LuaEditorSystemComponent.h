/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

namespace AzToolsFramework
{
    namespace Script
    {
        class LuaEditorSystemComponent
            : public AZ::Component
            , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(LuaEditorSystemComponent, "{8F3DFC84-2D59-416C-B04D-56C550114D9B}");

            LuaEditorSystemComponent() = default;
            ~LuaEditorSystemComponent() = default;

            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus
            void AddSourceFileCreators(
                const char* fullSourceFolderName,
                const AZ::Uuid& sourceUUID,
                AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators) override;
            void AddSourceFileOpeners(
                const char* fullSourceFileName,
                const AZ::Uuid& sourceUUID,
                AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationsBus
            void HandleInitialFilenameChange(const AZStd::string_view fullFilepath) override;
            ////////////////////////////////////////////////////////////////////////

            //! Generates boilerplate for a basic Lua component script which has the given component name.
            static AZStd::string GenerateLuaComponentBoilerplate(const AZStd::string& componentName);

        protected:
            void Activate() override;
            void Deactivate() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        private:
            //! Appends the next available digit to the filename if the filename is already taken.
            void MakeFilenameUnique(const AZStd::string& directoryPath, const AZStd::string& filename, AZStd::string& outFullFilepath) const;

            //! Saves the given contents to a Lua script on disk.
            //! The file is created if it does not exist, otherwise the file contents will be overwritten.
            AZ::Outcome<void, AZStd::string> SaveLuaScriptFile(const AZStd::string& fullFilepath, const AZStd::string& fileContents) const;

            static constexpr char LuaExtension[] = ".lua";
            static constexpr char LogName[] = "LuaEditorSystemComponent";

            // The ebus address for Lua component script initial name change notifications
            static constexpr AZ::Crc32 LuaComponentScriptBusId = AZ::Crc32("LuaComponentScriptRenameHandler");
        };
} // namespace Component
} // namespace AzToolsFramework

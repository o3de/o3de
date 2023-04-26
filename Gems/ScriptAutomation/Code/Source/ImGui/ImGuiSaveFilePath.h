/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>

namespace ScriptAutomation
{
    //! Provides a common utility for either auto-generating or manually selecting a file path for saving.
    class ImGuiSaveFilePath
    {
    public:
        struct WidgetSettings
        {
            struct Labels
            {
                const char* m_filePath = "File Path";
            } m_labels;
        };

        static void Reflect(AZ::ReflectContext* context);

        void Activate();
        void Deactivate();

        //! @param configFilePath - Path to a local file for maintaining state between runs. Should start with "@user@/"
        explicit ImGuiSaveFilePath(AZStd::string_view configFilePath);

        void SetDefaultFolder(const AZStd::string& folderPath);
        void SetDefaultFileName(const AZStd::string& fileNameNoExt);

        //! Sets a list of available extensions that can be used in Auto mode. The first one will be the default.
        void SetAvailableExtensions(const AZStd::vector<AZStd::string>& extensions);

        //! Draw the ImGui
        //! @return true if the asset selection changed
        void Tick(const WidgetSettings& widgetSettings);

        //! Returns the save file path chosen by the user, either manually entered or auto-generated.
        AZStd::string GetSaveFilePath() const;

        //! Returns the path to a new file that doesn't exist, in the form:
        //! "[default folder]/[default file name]_[counter].[current extension]"
        //! Note, the function you probably want is GetSaveFilePath(). GetNextAutoSaveFilePath() is only for special cases.
        AZStd::string GetNextAutoSaveFilePath();

    private:

        struct Config final
        {
            AZ_TYPE_INFO(Config, "{9844A4A8-FE6F-4B49-96D6-95E185C3BE1E}");
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            bool m_autoMode = true;
            int m_currentExtension = 0;
            AZStd::string m_filePath;
        };

        static bool GetExtension(void* data, int index, const char** out);

        bool LoadConfigFile();
        void SaveConfigFile();

        AZStd::string m_configFilePath;
        Config m_config;

        AZStd::string m_defaultFolder;
        AZStd::string m_defaultFileName;
        AZStd::vector<AZStd::string> m_availableExtensions;
        uint32_t m_autoFileIndex = 0;
        char m_filePath[AZ_MAX_PATH_LEN] = "";
    };
} // namespace ScriptAutomation

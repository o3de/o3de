/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        enum class SaveAllPrefabsPreference
        {
            AskEveryTime,
            SaveAll,
            SaveNone 
        };

        /*!
         * PrefabLoaderInterface
         * Interface for saving/loading Prefab files.
         */
        class PrefabLoaderInterface
        {
        public:
            AZ_RTTI(PrefabLoaderInterface, "{C7B26070-B1BE-470C-BE5A-45B0CF112E68}");

            /**
             * Load Prefab Template from given file path to memory and return the id of loaded Template.
             * Converts Prefab Asset form into Prefab Template form by expanding source path and patch info
             * into fully formed nested template info.
             * @param filePath A Prefab Template file path.
             * @return A unique id of Template on filePath loaded. Return invalid template id if loading Template on filePath failed.
             */
            virtual TemplateId LoadTemplateFromFile(AZ::IO::PathView filePath) = 0;

            /**
             * Load Prefab Template from given content string to memory and return the id of loaded Template.
             * Converts .prefab form into Prefab Template form by expanding source path and patch info
             * into fully formed nested template info.
             * @param content Json content of the prefab
             * @param originPath Path that will be used for the prefab in case of saved into a file.
             * @return A unique id of Template on filePath loaded. Return invalid template id if loading Template on filePath failed.
             */
            virtual TemplateId LoadTemplateFromString(AZStd::string_view content, AZ::IO::PathView originPath = GeneratePath()) = 0;

            /**
            * Saves a Prefab Template to the the source path registered with the template.
            * Converts Prefab Template form into Prefab Asset form by collapsing nested Template info
            * into a source path and patches.
            * @param templateId Id of the template to be saved
            * @return bool on whether the operation succeeded or not
            */
            virtual bool SaveTemplate(TemplateId templateId) = 0;

            /**
             * Saves a Prefab Template to the provided absolute source path, which needs to match the relative path in the template.
             * Converts Prefab Template form into .prefab form by collapsing nested Template info
             * into a source path and patches.
             * @param templateId Id of the template to be saved
             * @param absolutePath Absolute path to save the file to
             * @return bool on whether the operation succeeded or not
             */
            virtual bool SaveTemplateToFile(TemplateId templateId, AZ::IO::PathView absolutePath) = 0;

            /**
            * Saves a Prefab Template into the provided output string.
            * Converts Prefab Template form into .prefab form by collapsing nested Template info
            * into a source path and patches.
            * @param templateId Id of the template to be saved
            * @param outputJson Will contain the serialized template json on success
            * @return bool on whether the operation succeeded or not
            */
            virtual bool SaveTemplateToString(TemplateId templateId, AZStd::string& outputJson) = 0;

            //! Converts path into full absolute path. This will be used by loading/save IO operations.
            //! The path will always have the correct separator for the current OS
            virtual AZ::IO::Path GetFullPath(AZ::IO::PathView path) = 0;

            //! Converts path into a path that's relative to the highest-priority containing folder of all the folders registered
            //! with the engine.
            //! This path will be the path that appears in the .prefab file.
            //! The path will always use the '/' separator.
            virtual AZ::IO::Path GenerateRelativePath(AZ::IO::PathView path) = 0;

            virtual SaveAllPrefabsPreference GetSaveAllPrefabsPreference() const = 0;
            virtual void SetSaveAllPrefabsPreference(SaveAllPrefabsPreference saveAllPrefabsPreference) = 0;

        protected:

            // Generates a new path
            static AZ::IO::Path GeneratePath();
        };
    } // namespace Prefab
} // namespace AzToolsFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::Prefab::SaveAllPrefabsPreference, "{7E61EA82-4DE4-4A3F-945F-C8FEDC1114B5}");
}


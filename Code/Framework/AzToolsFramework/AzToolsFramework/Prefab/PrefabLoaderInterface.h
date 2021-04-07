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

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        constexpr size_t MaxPrefabFileSize = 1024 * 1024;

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

            //! Converts path into a relative path to the current project, this will be the paths in .prefab file.
            //! The path will always have '/' separator.
            virtual AZ::IO::Path GetRelativePathToProject(AZ::IO::PathView path) = 0;

        protected:

            // Generates a new path
            static AZ::IO::Path GeneratePath();
        };

    } // namespace Prefab
} // namespace AzToolsFramework


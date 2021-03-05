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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
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
             * @return A unique id of Template on filePath loaded. Return null id if loading Template on filePath failed.
             */
            virtual TemplateId LoadTemplate(const AZStd::string& filePath) = 0;

            /**
            * Saves a Prefab Template to the the source path registered with the template.
            * Converts Prefab Template form into Prefab Asset form by collapsing nested Template info
            * into a source path and patches.
            * @param templateId Id of the template to be saved
            * @return bool on whether the operation succeeded or not
            */
            virtual bool SaveTemplate(const TemplateId& templateId) = 0;
        };


    } // namespace Prefab
} // namespace AzToolsFramework


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

#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    class Entity;

    namespace Prefab
    {
        class PrefabSystemComponentInterface;

        /**
        * The Prefab Loader helps saving/loading Prefab files.
        */
        class PrefabLoader final
            : public PrefabLoaderInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabLoader, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabLoader, "{A302B072-4DC4-4B7E-9188-226F56A3429C8}", PrefabLoaderInterface);

            //////////////////////////////////////////////////////////////////////////
            // PrefabLoaderInterface interface implementation

            /**
             * Load Prefab Template from given file path to memory and return the id of loaded Template.
             * Converts Prefab Asset form into Prefab Template form by expanding source path and patch info
             * into fully formed nested template info.
             * @param filePath A Prefab Template file path.
             * @return A unique id of Template on filePath loaded. Return null id if loading Template on filePath failed.
             */
            TemplateId LoadTemplate(const AZStd::string& filePath) override;

            /**
            * Saves a Prefab Template to the the source path registered with the template.
            * Converts Prefab Template form into Prefab Asset form by collapsing nested Template info
            * into a source path and patches.
            * @param templateId Id of the template to be saved
            * @return bool on whether the operation succeeded or not
            */
            bool SaveTemplate(const TemplateId& templateId) override;
            //////////////////////////////////////////////////////////////////////////

            void RegisterPrefabLoaderInterface();

            void UnregisterPrefabLoaderInterface();

        private:
            /**
             * Load Prefab Template from given file path to memory and return the id of loaded Template.
             * @param filePath A path to a Prefab Template file.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return A unique id of Template on filePath loaded. Return invalid id if loading Template on filePath failed.
             */
            TemplateId LoadTemplate(
                const AZStd::string& filePath,
                AZStd::unordered_set<AZStd::string>& progressedFilePathsSet);

            /**
             * Load nested instance given a nested instance value iterator and target Template with its id.
             * @param instanceIterator A nested instance value iterator.
             * @param targetTemplateId A unique id of target Template.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return If loading the nested instance pointed by instanceIt on targetTemplate succeeded or not.
             */
            bool LoadNestedInstance(
                PrefabDomValue::MemberIterator& instanceIterator,
                TemplateId targetTemplateId,
                AZStd::unordered_set<AZStd::string>& progressedFilePathsSet);

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework


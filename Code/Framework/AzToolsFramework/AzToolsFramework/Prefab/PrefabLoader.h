/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <Prefab/ScriptingPrefabLoader.h>

namespace AZ
{
    class Entity;
} // namespace AZ

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabSystemComponentInterface;
        class Template;
        using TemplateReference = AZStd::optional<AZStd::reference_wrapper<Template>>;

        /**
        * The Prefab Loader helps saving/loading Prefab files.
        */
        class PrefabLoader final
            : public PrefabLoaderInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabLoader, AZ::SystemAllocator);
            AZ_RTTI(PrefabLoader, "{A302B072-4DC4-4B7E-9188-226F56A3429C}", PrefabLoaderInterface);

            static void Reflect(AZ::ReflectContext* context);

            //////////////////////////////////////////////////////////////////////////
            // PrefabLoaderInterface interface implementation

            /**
             * Load Prefab Template from given file path to memory and return the id of loaded Template.
             * Converts .prefab into Prefab Template form by expanding source path and patch info
             * into fully formed nested template info.
             * @param filePath A Prefab Template file path.
             * @return A unique id of Template on filePath loaded. Return invalid template id if loading Template on filePath failed.
             */
            TemplateId LoadTemplateFromFile(AZ::IO::PathView filePath) override;

             /**
             * Reloads Prefab Template from given file path and updates values that are changed
             * in the source file.
             * @param relativePath A Prefab Template relative file path.
             */
            void ReloadTemplateFromFile(AZ::IO::PathView relativePath);

            /**
             * Load Prefab Template from given content string to memory and return the id of loaded Template.
             * Converts .prefab form into Prefab Template form by expanding source path and patch info
             * into fully formed nested template info.
             * @param content Json content of the prefab
             * @param originPath Path that will be used for the prefab in case of saved into a file.
             * @return A unique id of Template on filePath loaded. Return invalid template id if loading Template on filePath failed.
             */
            TemplateId LoadTemplateFromString(AZStd::string_view content, AZ::IO::PathView originPath = GeneratePath()) override;

            /**
             * Saves a Prefab Template to the the source path registered with the template.
             * Converts Prefab Template form into .prefab form by collapsing nested Template info
             * into a source path and patches.
             * @param templateId Id of the template to be saved
             * @return bool on whether the operation succeeded or not
             */
            bool SaveTemplate(TemplateId templateId) override;

            /**
             * Saves a Prefab Template to the provided absolute source path, which needs to match the relative path in the template.
             * Converts Prefab Template form into .prefab form by collapsing nested Template info
             * into a source path and patches.
             * @param templateId Id of the template to be saved
             * @param absolutePath Absolute path to save the file to
             * @return bool on whether the operation succeeded or not
             */
            bool SaveTemplateToFile(TemplateId templateId, AZ::IO::PathView absolutePath) override;

            /**
             * Saves a Prefab Template into the provided output string.
             * Converts Prefab Template form into .prefab form by collapsing nested Template info
             * into a source path and patches.
             * @param templateId Id of the template to be saved
             * @param outputJson Will contain the serialized template json on success
             * @return bool on whether the operation succeeded or not
             */
            bool SaveTemplateToString(TemplateId templateId, AZStd::string& outputJson) override;

            //////////////////////////////////////////////////////////////////////////

            void RegisterPrefabLoaderInterface();
            void UnregisterPrefabLoaderInterface();

            //! Converts path into full absolute path. This will be used by loading/save IO operations.
            //! The path will always have the correct separator for the current OS
            AZ::IO::Path GetFullPath(AZ::IO::PathView path) override;

            //! Converts path into a path that's relative to the highest-priority containing folder of all the folders registered
            //! with the engine.
            //! This path will be the path that appears in the .prefab file.
            //! The path will always use the '/' separator.
            AZ::IO::Path GenerateRelativePath(AZ::IO::PathView path) override;

            //! Returns if the path is a valid path for a prefab
            static bool IsValidPrefabPath(AZ::IO::PathView path);

            SaveAllPrefabsPreference GetSaveAllPrefabsPreference() const override;
            void SetSaveAllPrefabsPreference(SaveAllPrefabsPreference saveAllPrefabsPreference) override;

        private:

            /**
             * Copies the template dom provided and manipulates it into the proper format to be saved to disk.
             * @param templateRef The template whose dom we want to transform into the proper format to be saved to disk.
             * @param[out] output The PrefabDom reference we want to store the result into.
             * @return True if the operation was completed correctly, false otherwise.
             */
            bool CopyTemplateIntoPrefabFileFormat(
                TemplateReference templateRef,
                PrefabDom& output);

            /**
             * Removes links from given template instances that are not present in the dom loaded from file.
             * @param Reference to the nested instances in the dom loaded from file.
             * @param Reference to the nested instances in the existing template dom.
             */
            void RemoveStaleLinksOnReload(
                PrefabDomValueReference instancesFileReference, PrefabDomValueReference instancesTemplateReference);

            /**
             * Reload nested instance given a nested instance value iterator and target Template with its id.
             * @param instanceIterator A nested instance value iterator.
             * @param targetTemplateId A unique id of target Template.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return If reloading the nested instance pointed by instanceIterator on targetTemplate succeeded or not.
             */
            bool ReloadNestedInstance(
                PrefabDomValue::MemberIterator& instanceIterator,
                TemplateId targetTemplateId,
                AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet);

            /**
             * Load Prefab Template from given file path to memory and return the id of loaded Template.
             * @param filePath A path to a Prefab Template file.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return A unique id of Template on filePath loaded. Return invalid id if loading Template on filePath failed.
             */
            TemplateId LoadTemplateFromFile(
                AZ::IO::PathView filePath,
                AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet);

            /**
             * Load Prefab Template from given string to memory and return the id of loaded Template.
             * @param fileContent Json content of the template
             * @param filePath Path that will be used for the template if saved to file.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return A unique id of Template on filePath loaded. Return invalid id if loading Template on filePath failed.
             */
            TemplateId LoadTemplateFromString(
                AZStd::string_view fileContent,
                AZ::IO::PathView filePath,
                AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet);

            /**
             * Load nested instance given a nested instance value iterator and target Template with its id.
             * @param instanceIterator A nested instance value iterator.
             * @param targetTemplateId A unique id of target Template.
             * @param progressedFilePathsSet An unordered_set to track if there's any cyclical dependency between Templates.
             * @return If loading the nested instance pointed by instanceIterator on targetTemplate succeeded or not.
             */
            bool LoadNestedInstance(
                PrefabDomValue::MemberIterator& instanceIterator,
                TemplateId targetTemplateId,
                AZStd::unordered_set<AZ::IO::Path>& progressedFilePathsSet);

            /*
             * Manipulate the provided PrefabDom into the right format to be stored in memory for editor usage.
             * @param loadedTemplateDom The template to manipulate. Changes will be applied in place.
             * @return True if the manipulations where applied correctly, false otherwise.
             */
            bool SanitizeLoadedTemplate(PrefabDomReference loadedTemplateDom);
            
            /*
             * Manipulate the provided PrefabDom into the right format to be stored to disk.
             * @param savingTemplateDom The template to manipulate. Changes will be applied in place.
             * @return True if the manipulations where applied correctly, false otherwise.
             */
            bool SanitizeSavingTemplate(PrefabDomReference savingTemplateDom);

            //! Retrieves Dom content and its path from a template id
            AZStd::optional<AZStd::pair<PrefabDom, AZ::IO::Path>> StoreTemplateIntoFileFormat(TemplateId templateId);

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            ScriptingPrefabLoader m_scriptingPrefabLoader;
            AZ::IO::Path m_projectPathWithOsSeparator;
            AZ::IO::Path m_projectPathWithSlashSeparator;
        };
    } // namespace Prefab
} // namespace AzToolsFramework


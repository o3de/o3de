/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RHI.Reflect/NameIdReflectionMap.h>

#include <Atom/RPI.Public/Configuration.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Reflect/System/AssetAliases.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RPI
    {
        class Pass;
        class PassTemplate;

        //! Library used to keep track of passes and pass template. Responsible for
        //! Storing all PassTemplates
        //! Storing all PassAssets
        //! Storing references to all Passes
        //! Retrieving a PassTemplate given it's Name
        //! Retrieving all passes given a PassTemplate
        //! Retrieving all passes with a given Name
        //! Retrieving all passes with a given PassFilter
        //!
        //! Because PassLibrary enables PassTemplates to be referenced with just a Name,
        //! this enables code to reference Passes defined in data and vice-versa
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API PassLibrary final
            : public Data::AssetBus::MultiHandler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            AZ_DISABLE_COPY_MOVE(PassLibrary);

        public:

            struct TemplateEntry
            {
                // The PassTemplate that will be used to create Passes
                AZStd::shared_ptr<PassTemplate> m_template;

                // The asset from which the pass template was created (if any)
                Data::Asset<PassAsset> m_asset;

                // The list of passes created from this template
                AZStd::vector<Pass*> m_passes;

                // The pass templates mapping asset id which this template is coming from.  
                Data::AssetId m_mappingAssetId;
            };

            typedef AZStd::unordered_map<Name, TemplateEntry> TemplateEntriesByName;

            PassLibrary() = default;
            ~PassLibrary() = default;

            void Init();
            void Shutdown();

            //! Register a Pass/PassTemplate with the library. 
            bool AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate, bool hotReloading = false);
            void AddPass(Pass* pass);

            //! Returns whether the library has a template/passes associated with a template given a template name
            bool HasTemplate(const Name& templateName) const;
            bool HasPassesForTemplate(const Name& templateName) const;

            //! Retrieves a PassTemplate from the library
            const AZStd::shared_ptr<const PassTemplate> GetPassTemplate(const Name& name) const;

            //! Returns a list of passes using the template with the name passed as argument
            const AZStd::vector<Pass*>& GetPassesForTemplate(const Name& templateName) const;

            //! Removes a PassTemplate by name, only if the following two conditions are met:
            //! 1- The template was NOT created from an Asset. This means the template will be erasable
            //!    only if it was created at runtime with C++.
            //! 2- The are no instantiated Passes referencing such template.
            //! If the template exists but both conditions are not met then the function will assert.
            //! If a template with the given name doesn't exist the function does nothing.
            //! This function should be used judiciously, and under rare circumstances. For example,
            //! Applications that iteratively create and need to delete templates at runtime.
            void RemovePassTemplate(const Name& name);

            //! Removes a pass from both it's associated template (if it has one) and from the pass name mapping
            void RemovePassFromLibrary(Pass* pass);

            //! Load pass templates which are list in an AssetAliases
            bool LoadPassTemplateMappings(const AZStd::string& templateMappingPath);
            bool LoadPassTemplateMappings(Data::Asset<AnyAsset> mappingAsset);

            //! Visit each pass which matches the filter
            void ForEachPass(const PassFilter& passFilter, AZStd::function<PassFilterExecutionFlow(Pass*)> passFunction);

        private:

            // Retrieves a template entry given a name, or nullptr if not found
            TemplateEntry* GetEntry(const Name& templateName);
            const TemplateEntry* GetEntry(const Name& templateName) const;

            // Adds templates to the library that are core to the RPI
            void AddCoreTemplates();

            // Functions for adding individual pass templates to the library. To create a new pass template in C++,
            // add a function here, implement it in the .cpp and call it in AddCoreTemplates()
            void AddCopyPassTemplate();

            // Loads pass template from a pass asset
            bool LoadPassAsset(const Name& name, const Data::Asset<PassAsset>& passAsset, bool hotReloading = false);

            // Find asset with specified pass template asset id and load pass template from the asset.
            bool LoadPassAsset(const Name& name, const Data::AssetId& passAssetId);

            // Data::AssetBus::Handler overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // Sets all formats to nearest device supported formats and warns if changes where made
            void ValidateDeviceFormats(const AZStd::shared_ptr<PassTemplate>& passTemplate);

            // Members...

            // A list of PassTemplates and associated data registered with the library
            TemplateEntriesByName m_templateEntries;

            // Each of these AnyAssets is a "pass template mapping", which contains a map of template name to pass asset ID.
            // We keep track of them so we can respond to the event in which they're reloaded
            AZStd::unordered_map<Data::AssetId, Data::Asset<AnyAsset>> m_templateMappingAssets;

            // Pass name to pass mapping for all pass instances
            AZStd::unordered_map<Name, AZStd::vector<Pass*>> m_passNameMapping;

            // Whether the pass library is shutting down. In this case we ignore removal functions
            bool m_isShuttingDown = false;
        };
    }   // namespace RPI
}   // namespace AZ

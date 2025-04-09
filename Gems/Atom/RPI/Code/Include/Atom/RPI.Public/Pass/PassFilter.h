/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        class Scene;
        class RenderPipeline;

        class ATOM_RPI_PUBLIC_API PassFilter
        {
        public:
            static PassFilter CreateWithPassName(Name passName, const Scene* scene);
            static PassFilter CreateWithPassName(Name passName, const RenderPipeline* renderPipeline);

            //! Create a PassFilter with pass hierarchy information
            //! Filter for passes which have a matching name and also with ordered parents.
            //! For example, if the filter is initialized with
            //! pass name: "ShadowPass1"
            //! pass parents names: "MainPipeline", "Shadow"
            //! Passes with these names match the filter:
            //!     "Root.MainPipeline.SwapChainPass.Shadow.ShadowPass1" 
            //! or  "Root.MainPipeline.Shadow.ShadowPass1"
            //! or  "MainPipeline.Shadow.Group1.ShadowPass1"
            //!
            //! Passes with these names wont match:
            //!     "MainPipeline.ShadowPass1"
            //! or  "Shadow.MainPipeline.ShadowPass1"
            static PassFilter CreateWithPassHierarchy(const AZStd::vector<Name>& passHierarchy);
            static PassFilter CreateWithPassHierarchy(const AZStd::vector<AZStd::string>& passHierarchy);
            static PassFilter CreateWithTemplateName(Name templateName, const Scene* scene);
            static PassFilter CreateWithTemplateName(Name templateName, const RenderPipeline* renderPipeline);
            template <typename PassClass>
            static PassFilter CreateWithPassClass();

            enum FilterOptions : uint32_t
            {
                Empty = 0,
                PassName = AZ_BIT(0),
                PassTemplateName = AZ_BIT(1),
                PassClass = AZ_BIT(2),
                PassHierarchy = AZ_BIT(3),
                OwnerScene = AZ_BIT(4),
                OwnerRenderPipeline = AZ_BIT(5)
            };

            void SetOwnerScene(const Scene* scene);
            void SetOwnerRenderPipeline(const RenderPipeline* renderPipeline);
            void SetPassName(Name passName);
            void SetTemplateName(Name passTemplateName);
            void SetPassClass(TypeId passClassTypeId);

            const Name& GetPassName() const;
            const Name& GetPassTemplateName() const;

            uint32_t GetEnabledFilterOptions() const;

            //! Return true if the input pass matches the filter
            bool Matches(const Pass* pass) const;

            //! Return true if the input pass matches the filter with selected filter options
            //! The input filter options should be a subset of options returned by GetEnabledFilterOptions()
            //! This function is used to avoid extra checks for passes which was already filtered.
            //! Check PassLibrary::ForEachPass() function's implementation for more details
            bool Matches(const Pass* pass, uint32_t options) const;

        private:
            void UpdateFilterOptions();

            Name m_passName;
            Name m_templateName;
            TypeId m_passClassTypeId = TypeId::CreateNull();
            AZStd::vector<Name> m_parentNames;
            const RenderPipeline* m_ownerRenderPipeline = nullptr;
            const Scene* m_ownerScene = nullptr;
            uint32_t m_filterOptions = 0;
        };

        template <typename PassClass>
        PassFilter PassFilter::CreateWithPassClass()
        {            
            PassFilter filter;
            filter.m_passClassTypeId = PassClass::RTTI_Type();
            filter.UpdateFilterOptions();
            return filter;
        }
    }   // namespace RPI
}   // namespace AZ


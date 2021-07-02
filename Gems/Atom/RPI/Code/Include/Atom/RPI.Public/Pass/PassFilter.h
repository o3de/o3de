/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        // A base class for a filter which can be used to filter passes
        class PassFilter
        {
        public:
            //! Whether the input pass matches with the filter 
            virtual bool Matches(const Pass* pass) const = 0;

            //! Return the pass' name if a pass name is used for the filter.
            //! Return nullptr if the filter doesn't have pass name used for matching
            virtual const Name* GetPassName() const = 0;

            //! Return this filter's info as a string
            virtual AZStd::string ToString() const = 0;
        };

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
        class PassHierarchyFilter
            : public PassFilter
        {
        public:
            AZ_RTTI(PassHierarchyFilter, "{478F169F-BA97-4321-AC34-EDE823997159}", PassFilter);
            AZ_CLASS_ALLOCATOR(PassHierarchyFilter, SystemAllocator, 0);

            //! Construct filter with only pass name. 
            PassHierarchyFilter(const Name& passName);

            //! Construct filter with pass name and its parents' names in the order of the hierarchy
            //! This means k-th element is always an ancestor of the (k-1)-th element.
            //! And the last element is the pass name. 
            PassHierarchyFilter(const AZStd::vector<Name>& passHierarchy);
            PassHierarchyFilter(const AZStd::vector<AZStd::string>& passHierarchy);

            // PassFilter overrides...
            bool Matches(const Pass* pass) const override;
            const Name* GetPassName() const override;
            AZStd::string ToString() const override;

        private:
            PassHierarchyFilter() = delete;

            AZStd::vector<Name> m_parentNames;
            Name m_passName;
        };

        //! Filter for passes based on their class.
        template<typename PassClass>
        class PassClassFilter
            : public PassFilter
        {
        public:
            AZ_RTTI(PassClassFilter, "{AF6E3AD5-433A-462A-997A-F36D8A551D02}", PassFilter);
            AZ_CLASS_ALLOCATOR(PassHierarchyFilter, SystemAllocator, 0);
            PassClassFilter() = default;

            // PassFilter overrides...
            bool Matches(const Pass* pass) const override;
            const Name* GetPassName() const override;
            AZStd::string ToString() const override;
        };

        template<typename PassClass>
        bool PassClassFilter<PassClass>::Matches(const Pass* pass) const
        {
            return pass->RTTI_IsTypeOf(PassClass::RTTI_Type());
        }

        template<typename PassClass>
        const Name* PassClassFilter<PassClass>::GetPassName() const
        {
            return nullptr;
        }

        template<typename PassClass>
        AZStd::string PassClassFilter<PassClass>::ToString() const
        {
            return AZStd::string::format("PassClassFilter<%s>", PassClass::RTTI_TypeName());
        }
    }   // namespace RPI
}   // namespace AZ


/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace AZ
{
    namespace RPI
    {
        PassFilter PassFilter::CreateWithPassName(Name passName, const Scene* scene)
        {
            PassFilter filter;
            filter.m_passName = passName;
            filter.m_ownerScene = scene;
            filter.UpdateFilterOptions();
            return filter;
        }

        PassFilter PassFilter::CreateWithPassName(Name passName, const RenderPipeline* renderPipeline)
        {
            PassFilter filter;
            filter.m_passName = passName;
            filter.m_ownerRenderPipeline = renderPipeline;
            filter.UpdateFilterOptions();
            return filter;
        }

        PassFilter PassFilter::CreateWithTemplateName(Name templateName, const Scene* scene)
        {
            PassFilter filter;
            filter.m_templateName = templateName;
            filter.m_ownerScene = scene;
            filter.UpdateFilterOptions();
            return filter;
        }

        PassFilter PassFilter::CreateWithTemplateName(Name templateName, const RenderPipeline* renderPipeline)
        {
            PassFilter filter;
            filter.m_templateName = templateName;
            filter.m_ownerRenderPipeline = renderPipeline;
            filter.UpdateFilterOptions();
            return filter;
        }

        PassFilter PassFilter::CreateWithPassHierarchy(const AZStd::vector<Name>& passHierarchy)
        {
            PassFilter filter;
            if (passHierarchy.size() == 0)
            {
                AZ_Assert(false, "passHierarchy should have at least one element");
                return filter;
            }

            filter.m_passName = passHierarchy.back();

            filter.m_parentNames.resize(passHierarchy.size() - 1);
            for (uint32_t index = 0; index < filter.m_parentNames.size(); index++)
            {
                filter.m_parentNames[index] = passHierarchy[index];
            }
            filter.UpdateFilterOptions();
            return filter;
        }

        PassFilter PassFilter::CreateWithPassHierarchy(const AZStd::vector<AZStd::string>& passHierarchy)
        {
            PassFilter filter;
            if (passHierarchy.size() == 0)
            {
                AZ_Assert(false, "passHierarchy should have at least one element");
                return filter;
            }

            filter.m_passName = Name(passHierarchy.back());

            filter.m_parentNames.resize(passHierarchy.size() - 1);
            for (uint32_t index = 0; index < filter.m_parentNames.size(); index++)
            {
                filter.m_parentNames[index] = Name(passHierarchy[index]);
            }
            filter.UpdateFilterOptions();
            return filter;
        }

        void PassFilter::SetOwenrScene(const Scene* scene)
        {
            m_ownerScene = scene;
            UpdateFilterOptions();
        }

        void PassFilter::SetOwenrRenderPipeline(const RenderPipeline* renderPipeline)
        {
            m_ownerRenderPipeline = renderPipeline;
            UpdateFilterOptions();
        }

        void PassFilter::SetPassName(Name passName)
        {
            m_passName = passName;
            UpdateFilterOptions();
        }

        void PassFilter::SetTemplateName(Name passTemplateName)
        {
            m_templateName = passTemplateName;
            UpdateFilterOptions();
        }

        void PassFilter::SetPassClass(TypeId passClassTypeId)
        {
            m_passClassTypeId = passClassTypeId;
            UpdateFilterOptions();
        }

        const Name& PassFilter::GetPassName() const
        {
            return m_passName;
        }

        const Name& PassFilter::GetPassTemplateName() const
        {
            return m_templateName;
        }

        uint32_t PassFilter::GetEnabledFilterOptions() const
        {
            return m_filterOptions;
        }

        bool PassFilter::Matches(const Pass* pass) const
        {
            return Matches(pass, m_filterOptions);
        }

        bool PassFilter::Matches(const Pass* pass, uint32_t options) const
        {
            AZ_Assert( (options&m_filterOptions) == options, "options should be a subset of m_filterOptions");

            // return false if the pass doesn't have a pass template or the template's name is not matching
            if (options & FilterOptions::PassTemplateName && (!pass->GetPassTemplate() || pass->GetPassTemplate()->m_name != m_templateName))
            {
                return false;
            }

            if ((options & FilterOptions::PassName) && pass->GetName() != m_passName)
            {
                return false;
            }

            if ((options & FilterOptions::PassClass) && pass->RTTI_GetType() != m_passClassTypeId)
            {
                return false;
            }

            if ((options & FilterOptions::OwnerRenderPipeline) && m_ownerRenderPipeline != pass->GetRenderPipeline())
            {
                return false;
            }
            
            // If the owner render pipeline was checked, the owner scene check can be skipped
            if (options & FilterOptions::OwnerScene)
            {
                if (pass->GetRenderPipeline())
                {
                    // return false if the owner scene doesn't match
                    if (m_ownerScene != pass->GetRenderPipeline()->GetScene())
                    {
                        return false;
                    }
                }
                else
                {
                    // return false if the pass doesn't have an owner scene
                    return false;
                }
            }

            if ((options & FilterOptions::PassHierarchy))
            {
                // Filter for passes which have a matching name and also with ordered parents.
                // For example, if the filter is initialized with
                // pass name: "ShadowPass1"
                // pass parents names: "MainPipeline", "Shadow"
                // Passes with these names match the filter:
                //    "Root.MainPipeline.SwapChainPass.Shadow.ShadowPass1" 
                // or  "Root.MainPipeline.Shadow.ShadowPass1"
                // or  "MainPipeline.Shadow.Group1.ShadowPass1"
                //
                // Passes with these names wont match:
                //    "MainPipeline.ShadowPass1"
                // or  "Shadow.MainPipeline.ShadowPass1"

                ParentPass* parent = pass->GetParent();

                // search from the back of the array with the most close parent 
                for (int32_t index = static_cast<int32_t>(m_parentNames.size() - 1); index >= 0; index--)
                {
                    const Name& parentName = m_parentNames[index];
                    while (parent)
                    {
                        if (parent->GetName() == parentName)
                        {
                            break;
                        }
                        parent = parent->GetParent();
                    }

                    // if parent is nullptr the it didn't find a parent has matching current parentName
                    if (!parent)
                    {
                        return false;
                    }

                    // move to next parent 
                    parent = parent->GetParent();
                }
            }

            return true;
        }

        void PassFilter::UpdateFilterOptions()
        {
            m_filterOptions = FilterOptions::Empty;
            if (!m_passName.IsEmpty())
            {
                m_filterOptions |= FilterOptions::PassName;
            }
            if (!m_templateName.IsEmpty())
            {
                m_filterOptions |= FilterOptions::PassTemplateName;
            }
            if (m_parentNames.size() > 0)
            {
                m_filterOptions |= FilterOptions::PassHierarchy;
            }
            if (m_ownerRenderPipeline)
            {
                m_filterOptions |= FilterOptions::OwnerRenderPipeline;
            }
            if (m_ownerScene)
            {
                // If the OwnerRenderPipeline exists, we shouldn't need to filter owner scene
                // Validate the owner render pipeline belongs to the owner scene
                if (m_filterOptions & FilterOptions::OwnerRenderPipeline)
                {
                    if (m_ownerRenderPipeline->GetScene() != m_ownerScene)
                    {
                        AZ_Warning("RPI", false, "The owner scene filter doesn't match owner render pipeline. It will be skipped.");
                    }
                }
                else
                {
                    m_filterOptions |= FilterOptions::OwnerScene;
                }
            }
            if (!m_passClassTypeId.IsNull())
            {
                m_filterOptions |= FilterOptions::PassClass;
            }
        }
    }   // namespace RPI
}   // namespace AZ

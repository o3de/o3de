/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AZ::Render
{
    //! List of passes to create.
    using PassDescriptorList = AZStd::vector<Name>;

    //! Parent pass for editor states.
    //! This base class is inherited by the specific editor states that wish to implement custom feedback effects.
    //! When a child class of this base class is constructed, the render passes in the pass descriptor list are
    //! constructed and added to the render pipeline. The ordering of the corresponding parent passes in the render
    //! pipeline for these passes is determined by the order in which they are added to the editor mode pass system
    //! (first in, first rendered) but it is the responsibility of the child classes themselves to enable and disable
    //! themselves as per the editor state, as well as handling their own mutal exclusivity (if any).
    class EditorStateParentPassBase
    {
    public:
        //! Constructs the editor state parent pass with the specified pass chain and mask draw list and connects
        //! it to the tick bus.
        //! @param stateName Name for this editor mode stat.
        //! @param passDescriptorList List of child passes to create, in order of appearance.
        //! @param maskDrawList The entity mask to use for this state.
        EditorStateParentPassBase(
            const AZStd::string& stateName,
            const PassDescriptorList& childPassDescriptorList,
            const AZStd::string& maskDrawList);

        //! Delegate constructor for editor state parents that use the default entity mask.
        EditorStateParentPassBase(const AZStd::string& stateName, const PassDescriptorList& childPassDescriptorList);

        virtual ~EditorStateParentPassBase() = default;

        //! Returns the name of this editor state.
        const AZStd::string& GetStateName() const;

        //! Returns the name of the entity mask daw list used this editor state.
        const Name& GetEntityMaskDrawList() const;

        //! Returns the child pass descriptor list for this editor mode state (used by the pass system to construct and configure the
        //! child passes state and routing).
        const PassDescriptorList& GetChildPassDescriptorList() const;

        //! Returns true of this editor mode state is to be enabled, otherwise false.
        virtual bool IsEnabled() const { return true; }

        //! Returns the entities that should be rendered to the entity mask for this editor state.
        [[nodiscard]] virtual AzToolsFramework::EntityIdList GetMaskedEntities() const = 0;

        Name GetPassTemplateName() const
        {
            return Name(GetStateName() + "Template");
        }

        Name GetPassName() const
        {
            return Name(GetStateName() + "Pass");
        }

        void AddParentPassForPipeline(const Name& pipelineName, RPI::Ptr<RPI::Pass> parentPass)
        {
            m_parentPasses[pipelineName] = parentPass;
        }

        void UpdateParentPasses()
        {
            for (auto& [pipelineName, parentPass] : m_parentPasses)
            {
                InitPassData(azdynamic_cast<RPI::ParentPass*>(parentPass.get()));
            }
        }

    protected:
        //!
        template<class ChildPass>
        ChildPass* FindChildPass(RPI::ParentPass* parentPass, std::size_t index) const
        {
            if (index >= m_childPassDescriptorList.size())
            {
                AZ_Error("EditorStateParentPassBase", false, "Couldn't find child pass");
                return 0;
            }

            auto* childPass = parentPass->FindChildPass(m_childPassDescriptorList[index]);
            if (!ChildPass)
            {
                return nullptr;
            }

            return dynamic_cast<ChildPass*>(childPass);
        }

        //! Opportunity for the editor mode state to initialize any child pass object state.
        virtual void InitPassData(RPI::ParentPass*){}
    private:

        AZStd::string m_stateName;
        PassDescriptorList m_childPassDescriptorList;
        Name m_entityMaskDrawList;
        AZStd::unordered_map<Name, RPI::Ptr<RPI::Pass>> m_parentPasses;
    };
} // namespace AZ::Render

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EditorModeFeedback/EditorStateRequestsBus.h>

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
    using PassNameList = AZStd::vector<Name>;

    //! Parent pass for editor states.
    //! This base class is inherited by the specific editor states that wish to implement custom feedback effects.
    //! When a child class of this base class is constructed, the render passes in the pass descriptor list are
    //! constructed and added to the render pipeline. The ordering of the corresponding parent passes in the render
    //! pipeline for these passes is determined by the order in which they are added to the editor mode pass system
    //! (first in, first rendered) but it is the responsibility of the child classes themselves to enable and disable
    //! themselves as per the editor state, as well as handling their own mutal exclusivity (if any).
    class EditorStateBase
        : private EditorStateRequestsBus::Handler
    {
    public:
        //! Constructs the editor state effect pass with the specified pass chain and mask draw list and connects
        //! it to the tick bus.
        //! @param editorState The editor state enumeration to associate with this pass.
        //! @param stateName Name for this editor mode stat.
        //! @param passNameList List of child passes to create, in order of appearance.
        //! @param maskDrawList The entity mask to use for this state.
        EditorStateBase(
            EditorState editorState,
            const AZStd::string& stateName,
            const PassNameList& childPassNameList,
            const AZStd::string& maskDrawList);

        //! Delegate constructor for editor state parents that use the default entity mask.
        EditorStateBase(
            EditorState editorState,
            const AZStd::string& stateName,
            const PassNameList& childPassNameList);

        virtual ~EditorStateBase();

        //! Returns the entities that should be rendered to the entity mask for this editor state.
        [[nodiscard]] virtual AzToolsFramework::EntityIdList GetMaskedEntities() const = 0;

        //! Returns the name of this editor state.
        const AZStd::string& GetStateName() const;

        //! Returns the name of the entity mask draw list used this editor state.
        const Name& GetEntityMaskDrawList() const;

        //! Returns the child pass descriptor list for this editor mode state (used by the pass system to construct and configure the
        //! child passes state and routing).
        const PassNameList& GetChildPassNameList() const;

        //! Returns true of this editor mode state is to be enabled, otherwise false.
        virtual bool IsEnabled() const { return m_enabled; }

        //! Returns the pass template name for this editor state effect pass.
        Name GetPassTemplateName() const;

        //! Returns the pass name of this editor state effect pass.
        Name GetPassName() const;

        //! Adds the pass class pointer for this pass for the specified pipeline.
        void AddParentPassForPipeline(const Name& pipelineName, RPI::Ptr<RPI::Pass> parentPass);

        //! Removes the pass class pointer for this pass for the specified pipeline.
        void RemoveParentPassForPipeline(const Name& pipelineName);

        //! Calls the update method for each pipeline this editor state effect pass is part of.
        void UpdatePassDataForPipelines();

        // EditorStateRequestsBus overrides ...
        void SetEnabled(bool enabled) override;

        //! Returns the generated name for the specified child pass.
        Name GetGeneratedChildPassName(size_t index) const;

    protected:
        //! Helper function for finding the specified child effect pass for this editor state effect pass.
        template<class ChildPass>
        ChildPass* FindChildPass(RPI::ParentPass* parentPass, size_t index) const
        {
            if (index >= m_childPassNameList.size())
            {
                AZ_Error("EditorStateBase", false, "Couldn't find child pass");
                return nullptr;
            }

            const auto childPassName = GetGeneratedChildPassName(index);
            auto childPass = parentPass->FindChildPass(childPassName);

            if (!childPass)
            {
                return nullptr;
            }

            return dynamic_cast<ChildPass*>(childPass.get());
        }

        //! Opportunity for the editor mode state to initialize any child pass object state.
        virtual void UpdatePassData(RPI::ParentPass*) {}
        
    private:
        EditorState m_state; //!< The editor state enumeration this editor state effect pass is associated with.
        AZStd::string m_stateName; //!< The name of this state.
        bool m_enabled = true; //!< True if this pass is enabled, otherwise false.
        PassNameList m_childPassNameList; //!< The child passes that compose this editor state effect pass.
        Name m_entityMaskDrawList; //!< The draw list of the mask this pass uses.

        //! The parent pass class instances for this editor state effect pass for each pipeline this pass is added to.
        AZStd::unordered_map<Name, RPI::Ptr<RPI::Pass>> m_parentPasses;
    };
} // namespace AZ::Render

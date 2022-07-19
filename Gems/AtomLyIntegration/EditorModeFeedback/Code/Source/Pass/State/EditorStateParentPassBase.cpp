/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateParentPassBase.h>

namespace AZ::Render
{
    // Name of the default mask for entities of interest
    static constexpr const char* const DefaultEntityMaskName = "editormodeinterestmask";

    EditorStateParentPassBase::EditorStateParentPassBase(
        EditorState editorState,
        const AZStd::string& stateName,
        const PassDescriptorList& childPassDescriptorList,
        const AZStd::string& maskDrawList)
        : m_state(editorState)
        , m_stateName(stateName)
        , m_childPassDescriptorList(childPassDescriptorList)
        , m_entityMaskDrawList(maskDrawList)
    {
        EditorStateRequestsBus::Handler::BusConnect(m_state);
    }

    EditorStateParentPassBase::EditorStateParentPassBase(
        EditorState editorState,
        const AZStd::string& stateName,
        const PassDescriptorList& childPassDescriptorList)
        : EditorStateParentPassBase(editorState, stateName, childPassDescriptorList, DefaultEntityMaskName)
    {
    }

    EditorStateParentPassBase::~EditorStateParentPassBase()
    {
        EditorStateRequestsBus::Handler::BusDisconnect();
    }

    const AZStd::string& EditorStateParentPassBase::GetStateName() const
    {
        return m_stateName;
    }

    const Name& EditorStateParentPassBase::GetEntityMaskDrawList() const
    {
        return m_entityMaskDrawList;
    }

    const PassDescriptorList& EditorStateParentPassBase::GetChildPassDescriptorList() const
    {
        return m_childPassDescriptorList;
    }

    Name EditorStateParentPassBase::GetPassTemplateName() const
    {
        return Name(GetStateName() + "Template");
    }

    Name EditorStateParentPassBase::GetPassName() const
    {
        return Name(GetStateName() + "Pass");
    }

    void EditorStateParentPassBase::AddParentPassForPipeline(const Name& pipelineName, RPI::Ptr<RPI::Pass> parentPass)
    {
        m_parentPasses[pipelineName] = parentPass;
    }

    void EditorStateParentPassBase::UpdatePassDataForPipelines()
    {
        for (auto& [pipelineName, parentPass] : m_parentPasses)
        {
            UpdatePassData(azdynamic_cast<RPI::ParentPass*>(parentPass.get()));
        }
    }

    void EditorStateParentPassBase::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }
} // namespace AZ::Render

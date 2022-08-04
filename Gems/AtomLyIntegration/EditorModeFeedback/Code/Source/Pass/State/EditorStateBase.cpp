/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBase.h>

namespace AZ::Render
{
    // Name of the default mask for entities of interest
    static constexpr const char* const DefaultEntityMaskName = "editormodeinterestmask";

    EditorStateBase::EditorStateBase(
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

    EditorStateBase::EditorStateBase(
        EditorState editorState,
        const AZStd::string& stateName,
        const PassDescriptorList& childPassDescriptorList)
        : EditorStateBase(editorState, stateName, childPassDescriptorList, DefaultEntityMaskName)
    {
    }

    EditorStateBase::~EditorStateBase()
    {
        EditorStateRequestsBus::Handler::BusDisconnect();
    }

    const AZStd::string& EditorStateBase::GetStateName() const
    {
        return m_stateName;
    }

    const Name& EditorStateBase::GetEntityMaskDrawList() const
    {
        return m_entityMaskDrawList;
    }

    const PassDescriptorList& EditorStateBase::GetChildPassDescriptorList() const
    {
        return m_childPassDescriptorList;
    }

    Name EditorStateBase::GetPassTemplateName() const
    {
        return Name(GetStateName() + "Template");
    }

    Name EditorStateBase::GetPassName() const
    {
        return Name(GetStateName() + "Pass");
    }

    void EditorStateBase::AddParentPassForPipeline(const Name& pipelineName, RPI::Ptr<RPI::Pass> parentPass)
    {
        m_parentPasses[pipelineName] = parentPass;
    }

    void EditorStateBase::UpdatePassDataForPipelines()
    {
        for (auto& [pipelineName, parentPass] : m_parentPasses)
        {
            UpdatePassData(azdynamic_cast<RPI::ParentPass*>(parentPass.get()));
        }
    }

    void EditorStateBase::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }
} // namespace AZ::Render

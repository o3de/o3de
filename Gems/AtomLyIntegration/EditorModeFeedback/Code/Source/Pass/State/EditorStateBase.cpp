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
        const PassNameList& childPassNameList,
        const AZStd::string& maskDrawList)
        : m_state(editorState)
        , m_stateName(stateName)
        , m_childPassNameList(childPassNameList)
        , m_entityMaskDrawList(maskDrawList)
    {
        EditorStateRequestsBus::Handler::BusConnect(m_state);
    }

    EditorStateBase::EditorStateBase(
        EditorState editorState,
        const AZStd::string& stateName,
        const PassNameList& childPassNameList)
        : EditorStateBase(editorState, stateName, childPassNameList, DefaultEntityMaskName)
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

    const PassNameList& EditorStateBase::GetChildPassNameList() const
    {
        return m_childPassNameList;
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

    void EditorStateBase::RemoveParentPassForPipeline(const Name& pipelineName)
    {
        m_parentPasses.erase(pipelineName);
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

    Name EditorStateBase::GetGeneratedChildPassName(size_t index) const
    {
        if (index >= m_childPassNameList.size())
        {
            AZ_Error("EditorStateBase", false, "Couldn't retrieve child pass name for index %zu", index);
            return Name("");
        }

        return Name(
            AZStd::string::format("%sChildPass%zu_%s", GetPassTemplateName().GetCStr(), index, m_childPassNameList[index].GetCStr()));
    }
} // namespace AZ::Render

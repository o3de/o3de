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
        const AZStd::string& stateName,
        const PassDescriptorList& childPassDescriptorList,
        const AZStd::string& maskDrawList)
        : m_stateName(stateName)
        , m_childPassDescriptorList(childPassDescriptorList)
        , m_entityMaskDrawList(maskDrawList)
    {
    }

    EditorStateParentPassBase::EditorStateParentPassBase(
        const AZStd::string& stateName, const PassDescriptorList& childPassDescriptorList)
        : EditorStateParentPassBase(stateName, childPassDescriptorList, DefaultEntityMaskName)
    {
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
} // namespace AZ::Render

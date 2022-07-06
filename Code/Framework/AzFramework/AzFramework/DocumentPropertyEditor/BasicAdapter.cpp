
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/BasicAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    void BasicAdapter::SetContents(Dom::Value contents)
    {
        m_value = contents;
        NotifyResetDocument();
    }

    Dom::Value BasicAdapter::GenerateContents()
    {
        return m_value;
    }

    Dom::PatchOutcome BasicAdapter::RequestContentChange(const Dom::Patch& patch)
    {
        Dom::PatchOutcome result = patch.ApplyInPlace(m_value);
        NotifyContentsChanged(patch);
        return result;
    }
} // namespace AZ::DocumentPropertyEditor

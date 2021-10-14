/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasSourceFileHandle.h>
#include <Editor/Include/ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>

namespace ScriptCanvasEditor
{
    SourceHandle::SourceHandle(ScriptCanvas::DataPtr graph, const AZ::Uuid& id, AZStd::string_view path)
        : m_data(graph)
        , m_id(id)
        , m_path(path)
    {}

    void SourceHandle::Clear()
    {
        m_data = nullptr;
        m_id = AZ::Uuid::CreateNull();
        m_path.clear();
    }

    GraphPtrConst SourceHandle::Get() const
    {
        return m_data ? m_data->GetEditorGraph() : nullptr;
    }

    const AZ::Uuid& SourceHandle::Id() const
    {
        return m_id;
    }

    bool SourceHandle::IsValid() const
    {
        return *this;
    }

    GraphPtr SourceHandle::Mod() const
    {
        return m_data ? m_data->ModEditorGraph() : nullptr;
    }

    SourceHandle::operator bool() const
    {
        return m_data != nullptr;
    }

    bool SourceHandle::operator!() const
    {
        return m_data == nullptr;
    }

    const AZStd::string& SourceHandle::Path() const
    {
        return m_path;
    }
}

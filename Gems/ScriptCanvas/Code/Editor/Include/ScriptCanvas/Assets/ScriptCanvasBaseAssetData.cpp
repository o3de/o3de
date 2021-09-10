/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>
#include <ScriptCanvas/Components/EditorGraph.h>

namespace ScriptCanvas
{
    Graph* ScriptCanvasData::ModGraph()
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(m_scriptCanvasEntity.get());
    }

    const Graph* ScriptCanvasData::GetGraph() const
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(m_scriptCanvasEntity.get());
    }
}

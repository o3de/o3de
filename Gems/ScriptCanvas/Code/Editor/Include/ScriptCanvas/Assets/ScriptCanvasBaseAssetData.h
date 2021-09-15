/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Core/GraphData.h>

namespace ScriptCanvas
{
    class ScriptCanvasData
    {
    public:

        AZ_RTTI(ScriptCanvasData, "{1072E894-0C67-4091-8B64-F7DB324AD13C}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasData, AZ::SystemAllocator, 0);
        ScriptCanvasData() {}
        virtual ~ScriptCanvasData() {}
        ScriptCanvasData(ScriptCanvasData&& other);
        ScriptCanvasData& operator=(ScriptCanvasData&& other);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZ::Entity* GetScriptCanvasEntity() const { return m_scriptCanvasEntity.get(); }

        Graph* ModGraph();

        const Graph* GetGraph() const;

        AZStd::unique_ptr<AZ::Entity> m_scriptCanvasEntity;
    private:
        ScriptCanvasData(const ScriptCanvasData&) = delete;
    };
}

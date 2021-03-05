/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        AZStd::unique_ptr<AZ::Entity> m_scriptCanvasEntity;
    private:
        ScriptCanvasData(const ScriptCanvasData&) = delete;
    };
}
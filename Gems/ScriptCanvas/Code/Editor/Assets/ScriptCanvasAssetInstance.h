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
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <Editor/Assets/ScriptCanvasAssetReference.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetInstance
    {
    public:
        AZ_TYPE_INFO(ScriptCanvasAssetInstance, "{96B16AAB-DB63-4D32-9FC9-7A5DE440B0B7}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetInstance, AZ::SystemAllocator, 0);

        ScriptCanvasAssetInstance() = default;
        ~ScriptCanvasAssetInstance();
        static void Reflect(AZ::ReflectContext* context);

        const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& GetBaseToInstanceMap() const { return m_baseToInstanceMap; }
        ScriptCanvasAssetReference& GetReference();
        const ScriptCanvasAssetReference& GetReference() const;

        ScriptCanvas::ScriptCanvasData& GetScriptCanvasData();
        const ScriptCanvas::ScriptCanvasData& GetScriptCanvasData() const;

        void ComputeDataPatch();
        void ApplyDataPatch();

    private:
        ScriptCanvasAssetInstance(const ScriptCanvasAssetInstance&) = delete;
        AZ::DataPatch::FlagsMap GetDataFlagsForPatching() const;

        ScriptCanvas::ScriptCanvasData m_scriptCanvasData;
        ScriptCanvasAssetReference m_assetRef;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_baseToInstanceMap;
        AZStd::unordered_map<AZ::EntityId, AZ::DataPatch::FlagsMap> m_entityToDataFlags;

        AZ::DataPatch m_dataPatch;
        bool m_canApplyPatch;
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

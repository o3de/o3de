/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Core/Core.h>

class ScriptCanvasAsset;


namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        /* source file description
        ScriptCanvasAssetDescription()
            : ScriptCanvas::AssetDescription(
                "{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}", // type
                "Script Canvas", // name 
                "Script Canvas Graph Asset", // description
                "@projectroot@/scriptcanvas", // suggested save path
                ".scriptcanvas", // fileExtention
                "Script Canvas", // group
                "Untitled-%i", // asset name pattern
                "Script Canvas Files (*.scriptcanvas)", // file filter
                "Script Canvas", // asset type display name
                "Script Canvas", // entity name
                "Icons/ScriptCanvas/Viewport/ScriptCanvas.png", // icon path
                AZ::Color(0.321f, 0.302f, 0.164f, 1.0f), // display color
                true // is editable type
            )
        {}

         AssetDescription(   AZ::Data::AssetType assetType,
                            const char* name,
                            const char* description,
                            const char* suggestedSavePath,
                            const char* fileExtension,
                            const char* group,
                            const char* assetNamePattern,
                            const char* fileFilter,
                            const char* assetTypeDisplayName,
                            const char* entityName,
                            const char* iconPath,
                            AZ::Color displayColor,
                            bool isEditableType)
        */
        class ScriptCanvasAssetHolder
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator, 0);

            ScriptCanvasAssetHolder() = default;
            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;
        };

        class ScriptCanvasAsset
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAsset, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAsset, AZ::SystemAllocator, 0);

            ScriptCanvasAsset() = default;
            static void Reflect(AZ::ReflectContext * context);
        };
    }
}

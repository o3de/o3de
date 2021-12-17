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
#include <AzCore/Asset/AssetCommon.h>

namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        // only used as a pass-through to loading a guid / hint during version conversion
        class ScriptCanvasAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAsset, "{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAsset, AZ::SystemAllocator, 0);

            ScriptCanvasAsset() = default;
        };

        // only used as a pass-through to loading a guid / hint during version conversion
        class ScriptCanvasAssetHolder
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;

            ScriptCanvasAssetHolder() = default;
        };

    }
}

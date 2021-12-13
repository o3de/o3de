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
        class ScriptCanvasAssetHolder
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator, 0);

            ScriptCanvasAssetHolder() = default;
            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;
        };
    }
}

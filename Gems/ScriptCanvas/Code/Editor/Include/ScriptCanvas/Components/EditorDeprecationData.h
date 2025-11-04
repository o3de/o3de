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

/// <summary>
/// This file is for deprecated Editor time / source data that is none-the-less used to assist developers writing code
/// that can load and properly update legacy source files for ScriptCanvas.
///
/// \note Runtime data should *never* be versioned. The serialized Assets should always be reflect the latest version of the
/// serialized C++ definitions, and when an update is required, the appropriate builder is bumped to trigger teh AP to regenerate all files
/// from their sources.
/// </summary>

namespace ScriptCanvasEditor
{
    using SourceHandle = SourceHandle;

    namespace Deprecated
    {
        class EditorScriptCanvasComponentVersionConverter
        {
        public:
            static bool Convert(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
        };  

        // only used as a pass-through to loading a guid / hint during version conversion
        class ScriptCanvasAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAsset, "{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAsset, AZ::SystemAllocator);

            ScriptCanvasAsset() = default;
        };

        // only used as a pass-through to loading a guid / hint during version conversion
        class ScriptCanvasAssetHolder
        {
        public:
            AZ_TYPE_INFO(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
            AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;

            ScriptCanvasAssetHolder() = default;
        };

    }
}

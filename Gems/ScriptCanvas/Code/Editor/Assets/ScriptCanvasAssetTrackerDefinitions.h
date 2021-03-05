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

#include <AzCore/std/function/function_base.h>
#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasMemoryAsset;

    namespace Callbacks
    {
        //! Callback used to know when a save operation failed or succeeded
        using OnSave = AZStd::function<void(bool saveSuccess, AZ::Data::Asset<ScriptCanvas::ScriptCanvasAssetBase>, AZ::Data::AssetId previousFileAssetId)>;

        using OnAssetReadyCallback = AZStd::function<void(ScriptCanvasMemoryAsset&)>;
        using OnAssetCreatedCallback = OnAssetReadyCallback;
    }

    namespace Tracker
    {
        enum class ScriptCanvasFileState : AZ::s32
        {
            NEW,
            MODIFIED,
            UNMODIFIED,
            INVALID = -1
        };
    }

}

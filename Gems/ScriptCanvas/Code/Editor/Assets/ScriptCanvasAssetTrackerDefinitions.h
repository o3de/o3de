/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        using OnSave = AZStd::function<void(bool saveSuccess, const SourceHandle& attempt, const SourceHandle& previous)>;

        using OnAssetReadyCallback = AZStd::function<void(ScriptCanvasMemoryAsset&)>;
        using OnAssetCreatedCallback = OnAssetReadyCallback;
    }


}

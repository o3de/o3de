/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Components/EditorDeprecationData.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        void ScriptCanvasAssetHolder::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptCanvasAssetHolder>()
                    ->Version(1)
                    ->Field("m_asset", &ScriptCanvasAssetHolder::m_scriptCanvasAsset)
                    ;
            }
        }
    }
}

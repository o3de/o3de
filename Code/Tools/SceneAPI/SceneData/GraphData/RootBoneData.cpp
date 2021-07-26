/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            void RootBoneData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {

                    serializeContext->Class<RootBoneData, BoneData>()->Version(1);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<RootBoneData>("Root Bone data", "First bone in the skeletal hierarchy.");
                    }
                }
            }
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            void BoneData::SetWorldTransform(const SceneAPI::DataTypes::MatrixType& transform)
            {
                m_worldTransform = transform;
            }

            const SceneAPI::DataTypes::MatrixType& BoneData::GetWorldTransform() const
            {
                return m_worldTransform;
            }

            void BoneData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BoneData>()->Version(1)
                        ->Field("worldTransform", &BoneData::m_worldTransform);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<BoneData>("Bone data", "Data this individual bone contributes to the overall skeleton.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &BoneData::m_worldTransform, "World", "World transform this bone contributes to the overall skeleton.");
                    }
                }
            }
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ

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

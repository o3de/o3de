/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            TransformData::TransformData(const SceneAPI::DataTypes::MatrixType& transform)
                : m_transform(transform)
            {
            }

            void TransformData::SetMatrix(const SceneAPI::DataTypes::MatrixType& transform)
            {
                m_transform = transform;
            }

            SceneAPI::DataTypes::MatrixType& TransformData::GetMatrix()
            {
                return m_transform;
            }

            const SceneAPI::DataTypes::MatrixType& TransformData::GetMatrix() const
            {
                return m_transform;
            }

            void TransformData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<TransformData>()->Version(1)
                        ->Field("transform", &TransformData::m_transform);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<TransformData>("Transform", "Transform matrix applied as a node or as a child.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &TransformData::m_transform, "", "Transform matrix applied as a node or as a child.");
                    }
                }
            }
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ

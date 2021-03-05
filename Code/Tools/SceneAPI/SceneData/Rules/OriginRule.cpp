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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/OriginRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            const AZStd::string OriginRule::c_defaultWorldUIString = "World";

            AZ_CLASS_ALLOCATOR_IMPL(OriginRule, SystemAllocator, 0)

            OriginRule::OriginRule()
                : m_rotation(Quaternion::CreateIdentity())
                , m_translation(Vector3::CreateZero())
                , m_scale(1.0f)
            {
            }

            const AZStd::string& OriginRule::GetOriginNodeName() const
            {
                return m_originNodeName;
            }

            bool OriginRule::UseRootAsOrigin() const
            {
                return m_originNodeName == c_defaultWorldUIString;
            }

            const Quaternion& OriginRule::GetRotation() const
            {
                return m_rotation;
            }

            const Vector3& OriginRule::GetTranslation() const
            {
                return m_translation;
            }

            float OriginRule::GetScale() const
            {
                return m_scale;
            }

            void  OriginRule::SetOriginNodeName(const AZStd::string& originNodeName)
            {
                m_originNodeName = originNodeName;
            }

            void  OriginRule::SetRotation(const Quaternion& rotation)
            {
                m_rotation = rotation;
            }

            void  OriginRule::SetTranslation(const Vector3& translation)
            {
                m_translation = translation;
            }

            void  OriginRule::SetScale(float scale)
            {
                m_scale = scale;
            }

            void OriginRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }
                
                serializeContext->Class<OriginRule, DataTypes::IOriginRule>()->Version(1)
                    ->Field("originNodeName", &OriginRule::m_originNodeName)
                    ->Field("translation", &OriginRule::m_translation)
                    ->Field("rotation", &OriginRule::m_rotation)
                    ->Field("scale", &OriginRule::m_scale);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<OriginRule>("Origin", "Configure where the mesh will load relative to world origin.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement("NodeListSelection", &OriginRule::m_originNodeName, "Relative Origin Node",
                            "Select a Node from the scene as the origin for this export. 'World' will export from the Root Scene Node.")
                            ->Attribute("DisabledOption", c_defaultWorldUIString)
                            ->Attribute("DefaultToDisabled", true)
                            ->Attribute("ExcludeEndPoints", true)
                        ->DataElement(Edit::UIHandlers::Default, &OriginRule::m_translation, "Translation", "Moves the group along the given vector.")
                        ->DataElement(Edit::UIHandlers::Default, &OriginRule::m_rotation, "Rotation", "Rotates the group after translation.")
                            ->Attribute(Edit::Attributes::LabelForX, "P")
                            ->Attribute(Edit::Attributes::LabelForY, "R")
                            ->Attribute(Edit::Attributes::LabelForZ, "Y")
                        ->DataElement(Edit::UIHandlers::Default, &OriginRule::m_scale, "Scale", "Scales the group up or down after translation and rotation.")
                            ->Attribute(Edit::Attributes::Min, 0.0001)
                            ->Attribute(Edit::Attributes::Max, 1000.0);
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ

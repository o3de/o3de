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
#include "TouchBending_precompiled.h"
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>

#include <Pipeline/TouchBendingRule.h>

namespace TouchBending
{
    namespace Pipeline
    {
        AZ_CLASS_ALLOCATOR_IMPL(TouchBendingRule, AZ::SystemAllocator, 0);

        static const float DEFAULT_STIFFNESS = 0.5f;
        static const float DEFAULT_DAMPING = 0.5f;
        static const float DEFAULT_THICKNESS = 0.01f;

        TouchBendingRule::TouchBendingRule() :
            m_stiffness(DEFAULT_STIFFNESS),
            m_damping(DEFAULT_DAMPING),
            m_thickness(DEFAULT_THICKNESS)
        {
        }

        void TouchBendingRule::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<ITouchBendingRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

            serializeContext->Class<TouchBendingRule, ITouchBendingRule>()->Version(1)
                ->Field("rootBoneName", &TouchBendingRule::m_rootBoneName)
                ->Field("proximityTriggerMeshes", &TouchBendingRule::m_proximityTriggerMeshes)
                ->Field("stiffness", &TouchBendingRule::m_stiffness)
                ->Field("damping", &TouchBendingRule::m_damping)
                ->Field("thickness", &TouchBendingRule::m_thickness);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<TouchBendingRule>("TouchBending", "Adds skinning data to the exported CGF asset. The skinning data will be used for touch bending simulation. A NoCollide physics MTL file is always generated.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->DataElement("NodeListSelection", &TouchBendingRule::m_rootBoneName, "Select root bone", "The root bone of the touch bendable mesh.")
                    ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingRule::m_proximityTriggerMeshes, "Proximity Trigger Mesh(es)", "Provides collision volume(s) for triggering touch bending. Each additional mesh added reduces performance. A new PhysicsNoDraw SubMaterial of type NoCollide will be created for the Mesh(es).")
                    ->Attribute("FilterName", "proximity meshes")
                    ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingRule::m_stiffness, "Stiffness", "Stiffness of all branches.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingRule::m_damping, "Damping", "Damping of all branches.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TouchBendingRule::m_thickness, "Thickness[m]", "Thickness of all branches, in meters. Interpreted as the radius of a cylinder.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.00001f);
            }
        }

    } // Pipeline
} // TouchBending
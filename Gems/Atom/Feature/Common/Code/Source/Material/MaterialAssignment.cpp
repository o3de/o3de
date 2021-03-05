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

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void MaterialAssignment::Reflect(ReflectContext* context)
        {
            MaterialAssignmentId::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialAssignmentMap>();
                serializeContext->RegisterGenericType<MaterialPropertyOverrideMap>();

                serializeContext->Class<MaterialAssignment>()
                    ->Version(1)
                    ->Field("MaterialAsset", &MaterialAssignment::m_materialAsset)
                    ->Field("PropertyOverrides", &MaterialAssignment::m_propertyOverrides)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MaterialAssignment>("MaterialAssignment")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const MaterialAssignment&>()
                    ->Constructor<const AZ::Data::AssetId&>()
                    ->Constructor<const Data::Asset<RPI::MaterialAsset>&>()
                    ->Constructor<const Data::Asset<RPI::MaterialAsset>&, const Data::Instance<RPI::Material>&>()
                    ->Method("ToString", &MaterialAssignment::ToString)
                    ->Property("materialAsset", BehaviorValueProperty(&MaterialAssignment::m_materialAsset))
                    ->Property("propertyOverrides", BehaviorValueProperty(&MaterialAssignment::m_propertyOverrides))
                    ;

                behaviorContext->ConstantProperty("DefaultMaterialAssignment", BehaviorConstant(DefaultMaterialAssignment))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->ConstantProperty("DefaultMaterialAssignmentId", BehaviorConstant(DefaultMaterialAssignmentId))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->ConstantProperty("DefaultMaterialAssignmentMap", BehaviorConstant(DefaultMaterialAssignmentMap))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

            }
        }
    } // namespace Render
} // namespace AZ

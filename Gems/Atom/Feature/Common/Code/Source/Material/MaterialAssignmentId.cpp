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

#include <Atom/Feature/Material/MaterialAssignmentId.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void MaterialAssignmentId::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialAssignmentId>()
                    ->Version(1)
                    ->Field("lodIndex", &MaterialAssignmentId::m_lodIndex)
                    ->Field("materialAssetId", &MaterialAssignmentId::m_materialAssetId)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MaterialAssignmentId>("MaterialAssignmentId")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const MaterialAssignmentId&>()
                    ->Constructor<MaterialAssignmentLodIndex, AZ::Data::AssetId>()
                    ->Method("IsDefault", &MaterialAssignmentId::IsDefault)
                    ->Method("IsAssetOnly", &MaterialAssignmentId::IsAssetOnly)
                    ->Method("IsLodAndAsset", &MaterialAssignmentId::IsLodAndAsset)
                    ->Method("ToString", &MaterialAssignmentId::ToString)
                    ->Property("lodIndex", BehaviorValueProperty(&MaterialAssignmentId::m_lodIndex))
                    ->Property("materialAssetId", BehaviorValueProperty(&MaterialAssignmentId::m_materialAssetId))
                    ;
            }
        }
    } // namespace Render
} // namespace AZ

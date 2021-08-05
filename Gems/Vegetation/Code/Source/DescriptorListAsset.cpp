/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Vegetation/DescriptorListAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Vegetation
{
    void DescriptorListAsset::Reflect(AZ::ReflectContext* context)
    {
        // Ensure that Descriptor has been reflected since we reference it in the ElementAttribute for NameLabelOverride
        Descriptor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorListAsset>()
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Version(0)
                ->Field("Descriptors", &DescriptorListAsset::m_descriptors)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DescriptorListAsset>(
                    "Vegetation Descriptor List", "Contains an ordered list of vegetation descriptors used to create instances")
                    ->DataElement(0, &DescriptorListAsset::m_descriptors, "Assets", "Set of vegetation descriptors.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, &Descriptor::GetDescriptorName)
                    ->ElementAttribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DescriptorListAsset>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Property("descriptors", BehaviorValueProperty(&DescriptorListAsset::m_descriptors))
                ;
        }
    }
}


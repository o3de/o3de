/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/RHISystemDescriptor.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace RHI
    {
        void RHISystemDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RHISystemDescriptor>()
                    ->Version(4)
                    ->Field("DrawItemTags", &RHISystemDescriptor::m_drawListTags)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<RHISystemDescriptor>("RHI Settings", "Settings for runtime RHI system")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RHISystemDescriptor::m_drawListTags, "Draw List Tags", "The set of globally declared draw list tags, which will be registered with the registry at startup.")
                        ;
                }
            }
        }
    } // namespace RPI
} // namespace AZ

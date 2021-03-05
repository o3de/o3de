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


#include "Core.h"
#include "Attributes.h"
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/AttributeReader.h>

namespace ScriptCanvas
{
    

    static bool SlotIdVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        //! Version 1: Slot Ids contained a Crc32 hash of the name given
        //! Version 2+: Slot Ids now contain a random Uuid
        //! The converter below reads in the old GraphCanvas node <-> ScriptCanvas node map and then iterates over all the 
        //! GraphCanvas nodes adding a reference to the ScriptCanvas node in it's user data field
        if (classElement.GetVersion() <= 1)
        {
            if (!classElement.RemoveElementByName(AZ_CRC("m_id", 0x7108ece0)))
            {
                return false;
            }

            if (classElement.RemoveElementByName(AZ_CRC("m_name", 0xc08c4427)))
            {
                return false;
            }

            classElement.AddElementWithData(context, "m_id", AZ::Uuid::CreateRandom());
        }

        return true;
    }

    void SlotId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SlotId>()
                ->Version(2, &SlotIdVersionConverter)
                ->Field("m_id", &SlotId::m_id)
                ;
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SlotId>("SlotId")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
        }
    }
}


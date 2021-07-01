/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Transform.h>
#include <Blast/BlastActor.h>

namespace Blast
{
    //! Data exposed through script canvas as a notification from BlastFamilyComponentNotificationBus.
    struct BlastActorData
    {
        AZ_TYPE_INFO(BlastActorData, "{A23453D5-79A8-49C8-B9F0-9CC35D711DD4}");

        BlastActorData() = default;
        explicit BlastActorData(const BlastActor& blastActor)
            : m_isStatic(blastActor.IsStatic())
            , m_entityId(blastActor.GetEntity()->GetId())
        {
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastActorData>()
                    ->Version(1)
                    ->Field("EntityId", &BlastActorData::m_entityId)
                    ->Field("IsStatic", &BlastActorData::m_isStatic);

                serializeContext->RegisterGenericType<AZStd::vector<BlastActorData>>();

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext
                        ->Class<BlastActorData>("Blast Actor Data", "Represents Blast Actor in a Script Canvas friendly format.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &BlastActorData::m_isStatic)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &BlastActorData::m_entityId);
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<BlastActorData>("BlastActorData")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Property("EntityId", BehaviorValueProperty(&BlastActorData::m_entityId))
                    ->Property("IsStatic", BehaviorValueProperty(&BlastActorData::m_isStatic));
            }
        }

        bool m_isStatic;
        AZ::EntityId m_entityId;
    };
} // namespace Blast

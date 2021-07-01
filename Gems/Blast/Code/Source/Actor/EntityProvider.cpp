/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StdAfx.h"

#include <Actor/EntityProvider.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace Blast
{
    AZStd::shared_ptr<EntityProvider> EntityProvider::Create()
    {
        return AZStd::make_shared<EntityProviderImpl>();
    }

    AZStd::shared_ptr<AZ::Entity> EntityProviderImpl::CreateEntity(const AZStd::vector<AZ::Uuid>& componentIds)
    {
        auto entity = AZStd::make_shared<AZ::Entity>();
        for (auto componentId : componentIds)
        {
            if (!entity->CreateComponent(componentId))
            {
                return nullptr;
            }
        }
        return entity;
    }
} // namespace Blast

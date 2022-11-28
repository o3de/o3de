/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MissingAssetNotificationHandler.h"

namespace AzToolsFramework
{
    void MissingAssetNotificationHandler::FileMissing(const char* filePath)
    {
        Call(FN_FileMissing, filePath);
    }

    void MissingAssetNotificationHandler::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzFramework::MissingAssetNotificationBus>("MissingAssetNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "asset")
                ->Attribute(AZ::Script::Attributes::Module, "asset")
                ->Handler<MissingAssetNotificationHandler>();
        }
    }

} // namespace AzToolsFramework

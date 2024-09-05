/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Slice/SliceRequestComponent.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>

namespace AzToolsFramework
{
    void SliceRequestComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SliceRequestComponent, AZ::Component>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SliceRequestBus>("SliceRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Slice")
                ->Attribute(AZ::Script::Attributes::Module, "slice")
                ->Event("IsSliceDynamic", &SliceRequests::IsSliceDynamic)
                ->Event("SetSliceDynamic", &SliceRequests::SetSliceDynamic)
                ;
        }
    }

    void SliceRequestComponent::Activate()
    {
        SliceRequestBus::Handler::BusConnect();
    }

    void SliceRequestComponent::Deactivate()
    {
        SliceRequestBus::Handler::BusDisconnect();
    }

    bool SliceRequestComponent::IsSliceDynamic(const AZ::Data::AssetId& assetId)
    {
        return SliceUtilities::IsDynamic(assetId);
    }

    void SliceRequestComponent::SetSliceDynamic(const AZ::Data::AssetId& assetId, bool isDynamic)
    {
        SliceUtilities::SetIsDynamic(assetId, isDynamic);
    }
} // namespace AzToolsFramework

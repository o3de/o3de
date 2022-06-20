/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/Translation/TranslationBus.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvasEditor
{
    namespace TranslationHelper
    {
        namespace GlobalKeys
        {
            static constexpr const char* EBusSenderIDKey = "Globals.EBusSenderBusId";
            static constexpr const char* EBusHandlerIDKey = "Globals.EBusHandlerBusId";
            static constexpr const char* MissingFunctionKey = "Globals.MissingFunction";
            static constexpr const char* EBusHandlerOutSlot = "Globals.EBusHandler.OutSlot";
        } // namespace GlobalKeys

        namespace AssetContext
        {
            static constexpr char AZEventContext[] = "AZEventHandler";
            static constexpr char BehaviorClassContext[] = "BehaviorClass";
            static constexpr char BehaviorGlobalMethodContext[] = "BehaviorMethod";
            static constexpr char BehaviorGlobalPropertyContext[] = "Constant";
            static constexpr char CustomNodeContext[] = "ScriptCanvas::Node";
            static constexpr char EBusHandlerContext[] = "EBusHandler";
            static constexpr char EBusSenderContext[] = "EBusSender";
        } // namespace AssetContext

        namespace AssetPath
        {
            static constexpr char AZEventPath[] = "AZEvents";
            static constexpr char BehaviorClassPath[] = "Classes";
            static constexpr char BehaviorGlobalMethodPath[] = "GlobalMethods";
            static constexpr char BehaviorGlobalPropertyPath[] = "Properties";
            static constexpr char CustomNodePath[] = "Nodes";
            static constexpr char EBusHandlerPath[] = "EBus\\Handlers";
            static constexpr char EBusSenderPath[] = "EBus\\Senders";
        } // namespace AssetPath

        inline AZStd::string GetSafeTypeName(ScriptCanvas::Data::Type dataType)
        {
            if (!dataType.IsValid())
            {
                return "";
            }

            AZStd::string azType = dataType.GetAZType().ToString<AZStd::string>();

            GraphCanvas::TranslationKey key;
            key << "BehaviorType" << azType << "details";

            GraphCanvas::TranslationRequests::Details details;

            details.m_name = ScriptCanvas::Data::GetName(dataType);

            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            return details.m_name;
        }
    }
}

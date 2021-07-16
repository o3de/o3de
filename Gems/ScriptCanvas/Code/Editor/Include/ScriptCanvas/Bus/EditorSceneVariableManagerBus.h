/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAbstractItemModel>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>

namespace ScriptCanvasEditor
{
    class EditorSceneVariableManagerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;
        
        // The QCompleter needs this to be non-const for some reason. Shouldn't
        // matter since we should only have have a single instance operating on it at once.
        virtual QAbstractItemModel* GetVariableItemModel() = 0;
    };

    using EditorSceneVariableManagerRequestBus = AZ::EBus<EditorSceneVariableManagerRequests>;
}

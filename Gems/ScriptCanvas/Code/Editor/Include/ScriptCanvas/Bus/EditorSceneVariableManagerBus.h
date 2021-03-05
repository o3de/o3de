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
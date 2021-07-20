/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/EBus/EBus.h>
#include <Editor/Framework/ScriptCanvasReporter.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    /* ScriptCanvasExecutionRequests - Execute a ScriptCanvas script in the Editor */

    class ScriptCanvasExecutionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual Reporter RunGraph(AZStd::string_view path, ScriptCanvas::ExecutionMode mode) = 0;
        virtual Reporter RunAssetGraph(AZ::Data::Asset<AZ::Data::AssetData>, ScriptCanvas::ExecutionMode mode) = 0;
    };
    using ScriptCanvasExecutionBus = AZ::EBus<ScriptCanvasExecutionRequests>;

}

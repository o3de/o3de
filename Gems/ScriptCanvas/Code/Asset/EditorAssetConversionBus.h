/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Translation/Translation.h>

namespace AZ
{
    class ScriptAsset;
}

namespace ScriptCanvas
{
    class RuntimeAsset;
    class SubgraphInterfaceAsset;
}

namespace ScriptCanvasEditor
{
    class EditorAssetConversionBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
      
        virtual AZ::Outcome<ScriptCanvas::Translation::LuaAssetResult, AZStd::string> CreateLuaAsset(const SourceHandle& editAsset, AZStd::string_view graphPathForRawLuaFile) = 0;
        virtual AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(const SourceHandle& editAsset) = 0;
    };

    using EditorAssetConversionBus = AZ::EBus<EditorAssetConversionBusTraits>;
}

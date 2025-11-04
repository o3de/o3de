/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class EditorScriptCanvasComponentSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(EditorScriptCanvasComponentSerializer, "{80B497B3-ABC1-4991-A3C4-047A8CB2C26C}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:
        JsonSerializationResult::Result Load
            ( void* outputValue
            , const Uuid& outputValueTypeId
            , const rapidjson::Value& inputValue
            , JsonDeserializerContext& context) override;
    };
}

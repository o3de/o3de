/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

namespace AZ
{
    class StackedString;
    struct JsonApplyPatchSettings;
    struct JsonCreatePatchSettings;

    class JsonMerger final
    {
        friend class JsonSerialization;

    private:
        //! Implementation of the JSON Patch algorithm: https://tools.ietf.org/html/rfc6902
        static JsonSerializationResult::ResultCode ApplyPatch(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
            JsonApplyPatchSettings& settings);

        //! Function to create JSON Patches: https://tools.ietf.org/html/rfc6902
        static JsonSerializationResult::ResultCode CreatePatch(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, JsonCreatePatchSettings& settings);

        //! Implementation of the JSON Merge Patch algorithm: https://tools.ietf.org/html/rfc7386
        static JsonSerializationResult::ResultCode ApplyMergePatch(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
            JsonApplyPatchSettings& settings);

        //! Implementation of the JSON Merge Patch algorithm: https://tools.ietf.org/html/rfc7386
        static JsonSerializationResult::ResultCode ApplyMergePatchInternal(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
            JsonApplyPatchSettings& settings, StackedString& element);

        //! Function to create JSON Merge Patches: https://tools.ietf.org/html/rfc7386
        static JsonSerializationResult::ResultCode CreateMergePatch(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, JsonCreatePatchSettings& settings);

        static JsonSerializationResult::ResultCode ApplyPatch_GetFromValue(rapidjson::Value** fromValue, rapidjson::Pointer& fromPointer,
            rapidjson::Value& target, const rapidjson::Value& entry, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Add(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_AddValue(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Pointer& path, rapidjson::Value&& newValue, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Remove(rapidjson::Value& target, const rapidjson::Pointer& path,
            StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Replace(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Move(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Copy(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Test(rapidjson::Value& target,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, JsonApplyPatchSettings& settings);

        static JsonSerializationResult::ResultCode CreatePatchInternal(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, StackedString& element, JsonCreatePatchSettings& settings);
        static rapidjson::Value CreatePatchInternal_Add(rapidjson::Document::AllocatorType& allocator,
            StackedString& path, const rapidjson::Value& value);
        static rapidjson::Value CreatePatchInternal_Remove(rapidjson::Document::AllocatorType& allocator, StackedString& path);
        static rapidjson::Value CreatePatchInternal_Replace(rapidjson::Document::AllocatorType& allocator,
            StackedString& path, const rapidjson::Value& value);

        static JsonSerializationResult::ResultCode CreateMergePatchInternal(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, StackedString& element, JsonCreatePatchSettings& settings);
    };
} // namespace AZ

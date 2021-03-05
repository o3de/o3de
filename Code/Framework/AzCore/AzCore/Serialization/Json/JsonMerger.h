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
            const JsonApplyPatchSettings& settings);

        //! Function to create JSON Patches: https://tools.ietf.org/html/rfc6902
        static JsonSerializationResult::ResultCode CreatePatch(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, const JsonCreatePatchSettings& settings);

        //! Implementation of the JSON Merge Patch algorithm: https://tools.ietf.org/html/rfc7386
        static JsonSerializationResult::ResultCode ApplyMergePatch(rapidjson::Value& target,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& patch,
            const JsonApplyPatchSettings& settings);

        //! Function to create JSON Merge Patches: https://tools.ietf.org/html/rfc7386
        static JsonSerializationResult::ResultCode CreateMergePatch(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, const JsonCreatePatchSettings& settings);

        static JsonSerializationResult::ResultCode ApplyPatch_GetFromValue(rapidjson::Value** fromValue, rapidjson::Pointer& fromPointer,
            rapidjson::Value& target, const rapidjson::Value& entry, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Add(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_AddValue(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Pointer& path, rapidjson::Value&& newValue, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Remove(rapidjson::Value& target, const rapidjson::Pointer& path,
            StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Replace(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Move(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Copy(rapidjson::Value& target, rapidjson::Document::AllocatorType& allocator,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, const JsonApplyPatchSettings& settings);
        static JsonSerializationResult::ResultCode ApplyPatch_Test(rapidjson::Value& target,
            const rapidjson::Value& entry, const rapidjson::Pointer& path, StackedString& element, const JsonApplyPatchSettings& settings);

        static JsonSerializationResult::ResultCode CreatePatchInternal(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, StackedString& element, const JsonCreatePatchSettings& settings);
        static rapidjson::Value CreatePatchInternal_Add(rapidjson::Document::AllocatorType& allocator,
            StackedString& path, const rapidjson::Value& value);
        static rapidjson::Value CreatePatchInternal_Remove(rapidjson::Document::AllocatorType& allocator, StackedString& path);
        static rapidjson::Value CreatePatchInternal_Replace(rapidjson::Document::AllocatorType& allocator,
            StackedString& path, const rapidjson::Value& value);

        static JsonSerializationResult::ResultCode CreateMergePatchInternal(rapidjson::Value& patch,
            rapidjson::Document::AllocatorType& allocator, const rapidjson::Value& source,
            const rapidjson::Value& target, StackedString& element, const JsonCreatePatchSettings& settings);
    };
} // namespace AZ

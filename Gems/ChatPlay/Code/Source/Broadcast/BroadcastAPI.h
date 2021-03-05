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
#ifndef CRYINCLUDE_CRYACTION_BROADCAST_BROADCAST_H
#define CRYINCLUDE_CRYACTION_BROADCAST_BROADCAST_H
#pragma once

#include <AzCore/PlatformDef.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/http/HttpTypes.h>
#include <aws/core/http/HttpResponse.h>
AZ_POP_DISABLE_WARNING
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace ChatPlay
{

    // BroadcastAPI Interface
    class IBroadcast
    {
    public:
        // Abstraction of channel ID
        typedef std::string ChannelId;

        enum class ApiCallResult
        {
            SUCCESS,
            ERROR_NULL_OBJECT,        // The requested key (or its parent) is missing
            ERROR_UNEXPECTED_TYPE,    // The requested key is of unexpected type (the REST API may have been changed)
            ERROR_HTTP_REQUEST_FAILED // The API request failed
        };

        // Value by type
        enum class ApiKey
        {
            // Bools
            CHANNEL_MATURE,
            CHANNEL_PARTNER,

            // Ints
            CHANNEL_DELAY = 100,
            RESERVED_1, // for future use
            CHANNEL_VIEWS,
            CHANNEL_FOLLOWERS,
            STREAM_VIEWERS,
            STREAM_VIDEO_HEIGHT,

            // Floats
            STREAM_AVERAGE_FPS = 200,

            // Strings
            CHANNEL_STATUS = 300,
            CHANNEL_BROADCASTER_LANGUAGE,
            CHANNEL_DISPLAY_NAME,
            CHANNEL_GAME,
            CHANNEL_LANGUAGE,
            CHANNEL_NAME,
            CHANNEL_CREATED_AT, // iso-formatted date/time
            CHANNEL_UPDATED_AT, // iso-formatted date/time
            CHANNEL_URL,
            STREAM_GAME,
            STREAM_CREATED_AT, // iso-formatted date/time
            USER_TYPE,
            USER_NAME,
            USER_CREATED_AT, // iso-formatted date/time
            USER_UPDATED_AT, // iso-formatted date/time
            USER_LOGO,
            USER_DISPLAY_NAME,
            USER_BIO,
            CHANNEL_ID,
            STREAM_ID,
            USER_ID,
        };

        // Callback types
        typedef AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, bool)> BoolCallback;
        typedef AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, int)> IntCallback;
        typedef AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, float)> FloatCallback;
        typedef AZStd::function<void(ApiCallResult, Aws::Http::HttpResponseCode, const std::string&)> StringCallback;

        virtual ~IBroadcast() = 0;

        // Methods for registering a callback to get the value mapped to a key in the API
        virtual void GetBoolValue(const ChannelId& channelID, ApiKey type, const BoolCallback& userCallback) = 0;
        virtual void GetIntValue(const ChannelId& channelID, ApiKey type, const IntCallback& userCallback) = 0;
        virtual void GetFloatValue(const ChannelId& channelID, ApiKey type, const FloatCallback& userCallback) = 0;
        virtual void GetStringValue(const ChannelId& channelID, ApiKey type, const StringCallback& userCallback) = 0;

        // Executes all awaiting callbacks on the thread that calls this function
        virtual size_t DispatchEvents() = 0;

        virtual const char* GetFlowNodeString() = 0;
    };

    inline IBroadcast::~IBroadcast() {}

    using IBroadcastPtr = AZStd::unique_ptr<IBroadcast> ;
    IBroadcastPtr CreateBroadcastAPI();

}

#endif //CRYINCLUDE_CRYACTION_BROADCASTAPI_BROADCAST_H

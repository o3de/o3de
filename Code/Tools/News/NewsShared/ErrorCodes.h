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

#include <QString>

namespace News
{
    enum class ErrorCode : int
    {
        None,
        OutOfSync,
        ManifestDownloadFail,
        FailedToSync,
        AlreadySyncing,
        FailedToParseManifest,
        MissingArticle,
        NoEndpoint,
        ManifestUploadFail,
        S3Fail
    };

    inline extern const char* GetErrorMessage(ErrorCode errorCode)
    {
        static const char* errors[]
        {
            "",
            "Your manifest is out of sync. Reopen the same endpoint and sync to resolve the conflict and try again.",
            "Failed to download resource manifest",
            "Failed to sync resources",
            "Sync is already running",
            "Failed to parse resource manifest",
            "Could not find article, try syncing again",
            "Missing or incorrect endpoint selected",
            "Failed to upload resource manifest",
            "Failed to init S3 connection"
        };
        int errorCount = sizeof errors / sizeof errors[0];
        int typeIndex = static_cast<int>(errorCode);
        if (typeIndex < 0 || typeIndex >= errorCount)
        {
            return "Invalid error code";
        }
        return errors[typeIndex];
    }
} // namespace News
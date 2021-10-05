/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DefaultClientIdProvider.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AWSMetrics
{
    AZStd::unique_ptr<IdentityProvider> IdentityProvider::CreateIdentityProvider()
    {
        return AZStd::make_unique<DefaultClientIdProvider>(GetEngineVersion());
    }

    AZStd::string IdentityProvider::GetEngineVersion()
    {
        static constexpr const char* EngineConfigFilePath = "@products@/engine.json";
        static constexpr const char* EngineVersionJsonKey = "O3DEVersion";

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO)
        {
            AZ_Error("AWSMetrics", false, "No FileIoBase Instance");
            return "";
        }

        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        if (!fileIO->ResolvePath(EngineConfigFilePath, resolvedPath, AZ_MAX_PATH_LEN))
        {
            AZ_Error("AWSMetrics", false, "Failed to resolve the engine config file directory");
            return "";
        }

        auto readOutcome = AZ::JsonSerializationUtils::ReadJsonFile(resolvedPath);
        if (!readOutcome.IsSuccess())
        {
            AZ_Error("AWSMetrics", false, readOutcome.GetError().c_str());
            return "";
        }

        rapidjson_ly::Document& jsonDoc = readOutcome.GetValue();
        auto memberIt = jsonDoc.FindMember(EngineVersionJsonKey);
        if (memberIt != jsonDoc.MemberEnd())
        {
            return memberIt->value.GetString();
        }

        return "";
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SchemaUtils.h"

#include<AzCore/Serialization/Utils.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace CopyDependencyBuilder
{
    bool SourceFileDependsOnSchema(const AzFramework::XmlSchemaAsset& schemaAsset, const AZStd::string& sourceFilePath)
    {
        AZStd::string normalizedSourceFilePath = sourceFilePath;
        // Resolve any aliases, otherwise they will cause problems.
        char normalizedSourcePathCharArray[AZ::IO::MaxPathLength];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(normalizedSourceFilePath.c_str(), normalizedSourcePathCharArray, sizeof(normalizedSourcePathCharArray));
        normalizedSourceFilePath = normalizedSourcePathCharArray;

        if (!AzFramework::StringFunc::Path::Normalize(normalizedSourceFilePath))
        {
            // The path couldn't be safely normalized.
            return false;
        }

        for (const AzFramework::MatchingRule& matchingRule : schemaAsset.GetMatchingRules())
        {
            AZStd::string filePathPattern = matchingRule.GetFilePathPattern();
            AZStd::string excludedFilePathPattern = matchingRule.GetExcludedFilePathPattern();

            AzFramework::StringFunc::Replace(filePathPattern, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
            AzFramework::StringFunc::Replace(excludedFilePathPattern, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

            // Do not check the file data version here since it requires to open the source XML file and is time consuming
            // We'll do the checking when parsing product dependencies in the XML builder worker
            if (!AZStd::wildcard_match(filePathPattern.c_str(), normalizedSourceFilePath.c_str()) ||
                (!excludedFilePathPattern.empty() && AZStd::wildcard_match(excludedFilePathPattern.c_str(), normalizedSourceFilePath.c_str())))
            {
                continue;
            }

            return true;
        }

        return false;
    }
}

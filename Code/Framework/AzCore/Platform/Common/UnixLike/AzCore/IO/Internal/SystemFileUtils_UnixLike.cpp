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

#include "SystemFileUtils_UnixLike.h"

namespace AZ
{
    namespace IO
    {
        namespace Internal
        {
            bool FormatAndPeelOffWildCardExtension(const char* sourcePath, char* filePath, size_t filePathSize, char* extensionPath, size_t extensionSize, bool keepWildcard)
            {
                if (sourcePath == nullptr || filePath == nullptr || extensionPath == nullptr || filePathSize == 0 || extensionSize == 0)
                {
                    AZ_Error("AZ::IO::Internal", false, "FormatAndPeelOffWildCardExtension: One or more parameters was invalid.");
                    return false;
                }
                const char* pSrcPath = sourcePath;
                char* pDestPath = filePath;
                size_t destinationSize = filePathSize;
                unsigned    numFileChars = 0;
                unsigned    numExtensionChars = 0;
                unsigned* pNumDestChars = &numFileChars;
                bool        bIsWildcardExtension = false;
                while (*pSrcPath)
                {
                    char srcChar = *pSrcPath++;

                    // Skip '*' and '.'
                    if ((!bIsWildcardExtension && srcChar != '*') || (bIsWildcardExtension && srcChar != '.' && (keepWildcard || srcChar != '*')))
                    {
                        unsigned numChars = *pNumDestChars;
                        pDestPath[numChars++] = srcChar;
                        *pNumDestChars = numChars;

                        --destinationSize;
                        if (destinationSize == 0)
                        {
                            AZ_Error(
                                "AZ::IO::Internal",
                                false,
                                "Error splitting sourcePath '%s' into filePath and extension, %s length is larger than storage size %d.",
                                sourcePath,
                                bIsWildcardExtension ? "extensionPath" : "filePath",
                                bIsWildcardExtension ? extensionSize : filePathSize);
                            return false;
                        }
                    }
                    // Wild-card extension is separate
                    if (srcChar == '*')
                    {
                        bIsWildcardExtension = true;
                        pDestPath = extensionPath;
                        destinationSize = extensionSize;
                        pNumDestChars = &numExtensionChars;
                        if (keepWildcard)
                        {
                            unsigned numChars = *pNumDestChars;
                            pDestPath[numChars++] = srcChar;
                            *pNumDestChars = numChars;

                            --destinationSize;
                            if (destinationSize == 0)
                            {
                                AZ_Error(
                                    "AZ::IO::Internal",
                                    false,
                                    "Error splitting sourcePath '%s' into filePath and extension, extensionPath length is larger than storage size %d.",
                                    sourcePath,
                                    extensionSize);
                                return false;
                            }
                        }
                    }
                }
                // Close strings
                filePath[numFileChars] = 0;
                extensionPath[numExtensionChars] = 0;
                return true;
            }
        }
    }
}
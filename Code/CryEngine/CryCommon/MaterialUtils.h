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
#ifndef CRYINCLUDE_CRYCOMMON_MATERIALUTILS_H
#define CRYINCLUDE_CRYCOMMON_MATERIALUTILS_H

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h> // for max path len
#include <AzCore/std/string/string.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ISystem.h>

namespace MaterialUtils
{
    //! UnifyMaterialName - given a non-unified material name, remove the extension, unify the slashes
    //! and fix up any legacy naming issues so that the material name can be used in a hash map
    //! and will work each lookup.
    inline void UnifyMaterialName(char* inputOutputBuffer)
    {
        if (!inputOutputBuffer)
        {
            return;
        }

        // convert slashes and remove extensions:
        size_t inputLength = strlen(inputOutputBuffer);
        if (inputLength == 0)
        {
            return;
        }
        // this must be done first, so that the extension cutting function below does not mistakenly destroy this when it finds the .
        if ((azstrnicmp(inputOutputBuffer, "./", 2) == 0) || (azstrnicmp(inputOutputBuffer, ".\\", 2) == 0))
        {
            memmove(inputOutputBuffer, inputOutputBuffer + 2, inputLength - 2);
            inputOutputBuffer[inputLength - 2] = 0;
            inputLength -= 2;
        }

        for (size_t pos = 0; pos < inputLength; ++pos)
        {
            if (inputOutputBuffer[pos] == '\\')
            {
                inputOutputBuffer[pos] = '/'; // unify slashes
            }
            else
            {
                inputOutputBuffer[pos] = tolower(inputOutputBuffer[pos]);
            }
        }
        AZStd::string tempString(inputOutputBuffer);
        AzFramework::StringFunc::Path::StripExtension(tempString);

        AZ_Assert(tempString.length() <= inputLength, "Extension stripped string has to be smaller than/same size as original string!");
        // real size of inputOutputBuffer is inputLength + 1 with Null character
        azstrcpy(inputOutputBuffer, inputLength + 1, tempString.c_str());
#if defined(SUPPORT_LEGACY_MATERIAL_NAMES)

        // LEGACY support  Some files may start with ./ in front of them.  This is not required anymore.
        static const char* removals[2] = {
            "engine/",
            nullptr // reserved for game name
        };
        static size_t removalSize = sizeof(removals) / sizeof(removals[0]);

        // LEGACY support.  Some files may start with gamename in front of them.  This is not required anymore.
        static char cachedGameName[AZ_MAX_PATH_LEN] = { 0 };
        if (!removals[removalSize - 1])
        {
            auto projectName = AZ::Utils::GetProjectName();
            if (!projectName.empty())
            {
                azstrcpy(cachedGameName, AZ_MAX_PATH_LEN, projectName.c_str());
                azstrcat(cachedGameName, AZ_MAX_PATH_LEN, "/");
            }

            if (cachedGameName[0] == 0)
            {
                // at least substitute something so that unit tests can make this assumption:
                azstrcpy(cachedGameName, AZ_MAX_PATH_LEN, "SamplesProject/");
            }

            removals[removalSize - 1] = cachedGameName;
        }

        for (size_t pos = 0; pos < removalSize; ++pos)
        {
            if (removals[pos])
            {
                size_t removalLength = strlen(removals[pos]);
                if (removalLength >= inputLength)
                {
                    continue;
                }

                if (azstrnicmp(inputOutputBuffer, removals[pos], removalLength) == 0)
                {
                    memmove(inputOutputBuffer, inputOutputBuffer + removalLength, inputLength - removalLength);
                    inputOutputBuffer[inputLength - removalLength] = 0;
                    inputLength -= removalLength;
                }
            }
        }

        // legacy:  Files were saved into a mtl with many leading forward or back slashes, we eat them all here.  We want it to start with a rel path.
        const char* actualFileName = inputOutputBuffer;
        size_t finalLength = inputLength;
        while ((actualFileName[0]) && ((actualFileName[0] == '\\') || (actualFileName[0] == '/')))
        {
            ++actualFileName;
            --finalLength;
        }
        if (finalLength != inputLength)
        {
            memmove(inputOutputBuffer, actualFileName, finalLength);
            inputOutputBuffer[finalLength] = 0;
            inputLength = finalLength;
        }

#endif
    }
}

#endif // CRYINCLUDE_CRYCOMMON_MATERIALUTILS_H

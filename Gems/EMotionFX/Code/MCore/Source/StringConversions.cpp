/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace MCore
{
    const char* CharacterConstants::wordSeparators = " \n\t";

    AZStd::string GenerateUniqueString(const char* prefix, const AZStd::function<bool(const AZStd::string& value)>& validationFunction)
    {
        MCORE_ASSERT(validationFunction);
        const AZStd::string prefixString = prefix;

        // find the last letter index from the right
        size_t lastIndex = AZStd::string::npos;
        const size_t numCharacters = prefixString.size();
        for (int i = static_cast<int>(numCharacters) - 1; i >= 0; --i)
        {
            if (!AZStd::is_digit(prefixString[i]))
            {
                lastIndex = i + 1;
                break;
            }
        }

        // copy the string
        AZStd::string nameWithoutLastDigits = prefixString.substr(0, lastIndex);

        // remove all space on the right
        AzFramework::StringFunc::TrimWhiteSpace(nameWithoutLastDigits, false /* leading */, true /* trailing */);

        // generate the unique name
        size_t nameIndex = 0;
        AZStd::string uniqueName = nameWithoutLastDigits + "0";
        while (validationFunction(uniqueName) == false)
        {
            uniqueName = nameWithoutLastDigits + AZStd::to_string(++nameIndex);
        }

        return uniqueName;
    }

    AZStd::string ConstructStringSeparatedBySemicolons(const AZStd::vector<AZStd::string>& stringVec)
    {
        AZStd::string result;

        for (const AZStd::string& currentString : stringVec)
        {
            if (!result.empty())
            {
                result += CharacterConstants::semiColon;
            }

            result += currentString;
        }

        return result;
    }
}

namespace AZStd
{
    void to_string(string& str, bool value)
    {
        str = value ? "true" : "false";
    }
}

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
// Original file Copyright Crytek GMBH or its affiliates, used under license.


// Returns the string version of any type convertible by AZStd::to_string()
// otherwise throws a compile error.
template <typename T>
AZStd::string LocalizationHelpers::DataToString(T t)
{
    return AZStd::to_string(t);
}

// Need a function to specifically handle AZStd::string, since AZStd::to_string() doesn't handle it.
AZStd::string LocalizationHelpers::DataToString(AZStd::string str)
{
    return str;
}

// Need a function to specifically handle const char*, since AZStd::to_string() doesn't handle it.
AZStd::string LocalizationHelpers::DataToString(const char* str)
{
    return str;
}

// Base case of the recursive function to convert a data list to strings
template<typename T>
void LocalizationHelpers::ConvertValuesToStrings(AZStd::vector<AZStd::string>& values, T t)
{
    values.push_back(DataToString(t));
}

// Recursive function to convert a data list to strings
template<typename T, typename... Args>
void LocalizationHelpers::ConvertValuesToStrings(AZStd::vector<AZStd::string>& values, T t, Args... args)
{
    values.push_back(DataToString(t));
    ConvertValuesToStrings(values, args...);
}

// Check if a key string is in a list of substitution strings
bool LocalizationHelpers::IsKeyInList(const AZStd::vector<AZStd::string>& keys, const AZStd::string& target, int& index)
{
    for (index = 0; index < keys.size(); ++index)
    {
        if (keys[index] == target)
        {
            return true;
        }
    }

    index = -1;
    return false;
}

// Summary:
//   Parse localized string and substitute data in for each key that is surrounded by curly braces. Number of arguments 
//   after 'const AZStd::vector<AZStd::string>& keys' should be equal to the number of strings in 'keys'.
//   ex:
//        float distance = GetWinDistance();
//        AZStd::string winState = IsPlayerFirstPlace() ? "won" : "lost";
//        LocalizationManagerRequests::Broadcast(&LocalizationManagerRequests::Events::LocalizeAndSubstitute<float, AZStd::string>
//                                          , "@QUICKRESULTS_DISTANCEDIFFERENCE, outLocalizedString
//                                          , MakeLocKeyString("race_result", "distance_ahead")
//                                          , winState, distance);
//
//  where "@QUICKRESULTS_DISTANCEDIFFERENCE" would be localized to "You {race_result} by {distance_ahead} meters!" and then 
//  "{race_result}" would be replaced by 'winState" and "{distance_ahead}" would be replaced by the 'distance' argument as a string.
template<typename T, typename... Args>
void LocalizationManagerRequests::LocalizeAndSubstitute(const AZStd::string& locString, AZStd::string& outLocalizedString, const AZStd::vector<AZStd::string>& keys, T t, Args... args)
{    
    AZStd::vector<AZStd::string> values;
    LocalizationHelpers::ConvertValuesToStrings(values, t, args...);

    outLocalizedString = locString;
    LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::LocalizeAndSubstituteInternal, outLocalizedString, keys, values);
}
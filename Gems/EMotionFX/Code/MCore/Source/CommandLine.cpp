/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "CommandLine.h"
#include "Command.h"
#include "CommandSyntax.h"
#include "LogManager.h"
#include "StringConversions.h"

namespace MCore
{
    // extended constructor, which automatically sets the command line to parse
    CommandLine::CommandLine(const AZStd::string& commandLine)
    {
        SetCommandLine(commandLine);
    }


    // get the value for a given parameter
    void CommandLine::GetValue(const char* paramName, const char* defaultValue, AZStd::string* outResult) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            *outResult = defaultValue;
            return;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            *outResult = defaultValue;
            return;
        }

        // return the parameter value
        *outResult = m_parameters[paramIndex].m_value;
    }


    // Get the value for a given parameter.
    void CommandLine::GetValue(const char* paramName, const char* defaultValue, AZStd::string& outResult) const
    {
        // Try to find the parameter index.
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            outResult = defaultValue;
            return;
        }

        // Return the default value if the parameter value is empty.
        if (m_parameters[paramIndex].m_value.empty())
        {
            outResult = defaultValue;
            return;
        }

        // Return the actual parameter value.
        outResult = m_parameters[paramIndex].m_value.c_str();
    }


    // get the value as an integer
    int32 CommandLine::GetValueAsInt(const char* paramName, int32 defaultValue) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            return defaultValue;
        }

        // return the parameter value
        return AzFramework::StringFunc::ToInt(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as a float
    float CommandLine::GetValueAsFloat(const char* paramName, float defaultValue) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            return defaultValue;
        }

        // return the parameter value
        return AzFramework::StringFunc::ToFloat(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as a boolean
    bool CommandLine::GetValueAsBool(const char* paramName, bool defaultValue) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            return true;
        }

        // return the parameter value
        return AzFramework::StringFunc::ToBool(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as a three component vector
    AZ::Vector3 CommandLine::GetValueAsVector3(const char* paramName, const AZ::Vector3& defaultValue) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            return defaultValue;
        }

        // return the parameter value

        return AzFramework::StringFunc::ToVector3(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as a four component vector
    AZ::Vector4 CommandLine::GetValueAsVector4(const char* paramName, const AZ::Vector4& defaultValue) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (m_parameters[paramIndex].m_value.empty())
        {
            return defaultValue;
        }

        // return the parameter value
        return AzFramework::StringFunc::ToVector4(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value of a given param
    void CommandLine::GetValue(const char* paramName, Command* command, AZStd::string* outResult) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, *outResult);
            return;
        }

        // return the parameter value
        *outResult = m_parameters[paramIndex].m_value;
    }


    void CommandLine::GetValue(const char* paramName, Command* command, AZStd::string& outResult) const
    {
        outResult = GetValue(paramName, command);
    }

    AZ::Outcome<AZStd::string> CommandLine::GetValueIfExists(const char* paramName, Command* command) const
    {
    AZ_UNUSED(command);
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex != InvalidIndex)
        {
            return AZ::Success(m_parameters[paramIndex].m_value);
        }

        return AZ::Failure();
    }


    const AZStd::string& CommandLine::GetValue(const char* paramName, Command* command) const
    {
        // Try to find the parameter index.
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName);
        }

        // return the parameter value
        return m_parameters[paramIndex].m_value;
    }


    // get the value as int
    int32 CommandLine::GetValueAsInt(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            AZStd::string result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return AzFramework::StringFunc::ToInt(result.c_str());
            }
            else
            {
                return InvalidIndexT<int32>;
            }
        }

        // return the parameter value
        return AzFramework::StringFunc::ToInt(m_parameters[paramIndex].m_value.c_str());
    }

    // get the value as float
    float CommandLine::GetValueAsFloat(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            AZStd::string result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return AzFramework::StringFunc::ToFloat(result.c_str());
            }
            else
            {
                return 0.0f;
            }
        }

        // return the parameter value
        return AzFramework::StringFunc::ToFloat(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as bool
    bool CommandLine::GetValueAsBool(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            AZStd::string result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return AzFramework::StringFunc::ToBool(result.c_str());
            }
            else
            {
                return false;
            }
        }

        // return the parameter value
        return AzFramework::StringFunc::ToBool(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as three component vector
    AZ::Vector3 CommandLine::GetValueAsVector3(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            AZStd::string result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return AzFramework::StringFunc::ToVector3(result.c_str());
            }
            else
            {
                return AZ::Vector3(0.0f, 0.0f, 0.0);
            }
        }

        // return the parameter value
        return AzFramework::StringFunc::ToVector3(m_parameters[paramIndex].m_value.c_str());
    }


    // get the value as four component vector
    AZ::Vector4 CommandLine::GetValueAsVector4(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            AZStd::string result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return AzFramework::StringFunc::ToVector4(result.c_str());
            }
            else
            {
                return AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // return the parameter value
        return AzFramework::StringFunc::ToVector4(m_parameters[paramIndex].m_value.c_str());
    }


    // get the number of parameters
    size_t CommandLine::GetNumParameters() const
    {
        return m_parameters.size();
    }


    // get the parameter name for a given parameter
    const AZStd::string& CommandLine::GetParameterName(size_t nr) const
    {
        return m_parameters[nr].m_name;
    }


    // get the parameter value for a given parameter number
    const AZStd::string& CommandLine::GetParameterValue(size_t nr) const
    {
        return m_parameters[nr].m_value;
    }


    // check if a given parameter has a value
    bool CommandLine::CheckIfHasValue(const char* paramName) const
    {
        // try to find the parameter index
        const size_t paramIndex = FindParameterIndex(paramName);
        if (paramIndex == InvalidIndex)
        {
            return false;
        }

        // return true the parameter has a value that is not empty
        return (m_parameters[paramIndex].m_value.empty() == false);
    }


    // try to find a given parameter's index into the parameter array
    size_t CommandLine::FindParameterIndex(const char* paramName) const
    {
        // compare all parameter names on a non-case sensitive way
        const auto foundParameter = AZStd::find_if(begin(m_parameters), end(m_parameters), [paramName](const Parameter& parameter)
        {
            return AzFramework::StringFunc::Equal(parameter.m_name, paramName, false /* no case */);
        });

        return foundParameter != end(m_parameters) ? AZStd::distance(begin(m_parameters), foundParameter) : InvalidIndex;
    }


    // check if we have a parameter with a given name defined
    bool CommandLine::CheckIfHasParameter(const char* paramName) const
    {
        return (FindParameterIndex(paramName) != InvalidIndex);
    }

    bool CommandLine::CheckIfHasParameter(const AZStd::string& paramName) const
    {
        return (FindParameterIndex(paramName.c_str()) != InvalidIndex);
    }

    // extract the next parameter, starting from a given offset
    bool CommandLine::ExtractNextParam(const AZStd::string& paramString, AZStd::string& outParamName, AZStd::string& outParamValue, size_t* inOutStartOffset)
    {
        outParamName.clear();
        outParamValue.clear();

        // check if we already reached the end of the string
        size_t offset = *inOutStartOffset;
        if (offset >= paramString.size())
        {
            return false;
        }

        // filter out the next parameter
        AZStd::string::const_iterator iterator = paramString.begin() + offset;
        size_t  paramNameStart      = InvalidIndex;
        size_t  paramValueStart     = InvalidIndex;
        bool    readingParamName    = false;
        bool    readingParamValue   = false;
        bool    foundNextParam      = false;
        bool    insideQuotes        = false;
        bool    prevCharWasSpace    = false;
        int32   bracketDepth        = 0;

        const char bracketOpen('{');
        const char bracketClose('}');

        while (foundNextParam == false)
        {
            // check if we reached the end now
            if (offset >= paramString.size())
            {
                *inOutStartOffset = offset;

                // if we were reading a parameter value
                if (readingParamValue)
                {
                    outParamValue = AZStd::string(&paramString[paramValueStart], offset - paramValueStart);
                    readingParamValue = false;
                    AzFramework::StringFunc::TrimWhiteSpace(outParamValue, false /* leading */, true /* trailing */);
                    AzFramework::StringFunc::Strip(outParamValue, bracketClose, true /* case sensitive */, false /* beginning */, true /* ending */);
                    AzFramework::StringFunc::Strip(outParamValue, bracketOpen, true /* case sensitive */, true /* beginning */, false /* ending */);
                    AzFramework::StringFunc::Strip(outParamValue, MCore::CharacterConstants::doubleQuote, true /* case sensitive */, true /* beginning */, true /* ending */);
                }

                return true;
            }

            // get the current character and check if it was a space or not
            char curChar = *iterator;
            if (offset > 0)
            {
                char prevChar = *(iterator - 1);

                prevCharWasSpace = (prevChar == MCore::CharacterConstants::space) || (prevChar == MCore::CharacterConstants::tab);
            }
            else
            {
                prevCharWasSpace = false;
            }

            // toggle inside quotes flag
            if (curChar == MCore::CharacterConstants::doubleQuote)
            {
                insideQuotes ^= true;
            }
            else if (curChar == bracketOpen)
            {
                bracketDepth++;
            }
            else if (curChar == bracketClose)
            {
                bracketDepth--;
            }

            // if it's the start of a parameter
            if (curChar == MCore::CharacterConstants::dash && !insideQuotes && !readingParamValue && outParamName.empty() && bracketDepth == 0)
            {
                paramNameStart = offset;
                readingParamName = true;
            }

            // if we found a parameter name
            if ((curChar == MCore::CharacterConstants::space || curChar == MCore::CharacterConstants::tab) && readingParamName)
            {
                outParamName = AZStd::string(&paramString[paramNameStart + 1], offset - paramNameStart - 1);
                readingParamName = false;
            }

            // detect the start of the parameter value
            if (!readingParamName && !readingParamValue && curChar != MCore::CharacterConstants::space && curChar != MCore::CharacterConstants::tab)
            {
                readingParamValue = true;
                paramValueStart = offset;
            }

            // the end of a value
            if (curChar == MCore::CharacterConstants::dash && insideQuotes == false && readingParamValue && !readingParamName && paramValueStart != offset && prevCharWasSpace && bracketDepth == 0)
            {
                outParamValue = AZStd::string(&paramString[paramValueStart], offset - paramValueStart);
                readingParamValue = false;
                *inOutStartOffset = offset;
                AzFramework::StringFunc::TrimWhiteSpace(outParamValue, false /* leading */, true /* trailing */);
                AzFramework::StringFunc::Strip(outParamValue, bracketClose, true /* case sensitive */, false /* beginning */, true /* ending */);
                AzFramework::StringFunc::Strip(outParamValue, bracketOpen, true /* case sensitive */, true /* beginning */, false /* ending */);
                AzFramework::StringFunc::Strip(outParamValue, MCore::CharacterConstants::doubleQuote, true /* case sensitive */, true /* beginning */, true /* ending */);
                return true;
            }

            // go to the next character
            ++offset;
            ++iterator;
        }

        return false;
    }


    // parse the command line (build the parameter array)
    void CommandLine::SetCommandLine(const AZStd::string& commandLine)
    {
        // get rid of previous parameters
        m_parameters.clear();

        // extract all parameters
        AZStd::string paramName;
        AZStd::string paramValue;
        size_t offset = 0;
        while (ExtractNextParam(commandLine, paramName, paramValue, &offset))
        {
            // if the parameter name is empty then it isn't a real parameter
            if (!paramName.empty())
            {
                m_parameters.emplace_back(Parameter {paramName, paramValue});
            }
        }
    }


    // log the command line cotents
    void CommandLine::Log(const char* debugName) const
    {
        const size_t numParameters = m_parameters.size();
        LogInfo("Command line '%s' has %d parameters", debugName, numParameters);
        for (size_t i = 0; i < numParameters; ++i)
        {
            LogInfo("Param %d (name='%s'  value='%s'", i, m_parameters[i].m_name.c_str(), m_parameters[i].m_value.c_str());
        }
    }
}   // namespace MCore

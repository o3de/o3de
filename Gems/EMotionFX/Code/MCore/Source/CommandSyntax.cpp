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

// include the required headers
#include "CommandSyntax.h"
#include "LogManager.h"
#include "Algorithms.h"
#include "StringConversions.h"

namespace MCore
{
    // the constructor
    CommandSyntax::CommandSyntax(uint32 numParamsToReserve)
    {
        ReserveParameters(numParamsToReserve);
    }


    // the destructor
    CommandSyntax::~CommandSyntax()
    {
        m_parameters.clear();
    }


    // reserve parameter space
    void CommandSyntax::ReserveParameters(uint32 numParamsToReserve)
    {
        if (numParamsToReserve > 0)
        {
            m_parameters.reserve(numParamsToReserve);
        }
    }


    // add a parameter to the syntax
    void CommandSyntax::AddParameter(const char* name, const char* description, CommandSyntax::EParamType paramType, const char* defaultValue)
    {
        m_parameters.emplace_back(
            name,
            description,
            defaultValue,
            paramType,
            false
        );
    }


    // add a parameter to the syntax
    void CommandSyntax::AddRequiredParameter(const char* name, const char* description, CommandSyntax::EParamType paramType)
    {
        m_parameters.emplace_back(
            name,
            description,
            "",
            paramType,
            true
        );
    }


    // check if this param is a required one or not
    bool CommandSyntax::GetParamRequired(uint32 index) const
    {
        return m_parameters[index].mRequired;
    }


    // get the parameter name
    const char* CommandSyntax::GetParamName(uint32 index) const
    {
        return m_parameters[index].mName.c_str();
    }


    // get the parameter description
    const char* CommandSyntax::GetParamDescription(uint32 index) const
    {
        return m_parameters[index].mDescription.c_str();
    }


    // get the parameter type string
    const char* CommandSyntax::GetParamTypeString(uint32 index) const
    {
        return GetParamTypeString(m_parameters[index]);
    }

    const char* CommandSyntax::GetParamTypeString(const Parameter& parameter) const
    {
        // check the type
        switch (parameter.mParamType)
        {
            case PARAMTYPE_STRING:
                return "String";
                break;
            case PARAMTYPE_BOOLEAN:
                return "Boolean";
                break;
            case PARAMTYPE_CHAR:
                return "Char";
                break;
            case PARAMTYPE_INT:
                return "Int";
                break;
            case PARAMTYPE_FLOAT:
                return "Float";
                break;
            case PARAMTYPE_VECTOR3:
                return "Vector3";
                break;
            case PARAMTYPE_VECTOR4:
                return "Vector4";
                break;
            default: ;
        };

        // an unknown type, which should never really happen
        return "Unknown";
    }


    // get the parameter type
    CommandSyntax::EParamType CommandSyntax::GetParamType(size_t index) const
    {
        return m_parameters[index].mParamType;
    }


    // check if we have a given parameter with a given name in this syntax
    bool CommandSyntax::CheckIfHasParameter(const char* parameter) const
    {
        return (FindParameterIndex(parameter) != MCORE_INVALIDINDEX32);
    }


    // find the parameter index of a given parameter name
    uint32 CommandSyntax::FindParameterIndex(const char* parameter) const
    {
        // try to find the parameter with the given name
        const size_t numParams = m_parameters.size();
        for (size_t i = 0; i < numParams; ++i)
        {
            if (AzFramework::StringFunc::Equal(m_parameters[i].mName.c_str(), parameter, false /* no case */))
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // get the default value for a given parameter
    const AZStd::string& CommandSyntax::GetDefaultValue(uint32 index) const
    {
        return m_parameters[index].mDefaultValue;
    }


    const AZStd::string& CommandSyntax::GetDefaultValue(const char* paramName) const
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index != MCORE_INVALIDINDEX32)
        {
            return m_parameters[index].mDefaultValue;
        }

        static const AZStd::string empty;
        return empty;
    }


    // get the default value for a given parameter name
    bool CommandSyntax::GetDefaultValue(const char* paramName, AZStd::string& outDefaultValue) const
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        outDefaultValue = m_parameters[index].mDefaultValue;
        return true;
    }
    
    
    // check if the provided command parameter string is valid with this syntax
    bool CommandSyntax::CheckIfIsValid(const char* parameterList, AZStd::string& outResult) const
    {
        return CheckIfIsValid(CommandLine(parameterList), outResult);
    }


    // check if the provided commandline parameter list is valid with this syntax
    bool CommandSyntax::CheckIfIsValid(const CommandLine& commandLine, AZStd::string& outResult) const
    {
        // clear the outresult string
        outResult.clear();

        // for all parameters in the syntax
        // check if the required ones are specified in the command line
        for (const Parameter& parameter : m_parameters)
        {
            // if the required parameter hasn't been specified
            if (parameter.mRequired && commandLine.CheckIfHasParameter(parameter.mName) == false)
            {
                outResult += AZStd::string::format("Required parameter '%s' has not been specified.\n", parameter.mName.c_str());
            }
            else
            {
                // find the parameter index
                const uint32 paramIndex = commandLine.FindParameterIndex(parameter.mName.c_str());
                if (paramIndex != MCORE_INVALIDINDEX32)
                {
                    const AZStd::string& value = commandLine.GetParameterValue(paramIndex);
                    const AZStd::string& paramName = parameter.mName;

                    // if the parameter value has not been specified and it is not a boolean parameter
                    if ((value.empty()) && parameter.mParamType != PARAMTYPE_BOOLEAN && parameter.mParamType != PARAMTYPE_STRING)
                    {
                        outResult += AZStd::string::format("Parameter '%s' has no value specified.\n", paramName.c_str());
                    }
                    else
                    {
                        // check if we specified a valid int
                        if (parameter.mParamType == PARAMTYPE_INT)
                        {
                            if (!AzFramework::StringFunc::LooksLikeInt(value.c_str()))
                            {
                                outResult += AZStd::string::format("The value (%s) of integer parameter '%s' is not a valid int.\n", value.c_str(), paramName.c_str());
                            }
                        }

                        // check if the specified float is valid
                        if (parameter.mParamType == PARAMTYPE_FLOAT)
                        {
                            if (!AzFramework::StringFunc::LooksLikeFloat(value.c_str()))
                            {
                                outResult += AZStd::string::format("The value (%s) of float parameter '%s' is not a valid float.\n", value.c_str(), paramName.c_str());
                            }
                        }

                        // check if this is a valid boolean
                        if (parameter.mParamType == PARAMTYPE_BOOLEAN)
                        {
                            if (!value.empty() && !AzFramework::StringFunc::LooksLikeBool(value.c_str()))
                            {
                                outResult += AZStd::string::format("The value (%s) of boolean parameter '%s' is not a valid boolean (use true|false|0|1).\n", value.c_str(), paramName.c_str());
                            }
                        }

                        // check if this is a valid boolean
                        if (parameter.mParamType == PARAMTYPE_CHAR)
                        {
                            if (value.size() > 1)
                            {
                                outResult += AZStd::string::format("The value (%s) of character parameter '%s' is not a valid character.\n", value.c_str(), paramName.c_str());
                            }
                        }

                        // check if the specified vector3 is valid
                        if (parameter.mParamType == PARAMTYPE_VECTOR3)
                        {
                            if (!AzFramework::StringFunc::LooksLikeVector3(value.c_str()))
                            {
                                outResult += AZStd::string::format("The value (%s) of Vector3 parameter '%s' is not a valid three component vector.\n", value.c_str(), paramName.c_str());
                            }
                        }

                        // check if the specified vector3 is valid
                        if (parameter.mParamType == PARAMTYPE_VECTOR4)
                        {
                            if (!AzFramework::StringFunc::LooksLikeVector4(value.c_str()))
                            {
                                outResult += AZStd::string::format("The value (%s) of Vector4 parameter '%s' is not a valid four component vector.\n", value.c_str(), paramName.c_str());
                            }
                        }
                    }
                } // if (paramIndex != MCORE_INVALIDINDEX32)
            }
        }

        // now add parameters that we specified but that are not defined in the syntax
        const uint32 numCommandLineParams = commandLine.GetNumParameters();
        for (uint32 p = 0; p < numCommandLineParams; ++p)
        {
            if (CheckIfHasParameter(commandLine.GetParameterName(p).c_str()) == false)
            {
                MCore::LogWarning("Parameter '%s' is not defined by the command syntax and will be ignored. Use the -help flag to show syntax information.", commandLine.GetParameterName(p).c_str());
            }
        }

        // return true when there are no errors, or false when there are
        return outResult.empty();
    }


    // log the syntax
    void CommandSyntax::LogSyntax()
    {
        // find the longest command name
        uint32 offset = 0;
        for (const Parameter& parameter : m_parameters)
        {
            offset = MCore::Max<uint32>(static_cast<uint32>(parameter.mName.size()), offset);
        }

        uint32 offset2 = offset;
        uint32 offset3 = offset;

        // log the header
        AZStd::string header = "Name";
        offset += 5;
        header.append(offset - header.size(), MCore::CharacterConstants::space);
        header += "Type";
        offset += 15;
        header.append(offset - header.size(), MCore::CharacterConstants::space);
        header += "Required";
        offset += 10;
        header.append(offset - header.size(), MCore::CharacterConstants::space);
        header += "Default Value";
        offset += 20;
        header.append(offset - header.size(), MCore::CharacterConstants::space);
        header += "Description";
        MCore::LogInfo(header.c_str());
        MCore::LogInfo("--------------------------------------------------------------------------------------------------");

        // log all parameters
        AZStd::string final;
        for (const Parameter& parameter : m_parameters)
        {
            offset2 = offset3;
            final = parameter.mName;
            offset2 += 5;
            final.append(offset2 - final.size(), MCore::CharacterConstants::space);
            final += GetParamTypeString(parameter);
            offset2 += 15;
            final.append(offset2 - final.size(), MCore::CharacterConstants::space);
            final += parameter.mRequired ? "Yes" : "No";
            offset2 += 10;
            final.append(offset2 - final.size(), MCore::CharacterConstants::space);
            final += parameter.mDefaultValue;
            offset2 += 20;
            final.append(offset2 - final.size(), MCore::CharacterConstants::space);
            final += parameter.mDescription;

            MCore::LogInfo(final.c_str());
        }
    }
}   // namespace MCore

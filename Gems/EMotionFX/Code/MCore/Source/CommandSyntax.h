/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "CommandLine.h"
#include <AzCore/std/containers/vector.h>


namespace MCore
{
    /**
     * The command syntax class.
     * This class describes the parameter syntax of a given command.
     * Using this syntax the command manager can automatically perform syntax error checking when executing a command.
     */
    class MCORE_API CommandSyntax
    {
        MCORE_MEMORYOBJECTCATEGORY(CommandSyntax, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDSYSTEM)

    public:
        /**
         * The parameter type.
         */
        enum EParamType
        {
            PARAMTYPE_STRING    = 0,    /**< The parameter value is a string. */
            PARAMTYPE_BOOLEAN   = 1,    /**< The parameter value is a boolean. */
            PARAMTYPE_CHAR      = 2,    /**< The parameter value is a character. */
            PARAMTYPE_INT       = 3,    /**< The parameter value is an integer. */
            PARAMTYPE_FLOAT     = 4,    /**< The parameter value is a float. */
            PARAMTYPE_VECTOR3   = 5,    /**< The parameter value is a three component vector. */
            PARAMTYPE_VECTOR4   = 6     /**< The parameter value is a four component vector. */
        };

    private:
        /**
         * The parameter class, which describes details about a given parameter.
         */
        struct MCORE_API Parameter
        {
            Parameter(AZStd::string name, AZStd::string description, AZStd::string defaultValue, EParamType paramType, bool required)
                : m_name(AZStd::move(name)), m_description(AZStd::move(description)), m_defaultValue(AZStd::move(defaultValue)), m_paramType(paramType), m_required(required)
            {
            }

            AZStd::string      m_name;          /**< The name of the parameter. */
            AZStd::string      m_description;   /**< The description of the parameter. */
            AZStd::string      m_defaultValue;  /**< The default value. */
            EParamType         m_paramType;     /**< The parameter type. */
            bool               m_required;      /**< Is this parameter required or optional? */
        };

    public:
        /**
         * The constructor.
         * @param numParamsToReserve The amount of parameters to pre-allocate memory for. This can reduce the number of reallocs needed when registering new paramters.
         */
        CommandSyntax(size_t numParamsToReserve = 5);

        /**
         * The destructor.
         */
        ~CommandSyntax();

        /**
         * Reserve space for a given number of parameters, to prevent memory reallocs when adding new parameters.
         * @param numParamsToReserve The number of parameters to reserve space for.
         */
        void ReserveParameters(size_t numParamsToReserve);

        /**
         * Add a new optional parameter to this syntax.
         * The order in which you add parameters isn't really important.
         * @param name The parameter name.
         * @param description The description of the parameter.
         * @param paramType The type of the parameter.
         * @param defaultValue The default value of the parameter.
         */
        void AddParameter(const char* name, const char* description, EParamType paramType, const char* defaultValue);

        /**
         * Add a required parameter to the syntax.
         * @param name The name of the parameter.
         * @param description The description of the parameter.
         * @param paramType The parameter type.
         */
        void AddRequiredParameter(const char* name, const char* description, EParamType paramType);

        /**
         * Check if a given parameter is required or not.
         * @param index The parameter number to check.
         * @result Returns true when the parameter is required, or false when it is optional.
         */
        bool GetParamRequired(size_t index) const;

        /**
         * Get the name of a given parameter.
         * @param index The parameter number to get the name for.
         * @result The string containing the name of the parameter.
         */
        const char* GetParamName(size_t index) const;

        /**
         * Get the description of a given parameter.
         * @param index The parameter number to get the description for.
         * @result A string containing the description of the parameter.
         */
        const char* GetParamDescription(size_t index) const;

        /**
         * Get the default value for a given parameter.
         * @param index The parameter number to get the default value from.
         * @result The string containing the default value.
         */
        const AZStd::string& GetDefaultValue(size_t index) const;

        /**
         * Get the default value for a parameter with a given name.
         * @param paramName The parameter name to check for.
         * @result The string that will receive the default value.
         */
        const AZStd::string& GetDefaultValue(const char* paramName) const;

        /**
         * Get the default value for a parameter with a given name.
         * @param paramName The parameter name to check for.
         * @param outDefaultValue The string that will receive the default value.
         * @result Returns true when the parameter default value has been looked up successfully, otherwise false is returned (no parameter with such name found).
         */
        bool GetDefaultValue(const char* paramName, AZStd::string& outDefaultValue) const;

        /**
         * Get the number of parameters registered to this syntax.
         * @result The number of added/registered parameters.
         */
        MCORE_INLINE size_t GetNumParameters() const                { return m_parameters.size(); }

        /**
         * Get the parameter type string of a given parameter.
         * This returns a human readable string of the parameter type, for example "STRING" or "FLOAT".
         * @param index The parameter number to get the type string for.
         * @result The parameter type string.
         */
        const char* GetParamTypeString(size_t index) const;
        const char* GetParamTypeString(const Parameter& parameter) const;

        /**
         * Get the value type of a given parameter.
         * @param index The parameter number to get the value type from.
         * @result The type of the parameter value.
         */
        EParamType GetParamType(size_t index) const;

        /**
         * Check if a given parameter list would be valid with this syntax.
         * The parameter list will look like: "-numItems 10 -enableMixing true".
         * @param parameterList The parameter string to validate with this syntax.
         * @param outResult The string that will receive the result of the validation. This will contain the errors in case it's invalid.
         * @result Returns false when the parameter list is invalid, otherwise true is returned.
         */
        bool CheckIfIsValid(const char* parameterList, AZStd::string& outResult) const;

        /**
         * Check if a given command line is valid in combination with this syntax.
         * @param commandLine The command line to check.
         * @param outResult The string that will receive the result of the validation. This will contain the errors in case it's invalid.
         * @result Returns false when the command line is invalid, otherwise true is returned.
         */
        bool CheckIfIsValid(const CommandLine& commandLine, AZStd::string& outResult) const;

        /**
         * Check if we already registered a parameter with a given name.
         * This is non-case-sensitive.
         * @param parameter The name of the parameter.
         * @result Returns true when the parameter has already been registered to this syntax, otherwise false is returned.
         */
        bool CheckIfHasParameter(const char* parameter) const;

        /**
         * Find the parameter number of the parameter with a specified name.
         * @param parameter The name of the parameter, non-case-sensitive.
         * @result Returns the index of the parameter, in range of [0..GetNumParameters()-1], or MCORE_INVALIDINDEX32 in case it hasn't been found.
         */
        size_t FindParameterIndex(const char* parameter) const;

        /**
         * Log the currently registered syntax using MCore::LogInfo(...).
         */
        void LogSyntax();

    private:
        AZStd::vector<Parameter> m_parameters;    /**< The array of registered parameters. */
    };
} // namespace MCore

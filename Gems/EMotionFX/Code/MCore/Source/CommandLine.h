/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include "StandardHeaders.h"


namespace MCore
{
    // forward declarations
    class Command;

    /**
     * A command line parser class.
     * This class makes it very easy to parse values from a command/argument line.
     * An example of a command line would be "-fullscreen true -xres 800 -yres 1024 -threshold 0.145 -culling false".
     * All parameters must have a value.
     * Use the GetValue, GetValueAsInt, GetValueAsFloat and GetValueAsBool methods to quickly extract values for
     * any given parameter in the command line. A parameter here is for example "xres" or "yres". Each parameter
     * can have a value associated with it.
     */
    class MCORE_API CommandLine
    {
        MCORE_MEMORYOBJECTCATEGORY(CommandLine, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_COMMANDLINE)

    public:
        /**
         * The default constructor.
         * This does not yet process any command line. You have to use the SetCommandLine method to specify the command line to parse.
         */
        CommandLine() = default;

        /**
         * The extended constructor.
         * @param commandLine The command line to parse. This automatically calls the SetCommandLine function.
         */
        explicit CommandLine(const AZStd::string& commandLine);

        /**
         * Get the value for a parameter with a specified name.
         * If the parameter with the given name does not exist, the default value is returned.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @param outResult The resulting value for the parameter. This is NOT allowed to be nullptr.
         * @result The value for the parameter with the specified name.
         */
        void GetValue(const char* paramName, const char* defaultValue, AZStd::string* outResult) const;
        void GetValue(const char* paramName, const char* defaultValue, AZStd::string& outResult) const;

        /**
         * Get the value for a parameter with a specified name, as an integer value.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @result The value for the parameter with the specified name.
         */
        int32 GetValueAsInt(const char* paramName, int32 defaultValue) const;

        /**
         * Get the value for a parameter with a specified name, as a floating point value.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @result The value for the parameter with the specified name.
         */
        float GetValueAsFloat(const char* paramName, float defaultValue) const;

        /**
         * Get the value for a parameter with a specified name, as a boolean.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, <b>true</b> will be returned!
         * This allows you to make command lines such as "-fullscreen -xres 800 -yres 600", where fullscreen has no value.
         * However, since the parameter fullscreen exists, this most likely means it is a mode that needs to be enabled, so this is why
         * true is being returned in such case. Now you can do things such as:<br>
         *
         * <pre>
         * bool fullScreen = commandLine.GetValueAsBool("FullScreen", false);
         * </pre>
         *
         * Now when "-fullscreen" is not specified, false is returned. But when it has been specified, but without value, true is being used.
         * It is also possible to specify "-fullscreen true" or "-fullscreen 1".
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @result The value for the parameter with the specified name.
         */
        bool GetValueAsBool(const char* paramName, bool defaultValue) const;

        /**
         * Get the value for a parameter with a specified name, as a three component vector.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @result The value for the parameter with the specified name.
         */
        AZ::Vector3 GetValueAsVector3(const char* paramName, const AZ::Vector3& defaultValue) const;

        /**
         * Get the value for a parameter with a specified name, as a four component vector.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param defaultValue The default value to return in case there is no such parameter with the given name, or if its value has not been specified.
         * @result The value for the parameter with the specified name.
         */
        AZ::Vector4 GetValueAsVector4(const char* paramName, const AZ::Vector4& defaultValue) const;

        /**
         * Get the value for a parameter with a specified name.
         * If the parameter with the given name does not exist, the default value is returned.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param command The command to retrieve the default value from (using the command syntax). Returns an empty string if the command syntax can't help.
         * @param outResult The string that will contain the resulting value. This is not allowed to be nullptr.
         */
        void GetValue(const char* paramName, Command* command, AZStd::string* outResult) const;
        void GetValue(const char* paramName, Command* command, AZStd::string& outResult) const;
        const AZStd::string& GetValue(const char* paramName, Command* command) const;

        AZ::Outcome<AZStd::string> GetValueIfExists(const char* paramName, Command* command) const;

        /**
         * Get the value for a parameter with a specified name, as an integer value.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param command The command to retrieve the default value from (using the command syntax). Returns MCORE_INVALIDINDEX32 if the command syntax can't help.
         * @result The value for the parameter with the specified name.
         */
        int32 GetValueAsInt(const char* paramName, Command* command) const;

        /**
         * Get the value for a parameter with a specified name, as a floating point value.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param command The command to retrieve the default value from (using the command syntax). Returns 0.0 if the command syntax can't help.
         * @result The value for the parameter with the specified name.
         */
        float GetValueAsFloat(const char* paramName, Command* command) const;

        /**
         * Get the value for a parameter with a specified name, as a boolean.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, <b>true</b> will be returned!
         * This allows you to make command lines such as "-fullscreen -xres 800 -yres 600", where fullscreen has no value.
         * However, since the parameter fullscreen exists, this most likely means it is a mode that needs to be enabled, so this is why
         * true is being returned in such case. Now you can do things such as:<br>
         *
         * <pre>
         * bool fullScreen = commandLine.GetValueAsBool("FullScreen", false);
         * </pre>
         *
         * Now when "-fullscreen" is not specified, false is returned. But when it has been specified, but without value, true is being used.
         * It is also possible to specify "-fullscreen true" or "-fullscreen 1".
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "XRES".
         * @param command The command to retrieve the default value from (using the command syntax). Returns false if the command syntax can't help.
         * @result The value for the parameter with the specified name.
         */
        bool GetValueAsBool(const char* paramName, Command* command) const;

        /**
         * Get the value for a parameter with a specified name, as a three component vector.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param command The command to retrieve the default value from (using the command syntax). Returns a zero vector if the command syntax can't help.
         * @result The value for the parameter with the specified name.
         */
        AZ::Vector3 GetValueAsVector3(const char* paramName, Command* command) const;

        /**
         * Get the value for a parameter with a specified name, as a four component vector.
         * If the parameter with the given name does not exist, the default value is returned.
         * If the parameter value exists, but the value specified is empty, the default value is returned as well.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to get the value for, for example this can be "THRESHOLD".
         * @param command The command to retrieve the default value from (using the command syntax). Returns a zero vector if the command syntax can't help.
         * @result The value for the parameter with the specified name.
         */
        AZ::Vector4 GetValueAsVector4(const char* paramName, Command* command) const;

        /**
         * Get the number of parameters that have been detected from the command line string that has been passed
         * to the extended constructor or to the SetCommandLine function.
         * @result The number of parameters that have been detected.
         */
        uint32 GetNumParameters() const;

        /**
         * Get the name of a given parameter.
         * @param nr The parameter number, which must be in range of [0 .. GetNumParameters()-1].
         * @result The name of the parameter.
         */
        const AZStd::string& GetParameterName(uint32 nr) const;

        /**
         * Get the value for a given parameter.
         * @param nr The parameter number, which must be in range of [0 .. GetNumParameters()-1].
         * @return The value of the parameter, or "" (an empty string) when no value has been specified.
         */
        const AZStd::string& GetParameterValue(uint32 nr) const;

        /**
         * Find the parameter index for a parameter with a specific name.
         * The parameter name is not case sensitive.
         * @param paramName The name of the parameter to search for.
         * @result The index/number of the parameter, or MCORE_INVALIDINDEX32 when no parameter with the specific name has been found.
         */
        uint32 FindParameterIndex(const char* paramName) const;

        /**
         * Check whether a given parameter has a value specified or not.
         * A value is not specified for parameter that have been defined like "-fullscreen -xres 800 -yres 600".
         * In this example command line, the parameter "fullscreen" has no value. Both "xres" and "yres" parameters have values set.
         * The parameter name is not case sensitive.
         * @param paramName The parameter name to check.
         */
        bool CheckIfHasValue(const char* paramName) const;

        /**
         * Check if the command line contains any parameter with a specified name.
         * The parameter name is not case sensitive.
         * @param paramName The parameter name to check.
         */
        bool CheckIfHasParameter(const char* paramName) const;
        bool CheckIfHasParameter(const AZStd::string& paramName) const;

        /**
         * Specify the command line string that needs to be parsed.
         * The extended constructor, which takes a command line string as parameter already automatically calls
         * this method. Before you can use any other methods of this class, you should make a call to this function.
         * The command line string can be something like "-fullscreen -xres 800 -yres 1024 -threshold 0.145 -culling false".
         * @param commandLine The command line string to parse.
         */
        void SetCommandLine(const AZStd::string& commandLine);

        /**
         * Logs the contents using MCore::LogInfo.
         * This is useful for debugging sometimes.
         * @param debugName The name of this command line, to make it easier to identify this command line when logging multiple ones.
         */
        void Log(const char* debugName = "") const;

    private:
        /**
         * A parameter, which is for example "-XRES 1024", but then split in a name and value.
         * In this case the name would be "XRES" and the value would be "1024".
         */
        struct MCORE_API Parameter
        {
            AZStd::string   mName;              /**< The parameter name, for example "XRES". */
            AZStd::string   mValue;             /**< The parameter value, for example "1024". */
        };

        AZStd::vector<Parameter> m_parameters;    /**< The parameters that have been detected in the command line string. */

        // extract the next parameter, starting from a given offset
        bool ExtractNextParam(const AZStd::string& paramString, AZStd::string& outParamName, AZStd::string& outParamValue, uint32* inOutStartOffset);
    };
}   // namespace MCore

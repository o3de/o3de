/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : This is the interface to access command line arguments.
//               This will avoid the need to parse command line in multiple
//               places  and thus  reduce unnecessary code duplication


#ifndef CRYINCLUDE_CRYCOMMON_ICMDLINE_H
#define CRYINCLUDE_CRYCOMMON_ICMDLINE_H
#pragma once

// The type of command line argument
enum ECmdLineArgType
{
    // Description:
    //      Argument was not preceeded by anything
    eCLAT_Normal = 0,
    // Description:
    //      Argument was preceeded by a minus sign '-'
    eCLAT_Pre,
    // Description:
    //      Argument was preceeded by a plus signe '+'
    eCLAT_Post,
    // Description:
    //      Argument is the executable filename
    eCLAT_Executable,
};

// Container for a command line Argument
class ICmdLineArg
{
public:
    // <interfuscator:shuffle>
    virtual ~ICmdLineArg(){}
    // Description:
    //      Retrieve the name of the argument.
    // Return Value:
    //      The name of the argument.
    virtual const char* GetName() const = 0;

    // Description:
    //      Retrieve the value of the argument.
    // Return Value:
    //      The value of the argument as a null-terminated string.
    virtual const char* GetValue() const = 0;

    // Description:
    //      Retrieve the type of argument.
    // Return Value:
    //      The type of argument.
    // See Also:
    //      ECmdLineArgType
    virtual const ECmdLineArgType GetType() const = 0;

    // Description:
    //      Retrieve the value of the argument.
    // Return Value:
    //      The value of the argument as float number.
    virtual const float GetFValue() const = 0;

    // Description:
    //      Retrieve the value of the argument.
    // Return Value:
    //      The value of the argument as integer number.
    virtual const int GetIValue() const = 0;
    // </interfuscator:shuffle>

    // Description:
    //      Retrieve the value of the argument.
    // Arguments:
    //      cmdLineValue. The cmdline value will be filled out if a valid boolean is found.
    // Return Value:
    //      Returns true if the cmdline arg is actually a boolean string matching "true" or "false"; otherwise return false.
    virtual const bool GetBoolValue(bool& cmdLineValue) const = 0;
};

// Command line interface
class ICmdLine
{
public:
    // <interfuscator:shuffle>
    virtual ~ICmdLine(){}
    // Description:
    //      Returns the n-th command line argument.
    // Arguments:
    //      n - 0 returns executable name, otherwise returns n-th argument.
    // Return Value:
    //      Pointer to the command line argument at index specified by idx.
    // See Also:
    //      ICmdLineArg
    virtual const ICmdLineArg* GetArg(int n) const = 0;

    // Description:
    //      Returns the number of command line arguments.
    // Return Value:
    //      The number of command line arguments.
    virtual int GetArgCount() const = 0;

    // Description:
    //      Finds an argument in the command line.
    // Arguments:
    //      name - the name of the argument to find, excluding any '+' or '-'
    // Return Value:
    //      0 when if the argument was not found.
    //      Pointer to a ICmdLineArg class containing the specified argument.
    // See Also:
    //      ICmdLineArg
    virtual const ICmdLineArg* FindArg(const ECmdLineArgType ArgType, const char* name, bool caseSensitive = false) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ICMDLINE_H

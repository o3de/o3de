/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : IValidator interface used to check objects for warnings and errors
//               Report missing resources or invalid files.


#ifndef CRYINCLUDE_CRYCOMMON_IVALIDATOR_H
#define CRYINCLUDE_CRYCOMMON_IVALIDATOR_H
#pragma once

#   define MAX_WARNING_LENGTH   4096

#if MAX_WARNING_LENGTH < 33
#error "MAX_WARNING_LENGTH should be bigger than 32"
#endif

#define ERROR_CANT_FIND_CENTRAL_DIRECTORY "Cannot find Central Directory Record in pak. This is either not a pak file, or a pak file without Central Directory. It does not mean that the data is permanently lost, but it may be severely damaged. Please repair the file with external tools, there may be enough information left to recover the file completely."

enum EValidatorSeverity
{
    VALIDATOR_ERROR,
    VALIDATOR_ERROR_DBGBRK, // will __debugbreak() if sys_error_debugbreak is 1
    VALIDATOR_WARNING,
    VALIDATOR_COMMENT
};

enum EValidatorModule
{
    VALIDATOR_MODULE_UNKNOWN,
    VALIDATOR_MODULE_RENDERER,
    VALIDATOR_MODULE_3DENGINE,
    VALIDATOR_MODULE_ASSETS,
    VALIDATOR_MODULE_SYSTEM,
    VALIDATOR_MODULE_AUDIO,
    VALIDATOR_MODULE_MOVIE,
    VALIDATOR_MODULE_EDITOR,
    VALIDATOR_MODULE_NETWORK,
    VALIDATOR_MODULE_PHYSICS,
    VALIDATOR_MODULE_RESERVED, // formerly VALIDATOR_MODULE_FLOWGRAPH
    VALIDATOR_MODULE_FEATURETESTS,
    VALIDATOR_MODULE_ONLINE,
    VALIDATOR_MODULE_SHINE,
    VALIDATOR_MODULE_DRS,
};

enum EValidatorFlags
{
    VALIDATOR_FLAG_FILE             = 0x0001, // Indicate that required file was not found or file was invalid.
    VALIDATOR_FLAG_TEXTURE          = 0x0002, // Problem with texture.
    VALIDATOR_FLAG_SCRIPT           = 0x0004, // Problem with script.
    VALIDATOR_FLAG_AUDIO            = 0x0008, // Problem with audio.
    VALIDATOR_FLAG_AI               = 0x0010, // Problem with AI.
    VALIDATOR_FLAG_LOG_ASSET_SCOPE  = 0x0020, // Log asset scope with the warning.
    VALIDATOR_FLAG_IGNORE_IN_EDITOR = 0x0040, // Do not log this with the editor
    VALIDATOR_FLAG_SKIP_VALIDATOR   = 0x0080, // Do not call validator's Report()
};

struct SValidatorRecord
{
    //! Severity of this error.
    EValidatorSeverity severity;
    //! In which module error occured.
    EValidatorModule module;
    //! Error Text.
    const char* text;
    //! File which is missing or causing problem.
    const char* file;
    //! Additional description for this error.
    const char* description;
    //! Asset scope sring
    const char* assetScope;
    //! Flags that suggest kind of error.
    int flags;

    //////////////////////////////////////////////////////////////////////////
    SValidatorRecord()
    {
        module = VALIDATOR_MODULE_UNKNOWN;
        text = NULL;
        file = NULL;
        assetScope = NULL;
        description = NULL;
        severity = VALIDATOR_WARNING;
        flags = 0;
    }
};

/*! This interface will be given to Validate methods of engine, for resources and objects validation.
 */
struct IValidator
{
    // <interfuscator:shuffle>
    virtual ~IValidator(){}
    virtual void Report(SValidatorRecord& record) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IVALIDATOR_H

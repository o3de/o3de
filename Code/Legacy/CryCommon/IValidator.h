/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : IValidator interface used to check objects for warnings and errors
//               Report missing resources or invalid files.
#pragma once

#   define MAX_WARNING_LENGTH   4096

static_assert(MAX_WARNING_LENGTH>32,"MAX_WARNING_LENGTH should be bigger than 32");

#define ERROR_CANT_FIND_CENTRAL_DIRECTORY "Cannot find Central Directory Record in pak. This is either not a pak file, or a pak file without Central Directory. It does not mean that the data is permanently lost, but it may be severely damaged. Please repair the file with external tools, there may be enough information left to recover the file completely."

enum EValidatorSeverity : int
{
    VALIDATOR_ERROR,
    VALIDATOR_ERROR_DBGBRK, // will __debugbreak() if sys_error_debugbreak is 1
    VALIDATOR_WARNING,
    VALIDATOR_COMMENT
};

enum EValidatorModule : int
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ_RESTRICTED_FILE takes an unquoted string and prepends the configured platform's directory and prefix to the filename.
// For instance, AZ_RESTRICTED_FILE(PlatformDef_h, AZ_RESTRICTED_PLATFORM) would become "ringo/PlatformDef_h_ringo.inl" for hypothetical restricted
// platform 'ringo'. This file structure is fixed, and mandates set of platform-specific folders sit within the current directory
// of the file invoking the restricted file machinery.
//
// The structure of this macro is very delicate - it depends solely on preprocessor states of evaluation, and does not run by the
// rules of C++ string concatenation, so where/when/if you stringize is really important.
//
//
#define AZ_RESTRICTED_FILE_STR(f) #f
#define AZ_RESTRICTED_FILE_EVAL(f) AZ_RESTRICTED_FILE_STR(f)
#define AZ_RESTRICTED_FILE_PASTE3(a,b,c) a##b##c
#define AZ_RESTRICTED_FILE_EXPLICIT(f,p) AZ_RESTRICTED_FILE_EVAL(AZ_RESTRICTED_FILE_PASTE3(f,_,p).inl)
#define AZ_RESTRICTED_FILE(f) AZ_RESTRICTED_FILE_EXPLICIT(f,AZ_RESTRICTED_PLATFORM)

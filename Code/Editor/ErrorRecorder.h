/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



// Description : Class that collects error reports to present them later.

#ifndef CRYINCLUDE_EDITOR_CORE_ERRORRECORDER_H
#define CRYINCLUDE_EDITOR_CORE_ERRORRECORDER_H
#pragma once

//////////////////////////////////////////////////////////////////////////
//! Automatic class to record and display error.
class EDITOR_CORE_API CErrorsRecorder
{
public:
    CErrorsRecorder(bool showErrors = true);
    ~CErrorsRecorder();
};

#endif // CRYINCLUDE_EDITOR_ERRORREPORT_H

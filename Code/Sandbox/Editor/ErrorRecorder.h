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

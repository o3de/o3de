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
#ifndef CRYINCLUDE_EDITOR_CORE_UTIL_UNDO_UTIL_H
#define CRYINCLUDE_EDITOR_CORE_UTIL_UNDO_UTIL_H
#pragma once

#include "Include/EditorCoreAPI.h"

struct IUndoObject;
class EDITOR_CORE_API CUndo
{
public:
    CUndo(const char* description);

    ~CUndo();

    void Cancel() { m_bCancelled = true; }

    //! Check if undo is recording.
    static bool IsRecording();

    //! Check if undo is suspended.
    static bool IsSuspended();

    //! Record specified object.
    static void Record(IUndoObject* undo);

private:
    static const uint32 scDescSize = 256;
    char m_description[scDescSize];
    bool m_bCancelled;
    bool m_bStartedRecord;
};

//! CUndoSuspend is a utility undo class
//! Define instance of this class in block of code where you want to suspend undo operations
class EDITOR_CORE_API CUndoSuspend
{
public:
    CUndoSuspend();

    ~CUndoSuspend();
};

#endif // CRYINCLUDE_EDITOR_CORE_UTIL_UNDO_UTIL_H

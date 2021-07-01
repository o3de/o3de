/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

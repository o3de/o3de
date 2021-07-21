/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiAnimUndoManager.h"

//! Helper class for using the UiAnimUndoManager
class UiAnimUndo
{
public:
    UiAnimUndo(const char* description);

    ~UiAnimUndo();

    void Cancel() { m_bCancelled = true; }

    //! Check if undo is recording.
    static bool IsRecording();

    //! Check if undo is suspended.
    static bool IsSuspended();

    //! Record specified object.
    static void Record(UiAnimUndoObject* undo);

private:
    AZStd::string m_description;
    bool m_bCancelled;
    bool m_bStartedRecord;
};

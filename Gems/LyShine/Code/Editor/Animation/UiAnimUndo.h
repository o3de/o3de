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

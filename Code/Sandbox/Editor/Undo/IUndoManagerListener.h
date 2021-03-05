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
#ifndef CRYINCLUDE_EDITORCORE_UNDO_IUNDOMANAGERLISTENER_H
#define CRYINCLUDE_EDITORCORE_UNDO_IUNDOMANAGERLISTENER_H
#pragma once

struct IUndoManagerListener
{
    virtual void BeginUndoTransaction() {}
    virtual void EndUndoTransaction() {}
    virtual void BeginRestoreTransaction() {}
    virtual void EndRestoreTransaction() {}
    virtual void SignalNumUndoRedo([[maybe_unused]] const unsigned int& numUndo, [[maybe_unused]] const unsigned int& numRedo){}
    virtual void UndoStackFlushed() {}
};

#endif // CRYINCLUDE_EDITORCORE_UNDO_IUNDOMANAGERLISTENER_H

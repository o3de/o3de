/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

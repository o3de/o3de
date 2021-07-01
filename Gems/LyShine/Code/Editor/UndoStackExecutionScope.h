/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class UndoStackExecutionScope
{
public:

    UndoStackExecutionScope(UndoStack* stack);
    virtual ~UndoStackExecutionScope();

private:

    UndoStack* m_stack;
};

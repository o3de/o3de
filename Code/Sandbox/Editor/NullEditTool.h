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

#if !defined(Q_MOC_RUN)
#include "EditTool.h"
#endif

/// An EditTool that does nothing - it provides the Null-Object pattern.
class SANDBOX_API NullEditTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE NullEditTool();
    virtual ~NullEditTool() = default;

    static const GUID& GetClassID();
    static void RegisterTool(CRegistrationContext& rc);

    // CEditTool
    void BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags) override {}
    void EndEditParams() override {}
    void Display([[maybe_unused]] DisplayContext& dc) override {}
    bool MouseCallback([[maybe_unused]] CViewport* view, [[maybe_unused]] EMouseEvent event, [[maybe_unused]] QPoint& point, [[maybe_unused]] int flags) override { return false; }
    bool OnKeyDown([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) override { return false; }
    bool OnKeyUp([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) override { return true; }
    void DeleteThis() override { delete this; }
};
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class CEditorAutoLevelLoadTest
    : public IEditorNotifyListener
{
public:
    static CEditorAutoLevelLoadTest& Instance();
private:
    CEditorAutoLevelLoadTest();
    virtual ~CEditorAutoLevelLoadTest();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
};

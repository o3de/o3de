/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "EditorAutoLevelLoadTest.h"

CEditorAutoLevelLoadTest& CEditorAutoLevelLoadTest::Instance()
{
    static CEditorAutoLevelLoadTest levelLoadTest;
    return levelLoadTest;
}

CEditorAutoLevelLoadTest::CEditorAutoLevelLoadTest()
{
    GetIEditor()->RegisterNotifyListener(this);
}

CEditorAutoLevelLoadTest::~CEditorAutoLevelLoadTest()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CEditorAutoLevelLoadTest::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnEndSceneOpen:
        CLogFile::WriteLine("[LevelLoadFinished]");
        exit(0);
        break;
    }
}

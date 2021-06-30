/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"
#include "ErrorRecorder.h"
#include "BaseLibraryItem.h"
#include "Include/IErrorReport.h"


//////////////////////////////////////////////////////////////////////////
CErrorsRecorder::CErrorsRecorder(bool showErrors)
{
    GetIEditor()->GetErrorReport()->SetImmediateMode(false);
    GetIEditor()->GetErrorReport()->SetShowErrors(showErrors);
}

//////////////////////////////////////////////////////////////////////////
CErrorsRecorder::~CErrorsRecorder()
{
    GetIEditor()->GetErrorReport()->Display();
}

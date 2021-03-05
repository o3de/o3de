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

#include "EditorDefs.h"

#include "NullEditTool.h"


NullEditTool::NullEditTool() {}

const GUID& NullEditTool::GetClassID()
{
    // {65AFF87A-34E0-479B-B062-94B1B867B13D}
    static const GUID guid =
    {
        0x65AFF87A, 0x34E0, 0x479B,{ 0xB0, 0x62, 0x94, 0xB1, 0xB8, 0x67, 0xB1, 0x3D }
    };

    return guid;
}

void NullEditTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(
        new CQtViewClass<NullEditTool>("EditTool.NullEditTool", "Select", ESYSTEM_CLASS_EDITTOOL));
}

#include <moc_NullEditTool.cpp>

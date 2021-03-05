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
#include "RenderDll_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include "Common/Shaders/Vertex.h"
#include "DriverD3D.h"

// General vertex stream stride
int32 m_cSizeVF[eVF_Max] =
{
    0,
    
    sizeof(SVF_P3F_C4B_T2F),
    sizeof(SVF_P3F_C4B_T2F_T2F),
    sizeof(SVF_P3S_C4B_T2S),
    sizeof(SVF_P3S_C4B_T2S_T2S),
    sizeof(SVF_P3S_N4B_C4B_T2S),

    sizeof(SVF_P3F_C4B_T4B_N3F2),
    sizeof(SVF_TP3F_C4B_T2F),
    sizeof(SVF_TP3F_T2F_T3F),
    sizeof(SVF_P3F_T3F),
    sizeof(SVF_P3F_T2F_T3F),

    sizeof(SVF_T2F),
    sizeof(SVF_W4B_I4S),
    sizeof(SVF_C4B_C4B),
    sizeof(SVF_P3F_P3F_I4B),
    sizeof(SVF_P3F),

    sizeof(SVF_C4B_T2S),

    sizeof(SVF_P2F_T4F_C4F),
    sizeof(SVF_P2F_T4F_T4F_C4F),

    sizeof(SVF_P2S_N4B_C4B_T1F),
    sizeof(SVF_P3F_C4B_T2S),

    sizeof(SVF_P2F_C4B_T2F_F4B),    
    sizeof(SVF_P3F_C4B),

    sizeof(SVF_P3F_C4F_T2F),  //format number 23 (for testing verification)
    sizeof(SVF_P3F_C4F_T2F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T3F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T1F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F_T3F),
    sizeof(SVF_P3F_C4F_T4F_T2F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F),  //30
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F_T3F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F),  //35
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F),  //40
    sizeof(SVF_P4F_T2F_C4F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T3F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T4F),  //45
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F),  //50
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F),  //55
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F),
    sizeof(SVF_P4F_T2F_C4F_T4F_T4F_T4F),  //60
    sizeof(SVF_P3F_C4F_T2F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F),  //65
    sizeof(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F),  //70
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F),  //75
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F),
    sizeof(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F),
    sizeof(SVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F),
};

// Legacy table copied from RenderMesh.cpp
#define OOFS(t, x) ((int)offsetof(t, x))

SBufInfoTable m_cBufInfoTable[eVF_Max] =
{
    // { Texcoord, Color, Normal }
    {
        -1, -1, -1
    },
    {      //eVF_P3F_C4B_T2F
        OOFS(SVF_P3F_C4B_T2F, st),
        OOFS(SVF_P3F_C4B_T2F, color.dcolor),
        -1
    },

    {      //eVF_P3F_C4B_T2F_T2F
        OOFS(SVF_P3F_C4B_T2F_T2F, st),
        OOFS(SVF_P3F_C4B_T2F_T2F, color.dcolor),
        -1
    },


    {      //eVF_P3S_C4B_T2S
        OOFS(SVF_P3S_C4B_T2S, st),
        OOFS(SVF_P3S_C4B_T2S, color.dcolor),
-1
    },

    {      //eVF_P3S_C4B_T2S_T2S
        OOFS(SVF_P3S_C4B_T2S_T2S, st),
        OOFS(SVF_P3S_C4B_T2S_T2S, color.dcolor),
        -1
    },

    {      //eVF_P3S_N4B_C4B_T2S
        OOFS(SVF_P3S_N4B_C4B_T2S, st),
        OOFS(SVF_P3S_N4B_C4B_T2S, color.dcolor),
        OOFS(SVF_P3S_N4B_C4B_T2S, normal)
    },
    {     // eVF_P3F_C4B_T4B_N3F2
        -1,
        OOFS(SVF_P3F_C4B_T4B_N3F2, color.dcolor),
        -1
    },
    {     // eVF_TP3F_C4B_T2F
        OOFS(SVF_TP3F_C4B_T2F, st),
        OOFS(SVF_TP3F_C4B_T2F, color.dcolor),
        -1
    },
    {     // eVF_TP3F_T2F_T3F
        OOFS(SVF_TP3F_T2F_T3F, st0),
        -1,
        -1
    },
    {     // eVF_P3F_T3F
        OOFS(SVF_P3F_T3F, st),
        -1,
        -1
    },
    {     // eVF_P3F_T2F_T3F
        OOFS(SVF_P3F_T2F_T3F, st0),
        -1,
        -1
    },
    {// eVF_T2F
        OOFS(SVF_T2F, st),
        -1,
        -1
    },
    { -1, -1, -1 },// eVF_W4B_I4S
    { -1, -1, -1 },// eVF_C4B_C4B
    { -1, -1, -1 },// eVF_P3F_P3F_I4B

    {
        -1,
        -1,
        -1
    },
    {     // eVF_C4B_T2S
        OOFS(SVF_C4B_T2S, st),
        OOFS(SVF_C4B_T2S, color.dcolor),
        -1
    },
    {    // eVF_P2F_T4F_C4F
        OOFS(SVF_P2F_T4F_C4F, st),
        OOFS(SVF_P2F_T4F_C4F, color),
        -1
    },
    {   // eVF_P2F_T4F_T4F_C4F
        OOFS(SVF_P2F_T4F_T4F_C4F, st),
        OOFS(SVF_P2F_T4F_T4F_C4F, color),
        -1
    },
    {     // eVF_P2S_N4B_C4B_T1F
        OOFS(SVF_P2S_N4B_C4B_T1F, z),
        OOFS(SVF_P2S_N4B_C4B_T1F, color.dcolor),
        OOFS(SVF_P2S_N4B_C4B_T1F, normal)
    },
    {     // eVF_P3F_C4B_T2S
        OOFS(SVF_P3F_C4B_T2S, st),
        OOFS(SVF_P3F_C4B_T2S, color.dcolor),
        -1
    },
    {     // SVF_P2F_C4B_T2F_F4B
        OOFS(SVF_P2F_C4B_T2F_F4B, st),
        OOFS(SVF_P2F_C4B_T2F_F4B, color.dcolor),
        -1
    },
    {     // eVF_P3F_C4B
        -1,
        OOFS(SVF_P3F_C4B, color.dcolor),
        -1
    },
    {     // eVF_P3F_C4F_T2F
        OOFS(SVF_P3F_C4F_T2F, st),
        OOFS(SVF_P3F_C4F_T2F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F
        OOFS(SVF_P3F_C4F_T2F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F_T3F
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F
        OOFS(SVF_P3F_C4F_T2F_T1F, st),
        OOFS(SVF_P3F_C4F_T2F_T1F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F_T3F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F
        OOFS(SVF_P3F_C4F_T4F_T2F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F_T3F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F, color),
        -1
    },
    {     // eVF_P4F_T2F_C4F_T4F_T4F
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F, st0),
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T4F
        OOFS(SVF_P3F_C4F_T2F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F, color),
        -1
    },
    {     // eVF_P4F_T2F_C4F_T4F_T4F_T4F
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F_T4F, st0),
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F, st0),
        OOFS(SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F, color),
        -1
    },
    {     // eVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F, st0),
        OOFS(SVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F, color),
        -1
    },
};
#undef OOFS

AZStd::vector<D3D11_INPUT_ELEMENT_DESC> Legacy_InitBaseStreamD3DVertexDeclaration(EVertexFormat nFormat)
{
    //========================================================================================
    // base stream declarations (stream 0)
    D3D11_INPUT_ELEMENT_DESC elemPosHalf = { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemTCHalf = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    D3D11_INPUT_ELEMENT_DESC elemPos = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPos2 = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPosTR = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };  // position
    D3D11_INPUT_ELEMENT_DESC elemPos2Half = { "POSITION", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPos1 = { "POSITION", 1, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    D3D11_INPUT_ELEMENT_DESC elemNormalB = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemTan = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };       // axis/size
    D3D11_INPUT_ELEMENT_DESC elemBitan = { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };    // axis/size
    D3D11_INPUT_ELEMENT_DESC elemColor = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };        // diffuse
    D3D11_INPUT_ELEMENT_DESC elemColorF = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };   // general color
    D3D11_INPUT_ELEMENT_DESC elemTC0 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC1 = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC2 = { "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC1_3 = { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC0_4 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC0_1 = { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC3_4 = { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture

    AZ::Vertex::Format vertexFormat = AZ::Vertex::Format((EVertexFormat)nFormat);
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> decl;

    uint texCoordOffset = 0;
    bool hasTexCoord = vertexFormat.TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord);
    uint colorOffset = 0;
    bool hasColor = vertexFormat.TryCalculateOffset(colorOffset, AZ::Vertex::AttributeUsage::Color);
    uint normalOffset = 0;
    bool hasNormal = vertexFormat.TryCalculateOffset(normalOffset, AZ::Vertex::AttributeUsage::Normal);

    // Position

    switch (nFormat)
    {
    case eVF_TP3F_C4B_T2F:
    case eVF_TP3F_T2F_T3F:
        decl.push_back(elemPosTR);
        break;
    case eVF_P3S_C4B_T2S:
    case eVF_P3S_N4B_C4B_T2S:
        decl.push_back(elemPosHalf);
        break;
    case eVF_P2S_N4B_C4B_T1F:
        decl.push_back(elemPos2Half);
        break;
    case eVF_P2F_T4F_C4F:
        decl.push_back(elemPos2);
        break;
    case eVF_T2F:
    case eVF_C4B_T2S:
    case eVF_Unknown:
        break;
    default:
        decl.push_back(elemPos);
    }

    // Normal
    if (hasNormal)
    {
        elemNormalB.AlignedByteOffset = normalOffset;
        decl.push_back(elemNormalB);
    }

#ifdef PARTICLE_MOTION_BLUR
    if (nFormat == eVF_P3F_C4B_T4B_N3F2)
    {
        elemTC0_4.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, prevXaxis);
        elemTC0_4.SemanticIndex = 0;
        decl.AddElem(elemTC0_4);
    }
#endif
    // eVF_P2F_T4F_C4F has special case logic below, so ignore it here
    if (hasColor && nFormat != eVF_P2F_T4F_C4F)
    {
        elemColor.AlignedByteOffset = colorOffset;
        elemColor.SemanticIndex = 0;
        decl.push_back(elemColor);
    }
    if (nFormat == eVF_P3F_C4B_T4B_N3F2)
    {
#ifdef PARTICLE_MOTION_BLUR
        elemTC1_3.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, prevPos);
        elemTC1_3.SemanticIndex = 1;
        decl.push_back(elemTC1_3);
#endif
        elemColor.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, st);
        elemColor.SemanticIndex = 1;
        decl.push_back(elemColor);

        elemTan.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, xaxis);
        decl.push_back(elemTan);

        elemBitan.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, yaxis);
        decl.push_back(elemBitan);
    }

    if (nFormat == eVF_P2F_T4F_C4F)
    {
        elemTC0_4.AlignedByteOffset = (int)offsetof(SVF_P2F_T4F_C4F, st);
        elemTC0_4.SemanticIndex = 0;
        decl.push_back(elemTC0_4);

        elemColorF.AlignedByteOffset = (int)offsetof(SVF_P2F_T4F_C4F, color);
        elemColorF.SemanticIndex = 0;
        decl.push_back(elemColorF);
    }

    //handle cases where 2D texture comes before 4F Color
    if (nFormat == eVF_P4F_T2F_C4F_T4F_T4F || nFormat == eVF_P4F_T2F_C4F_T4F_T4F_T4F || nFormat == eVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F)
    {
        elemTC2.AlignedByteOffset = (int)offsetof(SVF_P4F_T2F_C4F_T4F_T4F, st0);
        elemTC2.SemanticIndex = 0;
        decl.push_back(elemTC2);

        elemColorF.AlignedByteOffset = (int)offsetof(SVF_P4F_T2F_C4F_T4F_T4F, color);
        elemColorF.SemanticIndex = 0;
        decl.push_back(elemColorF);
    }

    if (hasTexCoord)
    {
        elemTC0.AlignedByteOffset = texCoordOffset;
        elemTC0.SemanticIndex = 0;
        switch (nFormat)
        {
        case eVF_P2F_T4F_C4F:
            // eVF_P2F_T4F_C4F has special case logic above, so ignore it here
            break;
        case eVF_P3S_C4B_T2S:
        case eVF_P3S_N4B_C4B_T2S:
        case eVF_C4B_T2S:
        case eVF_P3F_C4B_T2S:
            elemTCHalf.AlignedByteOffset = texCoordOffset;
            elemTCHalf.SemanticIndex = 0;
            decl.push_back(elemTCHalf);
            break;
        case eVF_P3F_T3F:
            elemTC1_3.AlignedByteOffset = texCoordOffset;
            elemTC1_3.SemanticIndex = 0;
            decl.push_back(elemTC1_3);
            break;
        case eVF_P2S_N4B_C4B_T1F:
            elemTC0_1.AlignedByteOffset = texCoordOffset;
            elemTC0_1.SemanticIndex = 0;
            decl.push_back(elemTC0_1);
            break;
        case eVF_TP3F_T2F_T3F:
        case eVF_P3F_T2F_T3F:
            decl.push_back(elemTC0);

            //This case needs two TexCoord elements
            elemTC1_3.AlignedByteOffset = texCoordOffset + 8;
            elemTC1_3.SemanticIndex = 1;
            decl.push_back(elemTC1_3);
            break;
        default:
            decl.push_back(elemTC0);
        }
    }
    if (nFormat == eVF_P2S_N4B_C4B_T1F)
    {
        elemPos1.AlignedByteOffset = (int)offsetof(SVF_P2S_N4B_C4B_T1F, z);
        decl.push_back(elemPos1);
    }
    decl.shrink_to_fit();

    return decl;
}

bool DeclarationsAreEqual(AZStd::vector<D3D11_INPUT_ELEMENT_DESC>& declarationA, AZStd::vector<D3D11_INPUT_ELEMENT_DESC>& declarationB)
{
    if (declarationA.size() != declarationB.size())
    {
        return false;
    }

    for (uint i = 0; i < declarationA.size(); ++i)
    {
        D3D11_INPUT_ELEMENT_DESC elementDescriptionA = declarationA[i];
        D3D11_INPUT_ELEMENT_DESC elementDescriptionB = declarationB[i];
        if (elementDescriptionA.SemanticIndex != elementDescriptionB.SemanticIndex ||
            elementDescriptionA.Format != elementDescriptionB.Format ||
            elementDescriptionA.InputSlot != elementDescriptionB.InputSlot ||
            elementDescriptionA.AlignedByteOffset != elementDescriptionB.AlignedByteOffset ||
            elementDescriptionA.InputSlotClass != elementDescriptionB.InputSlotClass ||
            elementDescriptionA.InstanceDataStepRate != elementDescriptionB.InstanceDataStepRate ||
            strcmp(elementDescriptionA.SemanticName, elementDescriptionB.SemanticName) != 0)
        {
            return false;
        }
    }
    return true;
}

class VertexFormatAssertTest
    : public ::testing::Test
    , public UnitTest::AllocatorsBase
{
protected:
    void SetUp() override
    {
        UnitTest::AllocatorsBase::SetupAllocator();
    }
    void TearDown() override
    {
        UnitTest::AllocatorsBase::TeardownAllocator();
    }
};

TEST_F(VertexFormatAssertTest, VertexFormatConstructor_AssertsOnInvalidInput)
{
    // The vertex format constructor should assert when an invalid vertex format enum is used
    AZ_TEST_START_TRACE_SUPPRESSION;
    AZ::Vertex::Format(static_cast<EVertexFormat>(EVertexFormat::eVF_Max));
    AZ::Vertex::Format(static_cast<EVertexFormat>(EVertexFormat::eVF_Max + 1));
    // Expect 2 asserts
    AZ_TEST_STOP_TRACE_SUPPRESSION(2);
}

class VertexFormatTest
    : public ::testing::TestWithParam < int >
    , public UnitTest::AllocatorsBase
{
public:
    void SetUp() override
    {
        UnitTest::AllocatorsBase::SetupAllocator();
    }

    void TearDown() override
    {
        UnitTest::AllocatorsBase::TeardownAllocator();
    }
};

TEST_P(VertexFormatTest, GetStride_MatchesExpected)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();

    AZ::Vertex::Format format(eVF);
    uint actualSize = format.GetStride();
    uint expectedSize = m_cSizeVF[eVF];
    EXPECT_EQ(actualSize, expectedSize);
}

TEST_P(VertexFormatTest, CalculateOffset_MatchesExpected)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();

    AZ::Vertex::Format format(eVF);
    // TexCoord
    uint actualOffset = 0;
    bool hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::TexCoord);
    int expectedOffset = m_cBufInfoTable[eVF].OffsTC;
    if (expectedOffset >= 0)
    {
        EXPECT_TRUE(hasOffset);
        EXPECT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        EXPECT_FALSE(hasOffset);
    }

    // Color
    hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::Color);
    expectedOffset = m_cBufInfoTable[eVF].OffsColor;
    if (expectedOffset >= 0)
    {
        EXPECT_TRUE(hasOffset);
        EXPECT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        EXPECT_FALSE(hasOffset);
    }

    // Normal
    hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::Normal);
    expectedOffset = m_cBufInfoTable[eVF].OffsNorm;
    if (expectedOffset >= 0)
    {
        EXPECT_TRUE(hasOffset);
        EXPECT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        EXPECT_FALSE(hasOffset);
    }
}

TEST_F(VertexFormatTest, CalculateOffsetMultipleUVs_MatchesExpected)
{
    AZ::Vertex::Format vertexFormat(eVF_P3F_C4B_T2F_T2F);
    uint offset = 0;
    // The first uv set exists
    EXPECT_TRUE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 0));
    // First Texture coordinate comes after the position, which has 3 32 bit floats, and the color, which has 4 bytes
    EXPECT_EQ(offset, AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_3].byteSize + AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Byte_4].byteSize);

    // The second uv set exists
    EXPECT_TRUE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 1));
    // Second Texture coordinate comes after the position + color + the first texture coordinate
    EXPECT_EQ(offset, AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_3].byteSize + AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Byte_4].byteSize + AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_2].byteSize);

    // The third uv set does not exist
    EXPECT_FALSE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 2));
}

TEST_P(VertexFormatTest, D3DVertexDeclarations_MatchesLegacy)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> expected = Legacy_InitBaseStreamD3DVertexDeclaration(eVF);
    bool matchesLegacy = true;

    //the new vertex definitions aren't legacy and never were a part of the legacy system in any way, and should be ignored for this test
    //new definitions occur after entry number 22;
    const unsigned int ignoredFormatsStart = aznumeric_cast<unsigned int>(eVF_P3F_C4F_T2F);  //23
    const unsigned int ignoredFormatsEnd = aznumeric_cast<unsigned int>(eVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F);  //61
    if (aznumeric_cast<unsigned int>(eVF) >= ignoredFormatsStart && aznumeric_cast<unsigned int>(eVF) <= ignoredFormatsEnd)
    {
        EXPECT_TRUE(matchesLegacy);
        return;
    }

    // The legacy result of EF_InitD3DVertexDeclarations for the following formats are flat out wrong (it defaults to one D3D11_INPUT_ELEMENT_DESC that is a position, which is clearly not the case for any of these)
    // eVF_W4B_I4S is really blendweights + indices
    // eVF_C4B_C4B is really two spherical harmonic coefficients
    // eVF_P3F_P3F_I4B is really two positions and an index
    // eVF_P2F_T4F_T4F_C4F doesn't actually exist in the engine anymore
    // ignore these cases
    // Also ignore eVF_P2S_N4B_C4B_T1F: the T1F attribute has a POSITION semantic name in the legacy declaration, even though both the engine and shader treat it as a TEXCOORD (despite the fact that it is eventually used for a position)
    // eVF_P3F_C4B_T2F_T2F, eVF_P3S_C4B_T2S_T2S, and eVF_P2F_C4B_T2F_F4B are all new
    else if (eVF != eVF_W4B_I4S && eVF != eVF_C4B_C4B && eVF != eVF_P3F_P3F_I4B && eVF != eVF_P2F_T4F_T4F_C4F && eVF != eVF_P2S_N4B_C4B_T1F && eVF != eVF_P2F_C4B_T2F_F4B && eVF != eVF_P3F_C4B_T2F_T2F && eVF != eVF_P3S_C4B_T2S_T2S)
    {
        AZStd::vector<D3D11_INPUT_ELEMENT_DESC> actual = GetD3D11Declaration(AZ::Vertex::Format(eVF));
        matchesLegacy = DeclarationsAreEqual(actual, expected);
    }
    EXPECT_TRUE(matchesLegacy);
}

TEST_P(VertexFormatTest, GetStride_4ByteAligned)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();
    AZ::Vertex::Format format(eVF);
    EXPECT_EQ(format.GetStride() % AZ::Vertex::VERTEX_BUFFER_ALIGNMENT, 0);
}

class VertexFormatComparisonTest
    : public ::testing::TestWithParam < int >
    , public UnitTest::AllocatorsBase
{
protected:
    void SetUp() override
    {
        UnitTest::AllocatorsBase::SetupAllocator();
        
        m_vertexFormatEnum = static_cast<EVertexFormat>(GetParam());
        m_vertexFormat = AZ::Vertex::Format(m_vertexFormatEnum);

        m_nextVertexFormatEnum = static_cast<EVertexFormat>((m_vertexFormatEnum + 1) % eVF_Max);
        m_nextVertexFormat = AZ::Vertex::Format(m_nextVertexFormatEnum);
    }
    void TearDown() override
    {
        UnitTest::AllocatorsBase::TeardownAllocator();
    }

    EVertexFormat m_vertexFormatEnum;
    AZ::Vertex::Format m_vertexFormat;

    AZ::Vertex::Format m_nextVertexFormat;
    EVertexFormat m_nextVertexFormatEnum;
};

TEST_P(VertexFormatComparisonTest, EqualTo_SameVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat == m_vertexFormatEnum);
    EXPECT_TRUE(m_vertexFormat == m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, EqualTo_NextVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat == m_nextVertexFormatEnum);
    EXPECT_FALSE(m_vertexFormat == m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, EqualTo_PreviousVertexFormat_False)
{
    EXPECT_FALSE(m_nextVertexFormat == m_vertexFormatEnum);
    EXPECT_FALSE(m_nextVertexFormat == m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, NotEqualTo_SameVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat != m_vertexFormatEnum);
    EXPECT_FALSE(m_vertexFormat != m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, NotEqualTo_NextVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat != m_nextVertexFormatEnum);
    EXPECT_TRUE(m_vertexFormat != m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, NotEqualTo_PreviousVertexFormat_True)
{
    EXPECT_TRUE(m_nextVertexFormat != m_vertexFormatEnum);
    EXPECT_TRUE(m_nextVertexFormat != m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThan_SameVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat > m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThan_NextVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat > m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThan_PreviousVertexFormat_True)
{
    EXPECT_TRUE(m_nextVertexFormat > m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThanOrEqualTo_SameVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat >= m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThanOrEqualTo_NextVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat >= m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, GreaterThanOrEqualTo_PreviousVertexFormat_True)
{
    EXPECT_TRUE(m_nextVertexFormat >= m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThan_SameVertexFormat_False)
{
    EXPECT_FALSE(m_vertexFormat < m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThan_NextVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat < m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThan_PreviousVertexFormat_False)
{
    EXPECT_FALSE(m_nextVertexFormat < m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThanOrEqualTo_SameVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat <= m_vertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThanOrEqualTo_NextVertexFormat_True)
{
    EXPECT_TRUE(m_vertexFormat <= m_nextVertexFormat);
}

TEST_P(VertexFormatComparisonTest, LessThanOrEqualTo_PreviousVertexFormat_False)
{
    EXPECT_FALSE(m_nextVertexFormat <= m_vertexFormat);
}

TEST_P(VertexFormatTest, GetEnum_MatchesExpected)
{
    EVertexFormat eVF = static_cast<EVertexFormat>(GetParam());
    AZ::Vertex::Format vertexFormat(eVF);

    EXPECT_EQ(vertexFormat.GetEnum(), eVF);
}

TEST_F(VertexFormatTest, GetAttributeUsageCount_MatchesExpected)
{
    AZ::Vertex::Format vertexFormatP3F_C4B_T2F(eVF_P3F_C4B_T2F);
    // eVF_P3F_C4B_T2F has one position, one color, one uv set, and no normal attribute
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::Position), 1);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::Color), 1);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord), 1);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::Normal), 0);

    // eVF_P3S_C4B_T2S and eVF_P3F_C4B_T2F_T2F actually have two uv sets
    EXPECT_EQ(AZ::Vertex::Format(eVF_P3S_C4B_T2S_T2S).GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord), 2);
    EXPECT_EQ(AZ::Vertex::Format(eVF_P3F_C4B_T2F_T2F).GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord), 2);
}

TEST_P(VertexFormatTest, IsSupersetOf_EquivalentVertexFormat_True)
{
    EVertexFormat eVF = static_cast<EVertexFormat>(GetParam());
    EXPECT_TRUE(AZ::Vertex::Format(eVF).IsSupersetOf(AZ::Vertex::Format(eVF)));
}

TEST_F(VertexFormatTest, IsSupersetOf_TargetHasExtraUVs_OnlyTargetIsSuperset)
{
    AZ::Vertex::Format vertexFormatP3F_C4B_T2F(eVF_P3F_C4B_T2F);
    AZ::Vertex::Format vertexFormatP3F_C4B_T2F_T2F(eVF_P3F_C4B_T2F_T2F);

    // eVF_P3F_C4B_T2F_T2F contains everything in eVF_P3F_C4B_T2F plus an extra uv set
    EXPECT_TRUE(vertexFormatP3F_C4B_T2F_T2F.IsSupersetOf(vertexFormatP3F_C4B_T2F));
    EXPECT_FALSE(vertexFormatP3F_C4B_T2F.IsSupersetOf(vertexFormatP3F_C4B_T2F_T2F));
}

TEST_F(VertexFormatTest, TryGetAttributeOffsetAndType_MatchesExpected)
{
    uint expectedOffset = 0;
    uint offset = 0;
    AZ::Vertex::AttributeType type = AZ::Vertex::AttributeType::NumTypes;
    AZ::Vertex::Format vertexFormatP3F_C4B_T2F_T2F(eVF_P3F_C4B_T2F_T2F);

    // eVF_P3F_C4B_T2F_T2F has a position at offset 0 that is a Float32_3
    EXPECT_TRUE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::Position, 0, offset, type));
    EXPECT_EQ(offset, expectedOffset);
    EXPECT_EQ(type, AZ::Vertex::AttributeType::Float32_3);
    expectedOffset += AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float32_3)].byteSize;

    // eVF_P3F_C4B_T2F_T2F does not have a second position
    EXPECT_FALSE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::Position, 1, offset, type));

    // eVF_P3F_C4B_T2F_T2F has a color, offset by 12 bytes, that is a Byte_4
    EXPECT_TRUE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::Color, 0, offset, type));
    EXPECT_EQ(offset, expectedOffset);
    EXPECT_EQ(type, AZ::Vertex::AttributeType::Byte_4);
    expectedOffset += AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Byte_4)].byteSize;

    // eVF_P3F_C4B_T2F_T2F has a TexCoord, offset by 16 bytes, that is a Float32_2
    EXPECT_TRUE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::TexCoord, 0, offset, type));
    EXPECT_EQ(offset, expectedOffset);
    EXPECT_EQ(type, AZ::Vertex::AttributeType::Float32_2);
    expectedOffset += AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float32_2)].byteSize;

    // eVF_P3F_C4B_T2F_T2F has a second TexCoord, offset by 24 bytes, that is a Float32_2
    EXPECT_TRUE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::TexCoord, 1, offset, type));
    EXPECT_EQ(offset, expectedOffset);
    EXPECT_EQ(type, AZ::Vertex::AttributeType::Float32_2);
    expectedOffset += AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float32_2)].byteSize;

    // eVF_P3F_C4B_T2F_T2F does not have a third TexCoord
    EXPECT_FALSE(vertexFormatP3F_C4B_T2F_T2F.TryGetAttributeOffsetAndType(AZ::Vertex::AttributeUsage::TexCoord, 2, offset, type));
}

TEST_F(VertexFormatTest, GetAttributeByteLength_MatchesExpected)
{
    AZ::Vertex::Format vertexFormatP3F_C4B_T2F(eVF_P3F_C4B_T2F);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Position), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float32_3)].byteSize);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Color), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Byte_4)].byteSize);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeByteLength(AZ::Vertex::AttributeUsage::TexCoord), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float32_2)].byteSize);
    EXPECT_EQ(vertexFormatP3F_C4B_T2F.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Normal), 0);

    AZ::Vertex::Format vertexFormatP3S_C4B_T2S(eVF_P3S_C4B_T2S);
    EXPECT_EQ(vertexFormatP3S_C4B_T2S.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Position), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float16_4)].byteSize);// vec3f16 is backed by a CryHalf4, so 16 bit positions use Float16_4
    EXPECT_EQ(vertexFormatP3S_C4B_T2S.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Color), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Byte_4)].byteSize);
    EXPECT_EQ(vertexFormatP3S_C4B_T2S.GetAttributeByteLength(AZ::Vertex::AttributeUsage::TexCoord), AZ::Vertex::AttributeTypeDataTable[static_cast<uint8>(AZ::Vertex::AttributeType::Float16_2)].byteSize);
    EXPECT_EQ(vertexFormatP3S_C4B_T2S.GetAttributeByteLength(AZ::Vertex::AttributeUsage::Normal), 0);
}

TEST_F(VertexFormatTest, Has16BitFloatPosition_MatchesExpected)
{
    // Vertex formats with 16 bit positions should return true
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3S_C4B_T2S).Has16BitFloatPosition());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3S_C4B_T2S_T2S).Has16BitFloatPosition());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3S_N4B_C4B_T2S).Has16BitFloatPosition());

    // Vertex formats with 32 bit positions should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2F).Has16BitFloatPosition());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2F_T2F).Has16BitFloatPosition());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T4B_N3F2).Has16BitFloatPosition()); 
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_T3F).Has16BitFloatPosition());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2S).Has16BitFloatPosition());

    // Vertex formats with no positions should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_T2F).Has16BitFloatPosition());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_W4B_I4S).Has16BitFloatPosition());
}

TEST_F(VertexFormatTest, Has16BitFloatTextureCoordinates_MatchesExpected)
{
    // Vertex formats with 16 bit texture coordinates should return true
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3S_C4B_T2S).Has16BitFloatTextureCoordinates());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3S_C4B_T2S_T2S).Has16BitFloatTextureCoordinates());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3F_C4B_T2S).Has16BitFloatTextureCoordinates());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_C4B_T2S).Has16BitFloatTextureCoordinates());

    // Vertex formats with 32 bit texture coordinates should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2F).Has16BitFloatTextureCoordinates());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2F_T2F).Has16BitFloatTextureCoordinates());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_T2F).Has16BitFloatTextureCoordinates());
    
    // Vertex formats with no texture coordinates should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_W4B_I4S).Has16BitFloatTextureCoordinates());
}

TEST_F(VertexFormatTest, Has32BitFloatTextureCoordinates_MatchesExpected)
{
    // Vertex formats with 16 bit texture coordinates should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3S_C4B_T2S).Has32BitFloatTextureCoordinates());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3S_C4B_T2S_T2S).Has32BitFloatTextureCoordinates());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_P3F_C4B_T2S).Has32BitFloatTextureCoordinates());
    EXPECT_FALSE(AZ::Vertex::Format(eVF_C4B_T2S).Has32BitFloatTextureCoordinates());

    // Vertex formats with 32 bit texture coordinates should return true
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3F_C4B_T2F).Has32BitFloatTextureCoordinates());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_P3F_C4B_T2F_T2F).Has32BitFloatTextureCoordinates());
    EXPECT_TRUE(AZ::Vertex::Format(eVF_T2F).Has32BitFloatTextureCoordinates());

    // Vertex formats with no texture coordinates should return false
    EXPECT_FALSE(AZ::Vertex::Format(eVF_W4B_I4S).Has32BitFloatTextureCoordinates());
}

TEST_F(VertexFormatTest, VertFormatForComponents_StandardWithOneUVSet_eVF_P3S_C4B_T2S)
{
    // Standard vertex format
    bool hasTextureCoordinate = true;
    bool hasTextureCoordinate2 = false;
    bool isParticle = false;
    bool hasNormal = false;
    EXPECT_EQ(AZ::Vertex::VertFormatForComponents(false, hasTextureCoordinate, hasTextureCoordinate2, isParticle, hasNormal), eVF_P3S_C4B_T2S);
}

TEST_F(VertexFormatTest, VertFormatForComponents_StandardWithTwoUVSets_eVF_P3F_C4B_T2F_T2F)
{
    // Multi-uv set vertex format
    bool hasTextureCoordinate = true;
    bool hasTextureCoordinate2 = true;
    bool isParticle = false;
    bool hasNormal = false;
    EXPECT_EQ(AZ::Vertex::VertFormatForComponents(false, hasTextureCoordinate, hasTextureCoordinate2, isParticle, hasNormal), eVF_P3F_C4B_T2F_T2F);
}

TEST_F(VertexFormatTest, VertFormatForComponents_IsParticle_eVF_P3F_C4B_T4B_N3F2)
{
    // Particle vertex format
    bool hasTextureCoordinate = true;
    bool hasTextureCoordinate2 = false;
    bool isParticle = true;
    bool hasNormal = false;
    EXPECT_EQ(AZ::Vertex::VertFormatForComponents(false, hasTextureCoordinate, hasTextureCoordinate2, isParticle, hasNormal), eVF_P3F_C4B_T4B_N3F2);
}

TEST_F(VertexFormatTest, VertFormatForComponents_HasNormal_eVF_P3S_N4B_C4B_T2S)
{
    // Vertex format with normals
    bool hasTextureCoordinate = true;
    bool hasTextureCoordinate2 = false;
    bool isParticle = false;
    bool hasNormal = true;
    EXPECT_EQ(AZ::Vertex::VertFormatForComponents(false, hasTextureCoordinate, hasTextureCoordinate2, isParticle, hasNormal), eVF_P3S_N4B_C4B_T2S);
}

// Instantiate tests
// Start with 1 to skip eVF_Unknown
INSTANTIATE_TEST_CASE_P(EVertexFormatValues, VertexFormatTest, ::testing::Range<int>(1, eVF_Max));

// Start with 1 to skip eVF_Unknown, up to but not including eVF_Max - 1 so we always know that the current value + 1 is within the range of valid vertex formats
INSTANTIATE_TEST_CASE_P(EVertexFormatValues, VertexFormatComparisonTest, ::testing::Range<int>(1, eVF_Max - 1));

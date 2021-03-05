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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Part of CryEngine's extension framework.


#ifndef CRYINCLUDE_CRYEXTENSION_IMPL_CRYGUIDHELPER_H
#define CRYINCLUDE_CRYEXTENSION_IMPL_CRYGUIDHELPER_H
#pragma once


#include "../CryGUID.h"
#include "../../CryString.h"


namespace CryGUIDHelper
{
    string Print(const CryGUID& val)
    {
        char buf[39]; // sizeof("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}")

        static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        char* p = buf;
        *p++ = '{';
        for (int i = 15; i >= 8; --i)
        {
            *p++ = hex[(unsigned char) ((val.hipart >> (i << 2)) & 0xF)];
        }
        *p++ = '-';
        for (int i = 7; i >= 4; --i)
        {
            *p++ = hex[(unsigned char) ((val.hipart >> (i << 2)) & 0xF)];
        }
        *p++ = '-';
        for (int i = 3; i >= 0; --i)
        {
            *p++ = hex[(unsigned char) ((val.hipart >> (i << 2)) & 0xF)];
        }
        *p++ = '-';
        for (int i = 15; i >= 12; --i)
        {
            *p++ = hex[(unsigned char) ((val.lopart >> (i << 2)) & 0xF)];
        }
        *p++ = '-';
        for (int i = 11; i >= 0; --i)
        {
            *p++ = hex[(unsigned char) ((val.lopart >> (i << 2)) & 0xF)];
        }
        *p++ = '}';
        *p++ = '\0';

        return string(buf);
    }
}


#endif // CRYINCLUDE_CRYEXTENSION_IMPL_CRYGUIDHELPER_H

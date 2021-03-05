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

#include <platform.h>
#include "XMLPakFileSink.h"
#include "StringHelpers.h"

XMLPakFileSink::XMLPakFileSink(IPakSystem* pakSystem, const string& archivePath, const string& filePath)
    :   pakSystem(pakSystem)
    , filePath(filePath)
{
    archive = pakSystem->OpenArchive(archivePath.c_str());
}

XMLPakFileSink::~XMLPakFileSink()
{
    if (archive && pakSystem)
    {
        SYSTEMTIME st;
        GetSystemTime(&st);

        FILETIME ft;
        ZeroStruct(ft);
        const BOOL ok = SystemTimeToFileTime(&st, &ft);

        LARGE_INTEGER lt;
        lt.HighPart = ft.dwHighDateTime;
        lt.LowPart = ft.dwLowDateTime;

        const __int64 modTime = lt.QuadPart;
        ;

        pakSystem->AddToArchive(archive, filePath.c_str(), &data[0], int(data.size()), modTime);
        pakSystem->CloseArchive(archive);
    }
}

void XMLPakFileSink::Write(const char* text)
{
    string asciiText = text;
    int len = int(asciiText.size());
    int start = int(data.size());
    data.resize(data.size() + len);
    memcpy(&data[start], asciiText.c_str(), len);
}

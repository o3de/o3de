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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_XMLPAKFILESINK_H
#define CRYINCLUDE_CRYCOMMONTOOLS_XMLPAKFILESINK_H
#pragma once


#include "XMLWriter.h"
#include "IPakSystem.h"

class XMLPakFileSink
    : public IXMLSink
{
public:
    XMLPakFileSink(IPakSystem* pakSystem, const string& archivePath, const string& filePath);
    ~XMLPakFileSink();

    // IXMLSink
    virtual void Write(const char* text);

private:
    IPakSystem* pakSystem;
    PakSystemArchive* archive;
    string filePath;
    std::vector<char> data;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_XMLPAKFILESINK_H

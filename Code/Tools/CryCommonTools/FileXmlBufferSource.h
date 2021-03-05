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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_FILEXMLBUFFERSOURCE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_FILEXMLBUFFERSOURCE_H
#pragma once


class FileXmlBufferSource
    : public IXmlBufferSource
{
public:
    FileXmlBufferSource(const char* path)
    {
        file = std::fopen(path, "r");
    }
    ~FileXmlBufferSource()
    {
        if (file)
        {
            std::fclose(file);
        }
    }

    virtual int Read(void* buffer, int size) const
    {
        if (!file)
        {
            return 0;
        }
        return std::fread(buffer, 1, size, file);
    }

private:
    mutable std::FILE* file;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_FILEXMLBUFFERSOURCE_H

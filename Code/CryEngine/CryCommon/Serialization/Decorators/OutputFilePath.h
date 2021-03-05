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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_OUTPUTFILEPATH_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_OUTPUTFILEPATH_H
#pragma once

#include "Serialization/Strings.h"

namespace Serialization
{
    class IArchive;

    struct OutputFilePath
    {
        string* m_path;
        // if we don't use dynamic filters we could replace following two with const char*
        string filter;
        string startFolder;

        // filters are defined in the following format:
        // "All Images (bmp, jpg, tga)|*.bmp;*.jpg;*.tga|Targa (tga)|*.tga"
        explicit OutputFilePath(string& path, const char* filter = "All files|*.*", const char* startFolder = "")
            : m_path(&path)
            , filter(filter)
            , startFolder(startFolder)
        {
        }

        // the function should stay virtual to ensure cross-dll calls are using right heap
        virtual void SetPath(const char* path) { *this->m_path = path; }
    };

    bool Serialize(Serialization::IArchive& ar, Serialization::OutputFilePath& value, const char* name, const char* label);
}

#include "OutputFilePathImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_OUTPUTFILEPATH_H

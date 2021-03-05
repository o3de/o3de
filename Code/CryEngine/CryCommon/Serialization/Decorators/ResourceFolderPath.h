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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATH_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATH_H
#pragma once

#include "Serialization/Strings.h"

namespace Serialization
{
    class IArchive;

    struct ResourceFolderPath
    {
        string* m_path;
        string startFolder;

        explicit ResourceFolderPath(string& path, const char* startFolder = "")
            : m_path(&path)
            , startFolder(startFolder)
        {
        }

        // the function should stay virtual to ensure cross-dll calls are using right heap
        virtual void SetPath(const char* path) { *this->m_path = path; }
    };

    bool Serialize(Serialization::IArchive& ar, Serialization::ResourceFolderPath& value, const char* name, const char* label);
}

#include "ResourceFolderPathImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCEFOLDERPATH_H

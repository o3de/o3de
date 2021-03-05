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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLISTIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLISTIMPL_H
#pragma once

#include "StringList.h"
#include "IArchive.h"
#include "DynArray.h"
#include "STL.h"

namespace Serialization {
    // ---------------------------------------------------------------------------
    inline void splitStringList(StringList* result, const char* str, char delimeter)
    {
        result->clear();

        const char* ptr = str;
        for (; *ptr; ++ptr)
        {
            if (*ptr == delimeter)
            {
                result->push_back(string(str, ptr));
                str = ptr + 1;
            }
        }
        result->push_back(string(str, ptr));
    }

    inline void joinStringList(string* result, const StringList& stringList, char sep)
    {
        YASLI_ESCAPE(result != 0, return );
        result->clear();
        for (StringList::const_iterator it = stringList.begin(); it != stringList.end(); ++it)
        {
            if (!result->empty())
            {
                result += sep;
            }
            result->append(*it);
        }
    }

    inline void joinStringList(string* result, const StringListStatic& stringList, char sep)
    {
        YASLI_ESCAPE(result != 0, return );
        result->clear();
        for (StringListStatic::const_iterator it = stringList.begin(); it != stringList.end(); ++it)
        {
            if (!result->empty())
            {
                (*result) += sep;
            }
            YASLI_ESCAPE(*it != 0, continue);
            result->append(*it);
        }
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::StringList& value, const char* name, const char* label)
    {
#ifdef SERIALIZATION_STANDALONE
        return ar(static_cast<std::vector<Serialization::string>&>(value), name, label);
#else
        return ar(static_cast<DynArray<Serialization::string>&>(value), name, label);
#endif
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::StringListValue& value, const char* name, const char* label)
    {
        using Serialization::string;
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(value), name, label);
        }
        else
        {
            string str;
            if (ar.IsOutput())
            {
                str = value.c_str();
            }
            if (ar(str, name, label) && ar.IsInput())
            {
                value = str.c_str();
                return true;
            }
            return false;
        }
    }

    inline bool Serialize(Serialization::IArchive& ar, Serialization::StringListStaticValue& value, const char* name, const char* label)
    {
        using Serialization::string;
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(value), name, label);
        }
        else
        {
            string str;
            if (ar.IsOutput())
            {
                str = value.c_str();
            }
            if (ar(str, name, label) && ar.IsInput())
            {
                value = str.c_str();
                return true;
            }
            return true;
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLISTIMPL_H

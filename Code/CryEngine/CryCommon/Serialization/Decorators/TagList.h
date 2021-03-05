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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLIST_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLIST_H
#pragma once

#include <vector>
#include "Serialization/Strings.h"

namespace Serialization {
    class IArchive;
}

struct ITagSource
{
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual unsigned int TagCount(unsigned int group) const = 0;
    virtual const char* TagValue(unsigned int group, unsigned int index) const = 0;
    virtual const char* TagDescription(unsigned int group, unsigned int index) const = 0;
    virtual const char* GroupName(unsigned int group) const = 0;
    virtual unsigned int GroupCount() const = 0;
};

struct TagList
{
    std::vector<Serialization::string>* tags;

    TagList(std::vector<Serialization::string>& tags)
        : tags(&tags)
    {
    }
};

bool Serialize(Serialization::IArchive& ar, TagList& tagList, const char* name, const char* label);

#include "TagListImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLIST_H

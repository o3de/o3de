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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLISTIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLISTIMPL_H
#pragma once

#include <vector>
#include "Serialization/IArchive.h"
#include "Serialization/Strings.h"
#include "Serialization/STL.h"

struct TagListContainer
    : Serialization::ContainerSTL<std::vector<Serialization::string>, Serialization::string>
{
    TagListContainer(TagList& tagList)
        : ContainerSTL(tagList.tags)
    {
    }

    Serialization::TypeID containerType() const override { return Serialization::TypeID::get<TagList>(); };
};

inline bool Serialize(Serialization::IArchive& ar, TagList& tagList, const char* name, const char* label)
{
    TagListContainer container(tagList);
    return ar(static_cast<Serialization::IContainer&>(container), name, label);
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_TAGLISTIMPL_H

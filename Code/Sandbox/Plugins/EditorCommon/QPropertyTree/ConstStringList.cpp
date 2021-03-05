/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

// Modifications copyright Amazon.com, Inc. or its affiliates.

#include "EditorCommon_precompiled.h"
#include "ConstStringList.h"
#include <algorithm>
#include "Serialization/STL.h"
#include "Serialization/IArchive.h"
#include "Serialization/STLImpl.h"

ConstStringList globalConstStringList;

const char* ConstStringList::findOrAdd(const char* string)
{
    // TODO: try sorted vector of const char*
    Strings::iterator it = std::find(strings_.begin(), strings_.end(), string);
    if (it == strings_.end()) {
        strings_.push_back(string);
        return strings_.back().c_str();
    }
    else {
        return it->c_str();
    }
}


ConstStringWrapper::ConstStringWrapper(ConstStringList* list, const char*& string)
    : list_(list ? list : &globalConstStringList)
    , string_(string)
{
    YASLI_ASSERT(string_);
}

using Serialization::string;

bool Serialize(Serialization::IArchive& ar, ConstStringWrapper& val, const char* name, const char* label)
{
    if (ar.IsOutput()) {
        YASLI_ASSERT(val.string_);
        string out = val.string_;
        return ar(out, name, label);
    }
    else {
        string in;
        bool result = ar(in, name, label);

        val.string_ = val.list_->findOrAdd(in.c_str());
        return result;
    }
}

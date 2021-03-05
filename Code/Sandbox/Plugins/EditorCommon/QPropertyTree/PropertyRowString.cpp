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

#include "EditorCommon_precompiled.h"
#include <math.h>

#include "PropertyRowString.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"

#include "Serialization/IArchive.h"
#include "Serialization/ClassFactory.h"
#include <QMenu>
#include "Unicode.h"

// ---------------------------------------------------------------------------
SERIALIZATION_CLASS_NAME(PropertyRow, PropertyRowString, "PropertyRowString", "string");

bool PropertyRowString::assignTo(string& str) const
{
    str = fromWideChar(value_.c_str());
    return true;
}

bool PropertyRowString::assignTo(wstring& str) const
{
    str = value_;
    return true;
}

PropertyRowWidget* PropertyRowString::createWidget(QPropertyTree* tree)
{
    return new PropertyRowWidgetString(this, tree);
}

bool PropertyRowString::assignToByPointer(void* instance, const Serialization::TypeID& type) const
{
    if (type == Serialization::TypeID::get<string>())
    {
        assignTo(*(string*)instance);
        return true;
    }
    else if (type == Serialization::TypeID::get<wstring>())
    {
        assignTo(*(wstring*)instance);
        return true;
    }
    return false;
}

string PropertyRowString::valueAsString() const
{
    return fromWideChar(value_.c_str());
}

void PropertyRowString::setValue(const wchar_t* str, const void* handle, const Serialization::TypeID& type)
{
    value_ = str;
    serializer_.setPointer((void*)handle);
    serializer_.setType(type);
}

void PropertyRowString::setValue(const char* str, const void* handle, const Serialization::TypeID& type)
{
    value_ = toWideChar(str);
    serializer_.setPointer((void*)handle);
    serializer_.setType(type);
}

void PropertyRowString::serializeValue(Serialization::IArchive& ar)
{
    ar(value_, "value", "Value");
}

#include <QPropertyTree/moc_PropertyRowString.cpp>
// vim:ts=4 sw=4:

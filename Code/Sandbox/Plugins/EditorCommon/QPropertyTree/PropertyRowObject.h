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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOBJECT_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOBJECT_H
#pragma once
#include "PropertyRow.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/Pointers.h"
#include "Serialization/Object.h"

namespace Serialization {
    class IArchive;
    struct SStruct;
    class MemoryWriter;
};

class PropertyRowObject
    : public PropertyRow
{
public:
    PropertyRowObject();
    ~PropertyRowObject();

    using PropertyRow::setValueAndContext;
    using PropertyRow::assignTo;

    void setValueAndContext(const Serialization::Object& obj, [[maybe_unused]] Serialization::IArchive& ar) { object_ = obj; }
    void setModel(PropertyTreeModel* model) { model_ = model; }
    bool isObject() const override { return true; }
    bool assignTo(Serialization::Object* obj);
    void Serialize(Serialization::IArchive& ar);
    const Serialization::Object& object() const{ return object_; }
protected:

    Serialization::Object object_;
    PropertyTreeModel* model_;
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWOBJECT_H

/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */
// Modifications copyright Amazon.com, Inc. or its affiliates.

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWIMPL_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWIMPL_H
#pragma once

#include "Serialization/STL.h"
#include "PropertyRowField.h"
#include "Serialization.h"

template<class Type>
class PropertyRowImpl;

template<class Type>
class PropertyRowImpl : public PropertyRowField{
public:
    bool assignTo(const Serialization::SStruct& ser) const override {
        *reinterpret_cast<Type*>(ser.pointer()) = value();
        return true;
    }
    bool isLeaf() const override{ return true; }
    bool isStatic() const override{ return false; }
    void setValue(const Type& value) { value_ = value; }
    Type& value() { return value_; }
    const Type& value() const{ return value_; }

    void setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) override {
        YASLI_ESCAPE(ser.size() == sizeof(Type), return);
        value_ = *(Type*)(ser.pointer());
    }

    void serializeValue(Serialization::IArchive& ar) override{
        ar(value_, "value", "Value");
    }
protected:
    Type value_; 
};


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_PROPERTYROWIMPL_H

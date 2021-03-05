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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ICLASSFACTORY_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ICLASSFACTORY_H
#pragma once

#include <map>

#include "Serialization/Assert.h"
#include "Serialization/TypeID.h"

namespace Serialization {
    class IArchive;
    class TypeDescription
    {
    public:
        TypeDescription(const char* name, const char* label)
            : name_(name)
            , label_(label)
        {
        }
        const char* name() const{ return name_; }
        const char* label() const{ return label_; }

    protected:
        const char* name_;
        const char* label_;
    };

    class IClassFactory
    {
        friend class ClassFactoryManager;
    public:
        IClassFactory(TypeID baseType)
            : baseType_(baseType)
            , nullLabel_(0)
        {
        }

        virtual ~IClassFactory() { }

        virtual size_t size() const = 0;
        virtual const TypeDescription* descriptionByIndex(int index) const = 0;
        virtual const TypeDescription* descriptionByRegisteredName(const char* typeName) const = 0;
        virtual const char* findAnnotation(const char* registeredTypeName, const char* annotationName) const = 0;
        virtual void serializeNewByIndex(IArchive& ar, int index, const char* name, const char* label) = 0;

        bool setNullLabel(const char* label){ nullLabel_ = label ? label : ""; return true; }
        const char* nullLabel() const{ return nullLabel_; }
    protected:
        TypeID baseType_;
        const char* nullLabel_;
        IClassFactory* m_next = nullptr;
    };


    struct TypeNameWithFactory
    {
        string registeredName;
        IClassFactory* factory;

        TypeNameWithFactory(const char* _registeredName, IClassFactory* _factory = 0)
            : registeredName(_registeredName)
            , factory(_factory)
        {
        }
    };

    bool Serialize(Serialization::IArchive& ar, Serialization::TypeNameWithFactory& value, const char* name, const char* label);
}

// vim:ts=4 sw=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ICLASSFACTORY_H

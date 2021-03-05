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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERSIMPL_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERSIMPL_H
#pragma once


#include "Pointers.h"
#include "Serialization/IClassFactory.h"
#include "Serialization/ClassFactory.h"

namespace Serialization {
    template<class T>
    class SharedPtrSerializer
        : public IPointer
    {
    public:
        SharedPtrSerializer(SharedPtr<T>& ptr)
            : ptr_(ptr)
        {}

        const char* registeredTypeName() const
        {
            if (ptr_)
            {
                return ClassFactory<T>::the().getRegisteredTypeName(ptr_.get());
            }
            else
            {
                return "";
            }
        }
        void create(const char* typeName) const
        {
            YASLI_ASSERT(!ptr_ || ptr_->refCount() == 1);
            if (typeName && typeName[0] != '\0')
            {
                ptr_.set(factory()->create(typeName));
            }
            else
            {
                ptr_.set((T*)0);
            }
        }
        TypeID baseType() const { return TypeID::get<T>(); }
        virtual SStruct serializer() const
        {
            return SStruct(*ptr_);
        }
        void* get() const
        {
            return reinterpret_cast<void*>(ptr_.get());
        }
        const void* handle() const
        {
            return &ptr_;
        }
        TypeID pointerType() const
        {
            return TypeID::get<SharedPtr<T> >();
        }
        virtual ClassFactory<T>* factory() const{ return &ClassFactory<T>::the(); }

    protected:
        SharedPtr<T>& ptr_;
    };

    template<class T>
    class PolyPtrSerializer
        : public IPointer
    {
    public:
        PolyPtrSerializer(PolyPtr<T>& ptr)
            : ptr_(ptr)
        {}

        TypeID type() const
        {
            if (ptr_)
            {
                return TypeID::get(ptr_.get());
            }
            else
            {
                return TypeID();
            }
        }
        void create(TypeID type) const
        {
            // YASLI_ASSERT(!ptr_ || ptr_->refCount() == 1); not necessary to be true
            if (type)
            {
                ptr_.set(ClassFactory<T>::the().create(type));
            }
            else
            {
                ptr_.set((T*)0);
            }
        }
        TypeID baseType() const { return TypeID::get<T>(); }
        virtual SStruct serializer() const
        {
            return SStruct(*ptr_);
        }
        void* get() const
        {
            return reinterpret_cast<void*>(ptr_.get());
        }
        IClassFactory* factory() const { return &ClassFactory<T>::the(); }

    protected:
        PolyPtr<T>& ptr_;
    };
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SharedPtr<T>& ptr, const char* name, const char* label)
{
    return ar(static_cast<Serialization::IPointer&>(Serialization::SharedPtrSerializer<T>(ptr)), name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::PolyPtr<T>& ptr, const char* name, const char* label)
{
    return ar(static_cast<Serialization::IPointer&>(Serialization::PolyPtrSerializer<T>(ptr)), name, label);
}
// vim:sw=4 ts=4:

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERSIMPL_H

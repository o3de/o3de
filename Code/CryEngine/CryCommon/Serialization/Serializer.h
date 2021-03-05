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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZER_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZER_H
#pragma once


#include <vector>
#include "Assert.h"
#include "TypeID.h"

namespace Serialization {
    class IArchive;
    class IClassFactory;

    typedef bool(* SerializeStructFunc)(void*, IArchive&);

    typedef bool(* SerializeContainerFunc)(void*, IArchive&, size_t index);
    typedef size_t(* ContainerResizeFunc)(void*, size_t size);
    typedef size_t(* ContainerSizeFunc)(void*);

    // Struct serializer.
    //
    // This type is used to pass needed struct/class type information through abstract interface.
    // Most importantly it captures:
    // - pointer to object
    // - reference to serialize method (indirectly through pointer to static func.)
    // - TypeID
    struct SStruct/*{{{*/
    {
        friend class IArchive;
    public:
        SStruct()
            : object_(0)
            , size_(0)
            , serializeFunc_(0)
        {
        }

        SStruct(TypeID type, void* object, size_t size, SerializeStructFunc Serialize)
            : type_(type)
            , object_(object)
            , size_(size)
            , serializeFunc_(Serialize)
        {
            YASLI_ASSERT(object != 0);
        }

        SStruct(const SStruct& _original)
            : type_(_original.type_)
            , object_(_original.object_)
            , size_(_original.size_)
            , serializeFunc_(_original.serializeFunc_)
        {
        }

        template<class T>
        explicit SStruct(const T& object)
        {
            type_ =  TypeID::get<T>();
            object_ = (void*)(&object);
            size_ = sizeof(T);
            serializeFunc_ = &SStruct::serializeRaw<T>;
        }

        template<class T>
        explicit SStruct(const T& object, TypeID type)
        {
            type_ =  type;
            object_ = (void*)(&object);
            size_ = sizeof(T);
            serializeFunc_ = &SStruct::serializeRaw<T>;
        }

        // This constructs SStruct from an object that doesn't have Serialize method.
        // Such SStruct can not be serialized but conveys object reference and type
        // information that is needed for Property-archives. Used for decorators.
        template<class T>
        static SStruct ForEdit(const T& object)
        {
            SStruct r;
            r.type_ = TypeID::get<T>();
            r.object_ = (void*)&object;
            r.size_ = sizeof(T);
            r.serializeFunc_ = 0;
            return r;
        }

        bool operator()(IArchive& ar, const char* name, const char* label) const;
        bool operator()(IArchive& ar) const;
        operator bool() const{
            return object_ != 0;
        }
        bool operator==(const SStruct& rhs) const{ return object_ == rhs.object_ && serializeFunc_ == rhs.serializeFunc_; }
        bool operator!=(const SStruct& rhs) const{ return !operator==(rhs); }
        void* pointer() const{ return object_; }
        void setPointer(void* p) { object_ = p; }
        TypeID type() const{ return type_; }
        void setType(const TypeID& type) { type_ = type; }
        size_t size() const{ return size_; }
        SerializeStructFunc serializeFunc() const{ return serializeFunc_; }

        template<class T>
        static bool serializeRaw(void* rawPointer, IArchive& ar)
        {
            YASLI_ESCAPE(rawPointer, return false);
            // If you're getting compile error here, most likely, you have one of the following situations:
            // - The type you're trying to serialize doesn't have Serialize _method_ implemented.
            // - Type is supposed to be serialized with non-member Serialize function and this function is out of scope.
            ((T*)(rawPointer))->Serialize(ar);
            return true;
        }

        template<class T>
        T* cast() const
        {
            if (type_ == Serialization::TypeID::get<T>())
            {
                return (T*)object_;
            }
            else
            {
                return 0;
            }
        }
    private:

        TypeID type_;
        void* object_;
        size_t size_;
        SerializeStructFunc serializeFunc_;
    };/*}}}*/
    typedef std::vector<SStruct> SStructs;

    // ---------------------------------------------------------------------------

    // This type is used to generalize access to specific container types.
    // It is used by concrete IArchive implementations.
    class IContainer
    {
    public:
        virtual ~IContainer() { }

        virtual size_t size() const = 0;
        virtual size_t resize(size_t size) = 0;
        virtual bool isFixedSize() const{ return false; }

        virtual void* pointer() const = 0;
        virtual bool next() = 0;
        virtual TypeID containerType() const = 0;

        virtual TypeID elementType() const = 0;
        virtual void* elementPointer() const = 0;
        virtual size_t elementSize() const = 0;

        virtual bool operator()(IArchive& ar, const char* name, const char* label) = 0;
        virtual operator bool() const = 0;
        virtual void serializeNewElement(IArchive& ar, const char* name = "", const char* label = 0) const = 0;
    };

    template<class T>
    class ContainerArray
        : public IContainer              /*{{{*/
    {
        friend class IArchive;
    public:
        explicit ContainerArray(T* array = 0, int size = 0)
            : array_(array)
            , index_(0)
            , size_(size)
        {
        }

        // from ContainerSerializationInterface:
        size_t size() const{ return size_; }
        size_t resize([[maybe_unused]] size_t size)
        {
            index_ = 0;
            return size_;
        }

        void* pointer() const{ return reinterpret_cast<void*>(array_); }
        TypeID containerType() const{ return TypeID::get<T>(); }
        TypeID elementType() const{ return TypeID::get<T>(); }
        void* elementPointer() const { return &array_[index_]; }
        size_t elementSize() const { return sizeof(T); }
        virtual bool isFixedSize() const{ return true; }

        bool operator()(IArchive& ar, const char* name, const char* label)
        {
            YASLI_ESCAPE(size_t(index_) < size_, return false);
            return ar(array_[index_], name, label);
        }
        operator bool() const{
            return array_ != 0;
        }
        bool next()
        {
            ++index_;
            return size_t(index_) < size_;
        }
        void serializeNewElement(IArchive& ar, const char* name, const char* label) const
        {
            T element;
            ar(element, name, label);
        }
        // ^^^

    private:
        T* array_;
        int index_;
        size_t size_;
    };/*}}}*/

    // Generialized interface over owning polymorphic pointers.
    // Used by concrete IArchive implementations.
    class IPointer
    {
    public:
        virtual ~IPointer() { }

        virtual const char* registeredTypeName() const = 0;
        virtual void create(const char* registedTypeName) const = 0;
        virtual TypeID baseType() const = 0;
        virtual SStruct serializer() const = 0;
        virtual void* get() const = 0;
        virtual const void* handle() const = 0;
        virtual TypeID pointerType() const = 0;
        virtual IClassFactory* factory() const = 0;

        void Serialize(IArchive& ar) const;
    };

    class IString
    {
    public:
        virtual ~IString() { }

        virtual void set(const char* value) = 0;
        virtual const char* get() const = 0;
        virtual const void* handle() const = 0;
        virtual TypeID type() const = 0;
    };
    class IWString
    {
    public:
        virtual ~IWString() { }

        virtual void set(const wchar_t* value) = 0;
        virtual const wchar_t* get() const = 0;
        virtual const void* handle() const = 0;
        virtual TypeID type() const = 0;
    };
}
// vim:ts=4 sw=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZER_H

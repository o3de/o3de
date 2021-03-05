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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_IARCHIVE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_IARCHIVE_H
#pragma once


#include <stdarg.h>
#include <map>

#include "Serializer.h"
#include "KeyValue.h"
#include "TypeID.h"

namespace Serialization {
    class IArchive;

    template<class T>
    bool Serialize(Serialization::IArchive& ar, T& object, const char* name, const char* label);

    class CEnumDescription;
    template <class Enum>
    CEnumDescription& getEnumDescription();
    bool serializeEnum(const CEnumDescription& desc, IArchive& ar, int& value, const char* name, const char* label);

    // SContextLink should not be used directly. See SContext<> below.
    struct SContextLink
    {
        SContextLink* outer;
        TypeID type;
        void* contextObject;

        SContextLink()
            : outer()
            , contextObject()
        {
        }
    };

    struct SBlackBox;
    struct ICallback;

    class IArchive
    {
    public:
        enum ArchiveCaps
        {
            INPUT = 1 << 0,
            OUTPUT = 1 << 1,
            TEXT = 1 << 2,
            BINARY = 1 << 3,
            EDIT = 1 << 4,
            INPLACE = 1 << 5,
            NO_EMPTY_NAMES = 1 << 6,
            VALIDATION = 1 << 7,
            DOCUMENTATION = 1 << 8
        };

        IArchive(int caps)
            : caps_(caps)
            , filter_(0)
            , innerContext_(0)
        {
        }
        virtual ~IArchive() {}

        bool IsInput() const{ return caps_ & INPUT ? true : false; }
        bool IsOutput() const{ return caps_ & OUTPUT ? true : false; }
        bool IsEdit() const
        {
#if !defined(CONSOLE) && !defined(RELEASE)
            return (caps_ & EDIT) != 0;
#else
            return false;
#endif
        }
        bool IsInPlace() const{ return caps_ & INPLACE ? true : false; }
        bool GetCaps(int caps) const { return (caps_ & caps) == caps; }

        void SetFilter(int filter)
        {
            filter_ = filter;
        }
        int GetFilter() const{ return filter_; }
        bool Filter(int flags) const
        {
            YASLI_ASSERT(flags != 0 && "flags is supposed to be a bit mask");
            YASLI_ASSERT(filter_ && "Filter is not set!");
            return (filter_ & flags) != 0;
        }

        virtual bool operator()([[maybe_unused]] bool& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0)           { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] char& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] uint8& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] int8& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] int16& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] uint16& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] int32& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] uint32& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] int64& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] uint64& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] float& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] double& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }

        virtual bool operator()([[maybe_unused]] IString& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0)    { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] IWString& value, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0)    { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] const SStruct& ser, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { notImplemented(); return false; }
        virtual bool operator()([[maybe_unused]] IContainer& ser, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { return false; }
        virtual bool operator()(IPointer& ptr, const char* name = "", const char* label = 0);
        virtual bool operator()(IKeyValue& keyValue, const char* name = "", const char* label = 0) { return operator()(SStruct(keyValue), name, label); }
        virtual bool operator()([[maybe_unused]] const SBlackBox& blackBox, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { return false; }
        virtual bool operator()([[maybe_unused]] ICallback& callback, [[maybe_unused]] const char* name = "", [[maybe_unused]] const char* label = 0) { return false; }

        template<class T>
        bool operator()(const T& value, const char* name = "", const char* label = 0);

        // Error and Warning calls are used for diagnostics and validation of the
        // values. Output depends on the specific implementation of IArchive,
        // for example PropertyTree uses it to show bubbles with errors in UI
        // next to the mentioned property.
        template<class T>
        void Error(T& value, const char* format, ...);
        template<class T>
        void Warning(T& value, const char* format, ...);

        void Error(const void* value, const Serialization::TypeID& type, const char* format, ...);
        // Used to add tooltips in PropertyTree
        void Doc(const char* docString);

        virtual bool OpenBlock([[maybe_unused]] const char* name, [[maybe_unused]] const char* label) { return true; }
        virtual void CloseBlock() {}

        // long, unsigned long and long double are intentionally omitted

        template<class T>
        T* FindContext() const { return (T*)FindContextByType(TypeID::get<T>()); }

        void* FindContextByType(const TypeID& type) const
        {
            SContextLink* context = innerContext_;
            while (context)
            {
                if (context->type == type)
                {
                    return context->contextObject;
                }
                context = context->outer;
            }
            return 0;
        }

        SContextLink* SetInnerContext(SContextLink* context)
        {
            SContextLink* result = innerContext_;
            innerContext_ = context;
            return result;
        }
        SContextLink* GetInnerContext() const{ return innerContext_; }
    protected:
        virtual void ValidatorMessage([[maybe_unused]] bool error, [[maybe_unused]] const void* handle, [[maybe_unused]] const TypeID& type, [[maybe_unused]] const char* message) {}
        virtual void DocumentLastField([[maybe_unused]] const char* text) {}

        void notImplemented() { YASLI_ASSERT(0 && "Not implemented!"); }

        int caps_;
        int filter_;

        SContextLink* innerContext_;
    };


    // IArchive::SContext can be used to establish access to outer objects in serialization stack.
    //
    // Example:
    //   void Scene::Serialize(...) {
    //     IArchive::SContext<Scene> context(ar, this);
    //     ar(rootNode, ...);
    //   }
    //
    //   void Node::Serialize(...) {
    //     Scene* scene = ar.FindContext<Scene>();
    //   }
    template<class T>
    struct SContext
        : SContextLink
    {
        SContext(IArchive& ar, T* context)
            : ar_(&ar)
        {
            outer = ar_->SetInnerContext(this);
            type = TypeID::get<T>();
            contextObject = (void*)context;
        }
        SContext(T* context)
            : ar_(0)
        {
            outer = 0;
            type = TypeID::get<T>();
            contextObject = (void*)context;
        }
        ~SContext()
        {
            if (ar_)
            {
                ar_->SetInnerContext(outer);
            }
        }
    private:
        IArchive* ar_;
    };

    namespace detail {
        template<bool C, class T1, class T2>
        struct Selector{};

        template<class T1, class T2>
        struct Selector<false, T1, T2>
        {
            typedef T2 type;
        };

        template<class T1, class T2>
        struct Selector<true, T1, T2>
        {
            typedef T1 type;
        };

        template<class C, class T1, class T2>
        struct Select
        {
            typedef typename Selector<C::value, T1, T2>::type selected_type;
            typedef typename selected_type::type type;
        };

        template<class T>
        struct Identity
        {
            typedef T type;
        };


        template<class T>
        struct IsArray
        {
            enum
            {
                value = false
            };
        };

        template<class T, int Size>
        struct IsArray< T[Size] >
        {
            enum
            {
                value = true
            };
        };

        template<class T, int Size>
        struct ArraySize
        {
            enum
            {
                value = true
            };
        };

        template<class T>
        struct SerializeStruct
        {
            static bool invoke(IArchive& ar, T& value, const char* name, const char* label)
            {
                SStruct ser(value);
                return ar(ser, name, label);
            };
        };

        template<class Enum>
        struct SerializeEnum
        {
            static bool invoke(IArchive& ar, Enum& value, const char* name, const char* label)
            {
                const CEnumDescription& enumDescription = getEnumDescription<Enum>();
                return serializeEnum(enumDescription, ar, reinterpret_cast<int&>(value), name, label);
            };
        };

        template<class T>
        struct SerializeArray{};

        template<class T, int Size>
        struct SerializeArray<T[Size]>
        {
            static bool invoke(IArchive& ar, T value[Size], const char* name, const char* label)
            {
                ContainerArray<T> ser(value, Size);
                return ar(static_cast<IContainer&>(ser), name, label);
            }
        };


        template<class T>
        struct IsClass
        {
        private:
            struct NoType
            {
                char dummy;
            };
            struct YesType
            {
                char dummy[100];
            };

            template<class U>
            static YesType function_helper(void(U::*)(void));

            template<class U>
            static NoType function_helper(...);
        public:
            enum
            {
                value = (sizeof(function_helper<T>(0)) == sizeof(YesType))
            };
        };
    }

    template<class T>
    bool IArchive::operator()(const T& value, const char* name, const char* label)
    {
        return Serialize(*this, const_cast<T&>(value), name, label);
    }

    inline bool IArchive::operator()(IPointer& ptr, const char* name, const char* label)
    {
        return (*this)(SStruct(const_cast<IPointer&>(ptr)), name, label);
    }

    inline void IArchive::Doc([[maybe_unused]] const char* docString)
    {
#if !defined(CONSOLE) && !defined(RELEASE)
        if (caps_ & DOCUMENTATION)
        {
            DocumentLastField(docString);
        }
#endif
    }

    template<class T>
    void IArchive::Error([[maybe_unused]] T& value, [[maybe_unused]] const char* format, ...)
    {
#if !defined(CONSOLE) && !defined(RELEASE)
        if ((caps_ & VALIDATION) == 0)
        {
            return;
        }
        va_list args;
        va_start(args, format);
        char buf[1024];
        azvsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        ValidatorMessage(true, &value, TypeID::get<T>(), buf);
#endif
    }

    inline void IArchive::Error([[maybe_unused]] const void* handle, [[maybe_unused]] const Serialization::TypeID& type, [[maybe_unused]] const char* format, ...)
    {
#if !defined(CONSOLE) && !defined(RELEASE)
        if ((caps_ & VALIDATION) == 0)
        {
            return;
        }
        va_list args;
        va_start(args, format);
        char buf[1024];
        azvsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        ValidatorMessage(true, handle, type, buf);
#endif
    }

    template<class T>
    void IArchive::Warning([[maybe_unused]] T& value, [[maybe_unused]] const char* format, ...)
    {
#if !defined(CONSOLE) && !defined(RELEASE)
        if ((caps_ & VALIDATION) == 0)
        {
            return;
        }
        va_list args;
        va_start(args, format);
        char buf[1024];
        azvsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        ValidatorMessage(false, &value, TypeID::get<T>(), buf);
#endif
    }

    template<class T, int Size>
    bool Serialize(Serialization::IArchive& ar, T object[Size], const char* name, const char* label)
    {
        YASLI_ASSERT(0);
        return false;
    }

    template<class T>
    bool Serialize(Serialization::IArchive& ar, const T& object, const char* name, const char* label)
    {
        T::unable_to_serialize_CONST_object();
        YASLI_ASSERT(0);
        return false;
    }

    template<class T>
    bool Serialize(Serialization::IArchive& ar, T& object, const char* name, const char* label)
    {
        using namespace Serialization::detail;

        return
            Select< IsClass<T>,
            Identity< SerializeStruct<T> >,
            Select< IsArray<T>,
                Identity< SerializeArray<T> >,
                Identity< SerializeEnum<T> >
                >
            >::type::invoke(ar, object, name, label);
    }
}

#include "Serialization/SerializerImpl.h"

// vim: ts=4 sw=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_IARCHIVE_H

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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_STLIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_STLIMPL_H
#pragma once


#include "Serialization/IArchive.h"
#include "Serialization/Serializer.h"

namespace Serialization {
    template<class Container, class Element>
    class ContainerSTL
        : public IContainer           /*{{{*/
    {
    public:
        explicit ContainerSTL(Container* container = 0)
            : container_(container)
            , it_(container->begin())
            , size_(container->size())
        {
            YASLI_ASSERT(container_ != 0);
        }

        template<class T, class A>
        void resizeHelper(size_t _size, std::vector<T, A>* _v) const
        {
            _v->resize(_size);
        }

        void resizeHelper(size_t _size, ...) const
        {
            while (size_t(container_->size()) > _size)
            {
                typename Container::iterator it = container_->end();
                --it;
                container_->erase(it);
            }
            while (size_t(container_->size()) < _size)
            {
                container_->insert(container_->end(), Element());
            }
        }

        // from ContainerSerializationInterface
        size_t size() const
        {
            YASLI_ESCAPE(container_ != 0, return 0);
            return container_->size();
        }
        size_t resize(size_t size)
        {
            YASLI_ESCAPE(container_ != 0, return 0);
            resizeHelper(size, container_);
            it_ = container_->begin();
            size_ = size;
            return size;
        }

        void* pointer() const{ return reinterpret_cast<void*>(container_); }
        TypeID elementType() const{ return TypeID::get<Element>(); }
        TypeID containerType() const{ return TypeID::get<Container>(); }


        bool next()
        {
            YASLI_ESCAPE(container_ && it_ != container_->end(), return false);
            ++it_;
            return it_ != container_->end();
        }

        void* elementPointer() const { return &*it_; }
        size_t elementSize() const { return sizeof(typename Container::value_type); }

        bool operator()(IArchive& ar, const char* name, const char* label)
        {
            YASLI_ESCAPE(container_, return false);
            if (it_ == container_->end())
            {
                it_ = container_->insert(container_->end(), Element());
                return ar(*it_, name, label);
            }
            else
            {
                return ar(*it_, name, label);
            }
        }
        operator bool() const{
            return container_ != 0;
        }
        void serializeNewElement(IArchive& ar, const char* name = "", const char* label = 0) const
        {
            Element element;
            ar(element, name, label);
        }
        // ^^^
    protected:
        Container* container_;
        typename Container::iterator it_;
        size_t size_;
    };/*}}}*/
}

namespace std
{
    template<class T, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::vector<T, Alloc>& container, const char* name, const char* label)
    {
        Serialization::ContainerSTL<std::vector<T, Alloc>, T> ser(&container);
        return ar(static_cast<Serialization::IContainer&>(ser), name, label);
    }

    template<class T, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::list<T, Alloc>& container, const char* name, const char* label)
    {
        Serialization::ContainerSTL<std::list<T, Alloc>, T> ser(&container);
        return ar(static_cast<Serialization::IContainer&>(ser), name, label);
    }

    template<class K, class V, class C, class Alloc>
    bool Serialize(Serialization::IArchive& ar, std::map<K, V, C, Alloc>& container, const char* name, const char* label)
    {
        std::vector<std::pair<K, V> > temp;
        if (ar.IsOutput())
        {
            temp.assign(container.begin(), container.end());
        }
        if (!ar(temp, name, label))
        {
            return false;
        }
        if (ar.IsInput())
        {
            container.clear();
            container.insert(temp.begin(), temp.end());
        }
        return true;
    }
}

// ---------------------------------------------------------------------------
namespace Serialization {
    class StringSTL
        : public IString
    {
    public:
        StringSTL(string& str)
            : str_(str) { }

        void set(const char* value) { str_ = value; }
        const char* get() const { return str_.c_str(); }
        const void* handle() const { return &str_; }
        TypeID type() const { return TypeID::get<string>(); }
    private:
        string& str_;
    };

    inline bool Serialize(Serialization::IArchive& ar, Serialization::string& value, const char* name, const char* label)
    {
        Serialization::StringSTL str(value);
        return ar(static_cast<Serialization::IString&>(str), name, label);
    }

    // ---------------------------------------------------------------------------

    class WStringSTL
        : public IWString
    {
    public:
        WStringSTL(Serialization::wstring& str)
            : str_(str) { }

        void set(const wchar_t* value) { str_ = value; }
        const wchar_t* get() const { return str_.c_str(); }
        const void* handle() const { return &str_; }
        TypeID type() const { return TypeID::get<wstring>(); }
    private:
        wstring& str_;
    };

    inline bool Serialize(Serialization::IArchive& ar, Serialization::wstring& value, const char* name, const char* label)
    {
        Serialization::WStringSTL str(value);
        return ar(static_cast<Serialization::IWString&>(str), name, label);
    }

    // ---------------------------------------------------------------------------

    template <class K, class V>
    struct StdPair
    {
        StdPair(std::pair<K, V>& pair)
            : pair_(pair) {}
        void Serialize(Serialization::IArchive& ar)
        {
            ar(pair_.first, "key", "Key");
            ar(pair_.second, "value", "Value");
        }
        std::pair<K, V>& pair_;
    };

    template<class V>
    struct StdStringPair
        : Serialization::IKeyValue
    {
        const char* get() const { return pair_.first.c_str(); }
        void set(const char* key) { pair_.first.assign(key); }
        const void* handle() const { return &pair_; }
        Serialization::TypeID type() const { return Serialization::TypeID::get<string>(); }
        bool serializeValue(Serialization::IArchive& ar, const char* name, const char* label)
        {
            return ar(pair_.second, name, label);
        }

        StdStringPair(std::pair<string, V>& pair)
            : pair_(pair)
        {
        }

        std::pair<string, V>& pair_;
    };
}

namespace   std
{
    template<class K, class V>
    bool Serialize(Serialization::IArchive& ar, std::pair<K, V>& pair, const char* name, const char* label)
    {
        Serialization::StdPair<K, V> keyValue(pair);
        return ar(keyValue, name, label);
    }

    template<class V>
    bool Serialize(Serialization::IArchive& ar, std::pair<string, V>& pair, const char* name, const char* label)
    {
        Serialization::StdStringPair<V> keyValue(pair);
        return ar(static_cast<Serialization::IKeyValue&>(keyValue), name, label);
    }
}
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_STLIMPL_H

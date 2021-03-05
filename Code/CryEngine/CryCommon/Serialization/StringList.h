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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLIST_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLIST_H
#pragma once


#include <vector>
#include "Serialization/Strings.h"
#include "Serialization/DynArray.h"
#include <string.h>
#include "Serialization/Assert.h"
#ifndef SERIALIZATION_STANDALONE
#include <CryArray.h>
#endif

#include <AzCore/std/containers/fixed_vector.h>

namespace Serialization {
    class IArchive;
    class StringListStatic
#ifdef SERIALIZATION_STANDALONE
        : public std::vector<const char*>
    {
#else
        : public AZStd::fixed_vector<const char*, 64> {
#endif
    public:
        enum
        {
            npos = -1
        };
        int find(const char* value) const
        {
            int numItems = int(size());
            for (int i = 0; i < numItems; ++i)
            {
                if (strcmp((*this)[i], value) == 0)
                {
                    return i;
                }
            }
            return npos;
        }
    };

    class StringListStaticValue
    {
    public:
        StringListStaticValue(const StringListStaticValue& original)
            : stringList_(original.stringList_)
            , index_(original.index_)
        {
            handle_ = this;
        }
        StringListStaticValue()
            : stringList_(0)
            , index_(StringListStatic::npos)
        {
            handle_ = this;
        }
        StringListStaticValue(const StringListStatic& stringList, int value)
            : stringList_(&stringList)
            , index_(value)
        {
            handle_ = this;
        }
        StringListStaticValue(const StringListStatic& stringList, int value, const void* handle, const Serialization::TypeID& type)
            : stringList_(&stringList)
            , index_(value)
            , handle_(handle)
            , type_(type)
        {
        }
        StringListStaticValue(const StringListStatic& stringList, const char* value, const void* handle, const Serialization::TypeID& type)
            : stringList_(&stringList)
            , index_(stringList.find(value))
            , handle_(handle)
            , type_(type)
        {
            YASLI_ASSERT(index_ != StringListStatic::npos);
        }
        StringListStaticValue& operator=(const char* value)
        {
            index_ = stringList_->find(value);
            return *this;
        }
        StringListStaticValue& operator=(int value)
        {
            YASLI_ASSERT(value >= 0 && size_t(value) < size_t(stringList_->size()));
            YASLI_ASSERT(this != 0);
            index_ = value;
            return *this;
        }
        StringListStaticValue& operator=(const StringListStaticValue& rhs)
        {
            stringList_ = rhs.stringList_;
            index_ = rhs.index_;
            return *this;
        }
        const char* c_str() const
        {
            if (index_ >= 0 && size_t(index_) < size_t(stringList_->size()))
            {
                return (*stringList_)[index_];
            }
            else
            {
                return "";
            }
        }
        int index() const{ return index_; }
        const void* handle() const{ return handle_; }
        Serialization::TypeID type() const { return type_; }
        const StringListStatic& stringList() const{ return *stringList_; }
        template<class IArchive>
        void Serialize(IArchive& ar)
        {
            ar(index_, "index");
        }
    protected:
        const StringListStatic* stringList_;
        int index_;
        const void* handle_;
        Serialization::TypeID type_;
    };

    class StringList
#ifdef SERIALIZATION_STANDALONE
        : public std::vector<string>
    {
#else
        : public DynArray<string>{
#endif
    public:
        StringList() {}
        StringList(const StringList& rhs)
        {
            *this  = rhs;
        }
        StringList& operator=(const StringList& rhs)
        {
            // As StringList crosses dll boundaries it is important to copy strings
            // rather than reference count them to be sure that stored CryString uses
            // proper allocator.
            resize(rhs.size());
            for (size_t i = 0; i < size_t(size()); ++i)
            {
                (*this)[i] = rhs[i].c_str();
            }
            return *this;
        }
        StringList(const StringListStatic& rhs)
        {
            const int size = int(rhs.size());
            resize(size);
            for (int i = 0; i < int(size); ++i)
            {
                (*this)[i] = rhs[i];
            }
        }
        enum
        {
            npos = -1
        };
        int find(const char* value) const
        {
            const int numItems = int(size());
            for (int i = 0; i < numItems; ++i)
            {
                if ((*this)[i] == value)
                {
                    return i;
                }
            }
            return npos;
        }
    };

    class StringListValue
    {
    public:
        explicit StringListValue(const StringListStaticValue& value)
        {
            stringList_.resize(value.stringList().size());
            for (size_t i = 0; i < size_t(stringList_.size()); ++i)
            {
                stringList_[i] = value.stringList()[i];
            }
            index_ = value.index();
        }
        StringListValue(const StringListValue& value)
        {
            stringList_ = value.stringList_;
            index_ = value.index_;
        }
        StringListValue()
            : index_(StringList::npos)
        {
            handle_ = this;
        }
        StringListValue(const StringList& stringList, int value)
            : stringList_(stringList)
            , index_(value)
        {
            handle_ = this;
        }
        StringListValue(const StringList& stringList, int value, const void* handle, const Serialization::TypeID& typeId)
            : stringList_(stringList)
            , index_(value)
            , handle_(handle)
            , type_(typeId)
        {
        }
        StringListValue(const StringList& stringList, const char* value)
            : stringList_(stringList)
            , index_(stringList.find(value))
        {
            handle_ = this;
            YASLI_ASSERT(index_ != StringList::npos);
        }
        StringListValue(const StringList& stringList, const char* value, const void* handle, const Serialization::TypeID& typeId)
            : stringList_(stringList)
            , index_(stringList.find(value))
            , handle_(handle)
            , type_(typeId)
        {
            YASLI_ASSERT(index_ != StringList::npos);
        }
        StringListValue(const StringListStatic& stringList, const char* value)
            : stringList_(stringList)
            , index_(stringList.find(value))
        {
            handle_ = this;
            YASLI_ASSERT(index_ != StringList::npos);
        }
        StringListValue& operator=(const char* value)
        {
            index_ = stringList_.find(value);
            return *this;
        }
        StringListValue& operator=(int value)
        {
            YASLI_ASSERT(value >= 0 && size_t(value) < size_t(stringList_.size()));
            YASLI_ASSERT(this != 0);
            index_ = value;
            return *this;
        }
        const char* c_str() const
        {
            if (index_ >= 0 && size_t(index_) < size_t(stringList_.size()))
            {
                return stringList_[index_].c_str();
            }
            else
            {
                return "";
            }
        }
        int index() const{ return index_; }
        const void* handle() const { return handle_; }
        Serialization::TypeID type() const { return type_; }
        const StringList& stringList() const{ return stringList_; }
        template<class IArchive>
        void Serialize(IArchive& ar)
        {
            ar(index_, "index");
            ar(stringList_, "stringList");
        }
    protected:
        StringList stringList_;
        int index_;
        const void* handle_;
        Serialization::TypeID type_;
    };

    class IArchive;

    void splitStringList(StringList* result, const char* str, char sep);
    void joinStringList(string* result, const StringList& stringList, char sep);
    void joinStringList(string* result, const StringListStatic& stringList, char sep);

    bool Serialize(Serialization::IArchive& ar, Serialization::StringList& value, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::StringListValue& value, const char* name, const char* label);
    bool Serialize(Serialization::IArchive& ar, Serialization::StringListStaticValue& value, const char* name, const char* label);
}

#include "StringListImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_STRINGLIST_H

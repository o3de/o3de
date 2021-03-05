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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERS_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERS_H
#pragma once

#include "Serialization/Assert.h"

namespace Serialization {
    class RefCounter
    {
    public:
        RefCounter()
            : refCounter_(0)
        {}
        ~RefCounter() {};

        int refCount() const { return refCounter_; }

        void acquire() { ++refCounter_; }
        int release() { return --refCounter_; }
    private:
        int refCounter_;
    };

    class PolyRefCounter
        : public RefCounter
    {
    public:
        virtual ~PolyRefCounter() {}
    };

    class PolyPtrBase
    {
    public:
        PolyPtrBase()
            : ptr_(0)
        {
        }
        void release()
        {
            if (ptr_)
            {
                if (!ptr_->release())
                {
                    delete ptr_;
                }
                ptr_ = 0;
            }
        }
        void set(PolyRefCounter* const ptr)
        {
            if (ptr_ != ptr)
            {
                release();
                ptr_ = ptr;
                if (ptr_)
                {
                    ptr_->acquire();
                }
            }
        }
    protected:
        PolyRefCounter* ptr_;
    };

    template<class T>
    class PolyPtr
        : public PolyPtrBase
    {
    public:
        PolyPtr()
            : PolyPtrBase()
        {
        }

        PolyPtr(PolyRefCounter* ptr)
        {
            set(ptr);
        }

        template<class U>
        PolyPtr(U* ptr)
        {
            // TODO: replace with static_assert
            YASLI_ASSERT("PolyRefCounter must be a first base when used with multiple inheritance." &&
                static_cast<PolyRefCounter*>(ptr) == reinterpret_cast<PolyRefCounter*>(ptr));
            set(static_cast<PolyRefCounter*>(ptr));
        }

        PolyPtr(const PolyPtr& ptr)
            : PolyPtrBase()
        {
            set(ptr.ptr_);
        }
        ~PolyPtr()
        {
            release();
        }
        operator T*() const {
            return get();
        }
        template<class U>
        operator PolyPtr<U>() const {
            return PolyPtr<U>(get());
        }
        operator bool() const {
            return ptr_ != 0;
        }

        PolyPtr& operator=(const PolyPtr& ptr)
        {
            set(ptr.ptr_);
            return *this;
        }
        T* get() const { return reinterpret_cast<T*>(ptr_); }
        T& operator*() const
        {
            return *get();
        }
        T* operator->() const { return get(); }
    };

    class IArchive;
    template<class T>
    class SharedPtr
    {
    public:
        SharedPtr()
            : ptr_(0) {}
        SharedPtr(T* const ptr)
            : ptr_(0)
        {
            set(ptr);
        }
        SharedPtr(const SharedPtr& ptr)
            : ptr_(0)
        {
            set(ptr.ptr_);
        }
        ~SharedPtr()
        {
            release();
        }
        operator T*() const {
            return get();
        }
        template<class U>
        operator SharedPtr<U>() const {
            return SharedPtr<U>(get());
        }
        SharedPtr& operator=(const SharedPtr& ptr)
        {
            set(ptr.ptr_);
            return *this;
        }
        T* get() { return ptr_; }
        T* get() const { return ptr_; }
        T& operator*()
        {
            return *get();
        }
        T* operator->() const { return get(); }
        void release()
        {
            if (ptr_)
            {
                if (!ptr_->release())
                {
                    delete ptr_;
                }
                ptr_ = 0;
            }
        }
        template<class _T>
        void set(_T* const ptr) { reset(ptr); }
        template<class _T>
        void reset(_T* const ptr)
        {
            if (ptr_ != ptr)
            {
                release();
                ptr_ = ptr;
                if (ptr_)
                {
                    ptr_->acquire();
                }
            }
        }
    protected:
        T* ptr_;
    };


    template<class T>
    class AutoPtr
    {
    public:
        AutoPtr()
            : ptr_(0)
        {
        }
        AutoPtr(T* ptr)
            : ptr_(0)
        {
            set(ptr);
        }
        ~AutoPtr()
        {
            release();
        }
        AutoPtr& operator=(T* ptr)
        {
            set(ptr);
            return *this;
        }
        void set(T* ptr)
        {
            if (ptr_ && ptr_ != ptr)
            {
                release();
            }
            ptr_ = ptr;
        }
        T* get() const { return ptr_; }
        operator T*() const {
            return get();
        }
        void detach()
        {
            ptr_ = 0;
        }
        void release()
        {
            delete ptr_;
            ptr_ = 0;
        }
        T& operator*() const { return *get(); }
        T* operator->() const { return get(); }
    private:
        T* ptr_;
    };

    class IArchive;
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SharedPtr<T>& ptr, const char* name, const char* label);

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::PolyPtr<T>& ptr, const char* name, const char* label);

#include <Serialization/PointersImpl.h>
#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_POINTERS_H

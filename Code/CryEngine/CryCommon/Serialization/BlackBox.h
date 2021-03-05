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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_BLACKBOX_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_BLACKBOX_H
#pragma once

#include <stdlib.h> // for malloc and free

namespace Serialization
{
    // Black box is used to store opaque data blobs in a format internal to
    // specific Archive. For example it can be used to store sections of the JSON
    // or binary archive.
    //
    // This is useful for the Editor to store portions of files with unfamiliar
    // structure.
    //
    // We store deallocation function here so we can safely pass the blob
    // across DLLs with different memory allocators.
    struct SBlackBox
    {
        const char* format;
        void* data;
        size_t size;
        typedef void(* FreeFunction)(void*);
        FreeFunction freeFunction;

        SBlackBox()
            : format("")
            , data(0)
            , size(0)
            , freeFunction(0)
        {
        }

        SBlackBox(const SBlackBox& rhs)
            : format("")
            , data(0)
            , size(0)
            , freeFunction(0)
        {
            *this = rhs;
        }

        void set(const char* _format, const void* _data, size_t _size)
        {
            if (_data && freeFunction)
            {
                freeFunction(this->data);
                this->data = 0;
                this->size = 0;
                freeFunction = 0;
            }
            this->format = _format;
            if (_data && _size)
            {
                this->data = CryModuleMalloc(_size);
                memcpy(this->data, _data, _size);
                this->size = _size;
                freeFunction = &Free;
            }
        }

        SBlackBox& operator=(const SBlackBox& rhs)
        {
            set(rhs.format, rhs.data, rhs.size);
            return *this;
        }

        ~SBlackBox()
        {
            set("", 0, 0);
        }

        static void Free(void* ptr)
        {
            CryModuleFree(ptr);
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_BLACKBOX_H

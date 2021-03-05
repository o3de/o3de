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

// For tags 16-bit xor-hash is used, with check for uniquness in debug
// Block size is automatic: 8, 16 or 32 bits

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_BINARCHIVE_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_BINARCHIVE_H
#pragma once

#include "Serialization/IArchive.h"
#include "MemoryWriter.h"
#include "EditorCommonAPI.h"

namespace Serialization {
    inline unsigned short calcHash(const char* str)
    {
        unsigned short hash = 0;
        const unsigned short* p = (const unsigned short*)(str);
        for (;; )
        {
            unsigned short w = *p++;
            if (!(w & 0xff))
            {
                break;
            }
            hash ^= w;
            if (!(w & 0xff00))
            {
                break;
            }
        }
        return hash;
    }

    class BinOArchive
        : public IArchive
    {
    public:
        BinOArchive();
        ~BinOArchive() {}

        void clear();
        size_t length() const;
        const char* buffer() const { return stream_.buffer(); }
        bool save(const char* fileName);

        bool operator()(bool& value, const char* name, const char* label);
        bool operator()(IString& value, const char* name, const char* label);
        bool operator()(IWString& value, const char* name, const char* label);
        bool operator()(float& value, const char* name, const char* label);
        bool operator()(double& value, const char* name, const char* label);
        bool operator()(int32& value, const char* name, const char* label);
        bool operator()(uint32& value, const char* name, const char* label);
        bool operator()(int16& value, const char* name, const char* label);
        bool operator()(uint16& value, const char* name, const char* label);
        bool operator()(int64& value, const char* name, const char* label);
        bool operator()(uint64& value, const char* name, const char* label);

        bool operator()(int8& value, const char* name, const char* label);
        bool operator()(uint8& value, const char* name, const char* label);
        bool operator()(char& value, const char* name, const char* label);

        bool operator()(const SStruct& ser, const char* name, const char* label);
        bool operator()(IContainer& ser, const char* name, const char* label);
        bool operator()(IPointer& ptr, const char* name, const char* label);

        using IArchive::operator();

    private:
        void openContainer(const char* name, int size, const char* typeName);
        void openNode(const char* name, bool size8 = true);
        void closeNode(const char* name, bool size8 = true);

        std::vector<unsigned int> blockSizeOffsets_;
        MemoryWriter stream_;
    };

    //////////////////////////////////////////////////////////////////////////

    class BinIArchive
        : public IArchive
    {
    public:
        BinIArchive();
        ~BinIArchive();

        bool load(const char* fileName);
        bool open(const char* buffer, size_t length); // doesn't copy the buffer
        bool open(const BinOArchive& ar) { return open(ar.buffer(), ar.length()); }
        void close();

        bool operator()(bool& value, const char* name, const char* label);
        bool operator()(IString& value, const char* name, const char* label);
        bool operator()(IWString& value, const char* name, const char* label);
        bool operator()(float& value, const char* name, const char* label);
        bool operator()(double& value, const char* name, const char* label);
        bool operator()(int16& value, const char* name, const char* label);
        bool operator()(uint16& value, const char* name, const char* label);
        bool operator()(int32& value, const char* name, const char* label);
        bool operator()(uint32& value, const char* name, const char* label);
        bool operator()(int64& value, const char* name, const char* label);
        bool operator()(uint64& value, const char* name, const char* label);

        bool operator()(int8& value, const char* name, const char* label);
        bool operator()(uint8& value, const char* name, const char* label);
        bool operator()(char& value, const char* name, const char* label);

        bool operator()(const SStruct& ser, const char* name, const char* label);
        bool operator()(IContainer& ser, const char* name, const char* label);
        bool operator()(IPointer& ptr, const char* name, const char* label);

        using IArchive::operator();

    private:
        class Block
        {
        public:
            Block(const char* data, int size)
                : begin_(data)
                , curr_(data)
                , end_(data + size)
                , complex_(false) {}

            bool get(const char* name, Block& block);

            void read(void* data, int size)
            {
                YASLI_ASSERT(curr_ + size <= end_);
                memcpy(data, curr_, size);
                curr_ += size;
            }

            template<class T>
            void read(T& x){ read(&x, sizeof(x)); }

            void read(string& s)
            {
                YASLI_ASSERT(curr_ + strlen(curr_) < end_);
                s = curr_;
                curr_ += strlen(curr_) + 1;
            }
            void read(wstring& s)
            {
                YASLI_ASSERT(curr_ + sizeof(wchar_t) * wcslen((wchar_t*)curr_) < end_);
                s = (wchar_t*)curr_;
                curr_ += (wcslen((wchar_t*)curr_) + 1) * sizeof(wchar_t);
            }

            unsigned int readPackedSize();

            bool validToClose() const { return complex_ || curr_ == end_; }

        private:
            const char* begin_;
            const char* end_;
            const char* curr_;
            bool complex_;
        };

        typedef std::vector<Block> Blocks;
        Blocks blocks_;
        const char* loadedData_;

        bool openNode(const char* name);
        void closeNode(const char* name, bool check = true);
        Block& currentBlock() { return blocks_.back(); }
        template<class T>
        void read(T& t) { currentBlock().read(t); }
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_BINARCHIVE_H

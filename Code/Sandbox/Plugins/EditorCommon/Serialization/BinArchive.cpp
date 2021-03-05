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

#include "EditorCommon_precompiled.h"
#include "BinArchive.h"
#include <map>
#include "Serialization/ClassFactory.h"

namespace Serialization {
    static const unsigned char SIZE16 = 254;
    static const unsigned char SIZE32 = 255;

    static const unsigned int BIN_MAGIC = 0xb1a4c17f;

    //#ifdef _DEBUG
    //typedef std::map<unsigned short, string> HashMap;
    //static HashMap hashMap;
    //#endif

    BinOArchive::BinOArchive()
        : IArchive(OUTPUT | BINARY)
    {
        clear();
    }

    void BinOArchive::clear()
    {
        stream_.clear();
        stream_.write((const char*)&BIN_MAGIC, sizeof(BIN_MAGIC));
    }

    size_t BinOArchive::length() const
    {
        return stream_.position();
    }

    bool BinOArchive::save(const char* filename)
    {
        FILE* f = nullptr;
        azfopen(&f, filename, "wb");
        if (!f)
        {
            return false;
        }

        if (fwrite(buffer(), 1, length(), f) != length())
        {
            fclose(f);
            return false;
        }

        fclose(f);
        return true;
    }

    inline void BinOArchive::openNode(const char* name, bool size8)
    {
        if (!strlen(name))
        {
            return;
        }

        unsigned short hash = calcHash(name);
        stream_.write(hash);

        blockSizeOffsets_.push_back(int(stream_.position()));
        stream_.write((unsigned char)0);
        if (!size8)
        {
            stream_.write((unsigned short)0);
        }

#ifdef _DEBUG
        //  HashMap::iterator i = hashMap.find(hash);
        //  if(i != hashMap.end() && i->second != name)
        //      ASSERT_STR(0, name);
        //  hashMap[hash] = name;
#endif
    }

    inline void BinOArchive::closeNode(const char* name, bool size8)
    {
        if (!strlen(name))
        {
            return;
        }

        unsigned int offset = blockSizeOffsets_.back();
        unsigned int size = (unsigned int)(stream_.position() - offset - sizeof(unsigned char) - (size8 ? 0 : sizeof(unsigned short)));
        blockSizeOffsets_.pop_back();
        unsigned char* sizePtr = (unsigned char*)(stream_.buffer() + offset);

        if (size < SIZE16)
        {
            *sizePtr = size;
            if (!size8)
            {
                unsigned char* buffer = sizePtr + 3;
                memmove(buffer - 2, buffer, size);
                stream_.setPosition(stream_.position() - 2);
            }
        }
        else
        {
            YASLI_ASSERT(!size8);
            if (size < 0x10000)
            {
                *sizePtr = SIZE16;
                *((unsigned short*)(sizePtr + 1)) = size;
            }
            else
            {
                unsigned char* buffer = sizePtr + 3;
                stream_.write((unsigned short)0);
                *sizePtr = SIZE32;
                memmove(buffer + 2, buffer, size);
                *((unsigned int*)(sizePtr + 1)) = size;
            }
        }
    }

    bool BinOArchive::operator()(bool& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(IString& value, const char* name, [[maybe_unused]] const char* label)
    {
        bool size8 = strlen(value.get()) + 1 < SIZE16;
        openNode(name, size8);
        stream_ << value.get();
        stream_.write(char(0));
        closeNode(name, size8);
        return true;
    }

    bool BinOArchive::operator()(IWString& value, const char* name, [[maybe_unused]] const char* label)
    {
        bool size8 = (wcslen(value.get()) + 1) * 2 < SIZE16;
        openNode(name, size8);
        stream_ << value.get();
        stream_.write(short(0));
        closeNode(name, size8);
        return true;
    }

    bool BinOArchive::operator()(float& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(double& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(int16& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(int8& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(uint8& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(char& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(uint16& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(int32& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(uint32& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(int64& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(uint64& value, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name);
        stream_.write(value);
        closeNode(name);
        return true;
    }

    bool BinOArchive::operator()(const SStruct& ser, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name, false);
        ser(*this);
        closeNode(name, false);
        return true;
    }

    bool BinOArchive::operator()(IContainer& ser, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name, false);

        unsigned int size = (unsigned int)ser.size();
        if (size < SIZE16)
        {
            stream_.write((unsigned char)size);
        }
        else if (size < 0x10000)
        {
            stream_.write(SIZE16);
            stream_.write((unsigned short)size);
        }
        else
        {
            stream_.write(SIZE32);
            stream_.write(size);
        }

        if (strlen(name))
        {
            if (size > 0)
            {
                int i = 0;
                do
                {
                    char elementName[16];
                    azitoa(i++, elementName, AZ_ARRAY_SIZE(elementName), 10);
                    ser(*this, elementName, "");
                } while (ser.next());
            }

            closeNode(name, false);
        }
        else
        {
            if (size > 0)
            {
                do
                {
                    ser(*this, "", "");
                }
                while (ser.next());
            }
        }

        return true;
    }

    bool BinOArchive::operator()(IPointer& ptr, const char* name, [[maybe_unused]] const char* label)
    {
        openNode(name, false);

        const char* typeName = ptr.registeredTypeName();
        if (!typeName)
        {
            typeName = "";
        }
        if (ptr.get() && typeName[0] == '\0')
        {
            CRY_ASSERT_MESSAGE(0, "Writing unregistered class. Use SERIALIZATION_CLASS_NAME macro for registration.");
        }

        TypeID baseType = ptr.baseType();

        if (ptr.get())
        {
            stream_ << typeName;
            stream_.write(char(0));
            ptr.serializer()(*this);
        }
        else
        {
            stream_.write(char(0));
        }

        closeNode(name, false);
        return true;
    }


    //////////////////////////////////////////////////////////////////////////

    BinIArchive::BinIArchive()
        : IArchive(INPUT | BINARY)
        , loadedData_(0)
    {
    }

    BinIArchive::~BinIArchive()
    {
        close();
    }

    bool BinIArchive::load(const char* filename)
    {
        close();

        FILE* f = nullptr;
        azfopen(&f, filename, "rb");
        if (!f)
        {
            return false;
        }
        fseek(f, 0, SEEK_END);
        size_t length = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (length == 0)
        {
            fclose(f);
            return false;
        }
        loadedData_ = new char[length];
        if (fread((void*)loadedData_, 1, length, f) != length || !open(loadedData_, length))
        {
            close();
            fclose(f);
            return false;
        }
        fclose(f);
        return true;
    }

    bool BinIArchive::open(const char* buffer, size_t size)
    {
        if (size < sizeof(int))
        {
            return false;
        }
        if (*(unsigned*)(buffer) != BIN_MAGIC)
        {
            return false;
        }
        buffer += sizeof(unsigned int);
        size -= sizeof(unsigned int);

        blocks_.push_back(Block(buffer, (unsigned int)size));
        return true;
    }

    void BinIArchive::close()
    {
        if (loadedData_)
        {
            delete [] loadedData_;
        }
        loadedData_ = 0;
    }

    bool BinIArchive::openNode(const char* name)
    {
        Block block(0, 0);
        if (currentBlock().get(name, block))
        {
            blocks_.push_back(block);
            return true;
        }
        return false;
    }

    void BinIArchive::closeNode([[maybe_unused]] const char* name, [[maybe_unused]] bool check)
    {
        YASLI_ASSERT(!check || currentBlock().validToClose());
        blocks_.pop_back();
    }

    bool BinIArchive::operator()(bool& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(IString& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            string str;
            read(str);
            value.set(str.c_str());
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        string str;
        read(str);
        value.set(str.c_str());
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(IWString& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            wstring str;
            read(str);
            value.set(str.c_str());
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        wstring str;
        read(str);
        value.set(str.c_str());
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(float& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(double& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(int16& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(uint16& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }


    bool BinIArchive::operator()(int32& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(uint32& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(int64& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(uint64& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(int8& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(uint8& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(char& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            read(value);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        read(value);
        closeNode(name);
        return true;
    }

    bool BinIArchive::operator()(const SStruct& ser, const char* name, [[maybe_unused]] const char* label)
    {
        if (!strlen(name))
        {
            ser(*this);
            return true;
        }

        if (!openNode(name))
        {
            return false;
        }

        ser(*this);
        closeNode(name, false);
        return true;
    }

    bool BinIArchive::operator()(IContainer& ser, const char* name, [[maybe_unused]] const char* label)
    {
        if (strlen(name))
        {
            if (!openNode(name))
            {
                return false;
            }

            size_t size = currentBlock().readPackedSize();
            ser.resize(size);

            if (size > 0)
            {
                int i = 0;
                do
                {
                    char elementName[16];
                    azitoa(i++, elementName, AZ_ARRAY_SIZE(elementName), 10);
                    ser(*this, elementName, "");
                }
                while (ser.next());
            }
            closeNode(name);
            return true;
        }
        else
        {
            size_t size = currentBlock().readPackedSize();
            ser.resize(size);
            if (size > 0)
            {
                do
                {
                    ser(*this, "", "");
                }
                while (ser.next());
            }
            return true;
        }
    }

    bool BinIArchive::operator()(IPointer& ptr, const char* name, [[maybe_unused]] const char* label)
    {
        if (strlen(name) && !openNode(name))
        {
            return false;
        }

        string typeName;
        read(typeName);
        if (ptr.get() && (typeName.empty() || (typeName != ptr.registeredTypeName())))
        {
            ptr.create(""); // 0
        }
        if (!typeName.empty() && !ptr.get())
        {
            ptr.create(typeName.c_str());
        }

        if (SStruct ser = ptr.serializer())
        {
            ser(*this);
        }

        if (strlen(name))
        {
            closeNode(name);
        }
        return true;
    }

    unsigned int BinIArchive::Block::readPackedSize()
    {
        unsigned char size8;
        read(size8);
        if (size8 < SIZE16)
        {
            return size8;
        }
        if (size8 == SIZE16)
        {
            unsigned short size16;
            read(size16);
            return size16;
        }
        unsigned int size32;
        read(size32);
        return size32;
    }

    bool BinIArchive::Block::get(const char* name, Block& block)
    {
        if (begin_ == end_)
        {
            return false;
        }
        complex_ = true;
        unsigned short hashName = calcHash(name);
        const char* currInitial = curr_;
        bool restarted = false;
        for (;; )
        {
            if (curr_ >= end_)
            {
                return false;
            }

            unsigned short hash;
            read(hash);
            unsigned int size = readPackedSize();

            const char* currPrev = curr_;
            if ((curr_ += size) == end_)
            {
                if (restarted)
                {
                    return false;
                }
                curr_ = begin_;
                restarted = true;
            }

            //ASSERT(curr_ < end_);

            if (hash == hashName)
            {
                block = Block(currPrev, size);
                return true;
            }

            if (curr_ == currInitial)
            {
                return false;
            }
        }
    }
}

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

#pragma once

#include "Serialization/IArchive.h"
#include "MemoryReader.h"
#include "Token.h"
#include <memory>

namespace Serialization {
    class MemoryReader;

    class JSONIArchive
        : public IArchive
    {
    public:
        JSONIArchive();
        ~JSONIArchive();

        bool load(const char* filename);
        bool open(const char* buffer, size_t length, bool free = false);

        // virtuals:
        bool operator()(bool& value, const char* name = "", const char* label = 0);
        bool operator()(IString& value, const char* name = "", const char* label = 0);
        bool operator()(IWString& value, const char* name = "", const char* label = 0);
        bool operator()(float& value, const char* name = "", const char* label = 0);
        bool operator()(double& value, const char* name = "", const char* label = 0);
        bool operator()(int16& value, const char* name = "", const char* label = 0);
        bool operator()(uint16& value, const char* name = "", const char* label = 0);
        bool operator()(int32& value, const char* name = "", const char* label = 0);
        bool operator()(uint32& value, const char* name = "", const char* label = 0);
        bool operator()(int64& value, const char* name = "", const char* label = 0);
        bool operator()(uint64& value, const char* name = "", const char* label = 0);

        bool operator()(int8& value, const char* name = "", const char* label = 0);
        bool operator()(uint8& value, const char* name = "", const char* label = 0);
        bool operator()(char& value, const char* name = "", const char* label = 0);

        bool operator()(const SStruct& ser, const char* name = "", const char* label = 0);
        bool operator()(const SBlackBox& ser, const char* name = "", const char* label = 0);
        bool operator()(IContainer& ser, const char* name = "", const char* label = 0);
        bool operator()(IKeyValue& ser, const char* name = "", const char* label = 0);
        bool operator()(IPointer& ser, const char* name = "", const char* label = 0);

        using IArchive::operator();
    private:
        bool findName(const char* name, Token* outName = 0);
        bool openBracket();
        bool closeBracket();

        bool openContainerBracket();
        bool closeContainerBracket();

        void checkValueToken();
        bool checkStringValueToken();
        void readToken();
        void putToken();
        int line(const char* position) const;
        bool isName(Token token) const;

        bool expect(char token);
        void skipBlock();

        struct Level
        {
            const char* start;
            const char* firstToken;
            bool isContainer;
            bool isKeyValue;
            Level()
                : isContainer(false)
                , isKeyValue(false) {}
        };
        typedef std::vector<Level> Stack;
        Stack stack_;

        std::unique_ptr<MemoryReader> reader_;
        Token token_;
        std::vector<char> unescapeBuffer_;
        string filename_;
        void* buffer_;
    };
}

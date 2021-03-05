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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_JSONOARCHIVE_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_JSONOARCHIVE_H
#pragma once

#include "Serialization/IArchive.h"
#include "Serialization/MemoryWriter.h"
#include "EditorCommonAPI.h"
#include <memory>

namespace Serialization {
    class MemoryWriter;

    class JSONOArchive
        : public IArchive
    {
    public:
        // header = 0 - default header, use "" to omit
        JSONOArchive(int textWidth = 80, const char* header = 0);
        ~JSONOArchive();

        bool save(const char* fileName);

        const char* c_str() const;
        const char* buffer() const { return c_str(); }
        size_t length() const;

        // from Archive:
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

        bool operator()(char& value, const char* name = "", const char* label = 0);
        bool operator()(int8& value, const char* name = "", const char* label = 0);
        bool operator()(uint8& value, const char* name = "", const char* label = 0);

        bool operator()(const SStruct& ser, const char* name = "", const char* label = 0);
        bool operator()(const SBlackBox& box, const char* name = "", const char* label = 0);
        bool operator()(IContainer& ser, const char* name = "", const char* label = 0);
        bool operator()(IKeyValue& keyValue, const char* name = "", const char* label = 0);
        bool operator()(IPointer& ser, const char* name = "", const char* label = 0);
        // ^^^

        using IArchive::operator();

    private:
        void openBracket();
        void closeBracket();
        void openContainerBracket();
        void closeContainerBracket();
        void placeName(const char* name);
        void placeIndent(bool putComma = true);
        void placeIndentCompact(bool putComma = true);

        bool joinLinesIfPossible();

        struct Level
        {
            Level(bool _isContainer, std::size_t position, int column)
                : isContainer(_isContainer)
                , isKeyValue(false)
                , isDictionary(false)
                , startPosition(position)
                , indentCount(-column)
                , elementIndex(0)
                , nameIndex(0)
            {}
            bool isKeyValue;
            bool isContainer;
            bool isDictionary;
            std::size_t startPosition;
            int nameIndex;
            int elementIndex;
            int indentCount;
        };

        typedef std::vector<Level> Stack;
        Stack stack_;
        std::unique_ptr<MemoryWriter> buffer_;
        const char* header_;
        int textWidth_;
        string fileName_;
        int compactOffset_;
        bool isKeyValue_;
    };
}

#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_JSONOARCHIVE_H

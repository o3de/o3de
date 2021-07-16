/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_XMLBINARYHEADERS_H
#define CRYINCLUDE_CRYCOMMON_XMLBINARYHEADERS_H
#pragma once


namespace XMLBinary
{
    class IDataWriter
    {
    public:
        virtual ~IDataWriter() {}
        virtual void Write(const void* pData, size_t size) = 0;
    };

    class IFilter
    {
    public:
        enum EType
        {
            eType_ElementName,
            eType_AttributeName
        };
        virtual ~IFilter() {}
        virtual bool IsAccepted(EType type, const char* pName) const = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    typedef uint32 NodeIndex;            // note: only uint32 or uint16 are supported

    template<int size>
    struct Pad
    {
        char pad[size];
    };
    template<>
    struct Pad<0> { };

    struct Node
    {
        uint32 nTagStringOffset;         // offset in CBinaryXmlData::pStringData
        uint32 nContentStringOffset;     // offset in CBinaryXmlData::pStringData
        uint16 nAttributeCount;
        uint16 nChildCount;
        NodeIndex nParentIndex;
        NodeIndex nFirstAttributeIndex;
        NodeIndex nFirstChildIndex;
        Pad<sizeof(uint32) - sizeof(NodeIndex)> reserved_for_alignment;
    };

    struct Attribute
    {
        uint32 nKeyStringOffset;         // offset in CBinaryXmlData::pStringData
        uint32 nValueStringOffset;       // offset in CBinaryXmlData::pStringData
    };

    struct BinaryFileHeader
    {
        char szSignature[8];
        uint32 nXMLSize;
        uint32 nNodeTablePosition;
        uint32 nNodeCount;
        uint32 nAttributeTablePosition;
        uint32 nAttributeCount;
        uint32 nChildTablePosition;
        uint32 nChildCount;
        uint32 nStringDataPosition;
        uint32 nStringDataSize;
    };
}

#endif // CRYINCLUDE_CRYCOMMON_XMLBINARYHEADERS_H

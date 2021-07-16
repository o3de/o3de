/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Moved Craig's ReadWriteXMLSink from CryAction to CrySystem


#ifndef CRYINCLUDE_CRYCOMMON_IREADWRITEXMLSINK_H
#define CRYINCLUDE_CRYCOMMON_IREADWRITEXMLSINK_H
#pragma once

#include <IXml.h>
#include <AzCore/std/containers/variant.h>

struct IReadXMLSink;
struct IWriteXMLSource;

struct IReadWriteXMLSink
{
    // <interfuscator:shuffle>
    virtual ~IReadWriteXMLSink(){}
    virtual bool ReadXML(const char* definitionFile, const char* dataFile, IReadXMLSink* pSink) = 0;
    virtual bool ReadXML(const char* definitionFile, XmlNodeRef node, IReadXMLSink* pSink) = 0;
    virtual bool ReadXML(XmlNodeRef definition, const char* dataFile, IReadXMLSink* pSink) = 0;
    virtual bool ReadXML(XmlNodeRef definition, XmlNodeRef node, IReadXMLSink* pSink) = 0;

    virtual XmlNodeRef CreateXMLFromSource(const char* definitionFile, IWriteXMLSource* pSource) = 0;
    virtual bool WriteXML(const char* definitionFile, const char* dataFile, IWriteXMLSource* pSource) = 0;
    // </interfuscator:shuffle>
};


struct SReadWriteXMLCommon
{
    typedef AZStd::variant<Vec3, int, float, const char*, bool> TValue;
};


TYPEDEF_AUTOPTR(IReadXMLSink);
typedef IReadXMLSink_AutoPtr IReadXMLSinkPtr;

// this interface allows customization of the data read routines
struct IReadXMLSink
    : public SReadWriteXMLCommon
{
    // <interfuscator:shuffle>
    virtual ~IReadXMLSink(){}
    // reference counting
    virtual void AddRef() = 0;
    virtual void Release() = 0;

    virtual IReadXMLSinkPtr BeginTable(const char* name, const XmlNodeRef& definition) = 0;
    virtual IReadXMLSinkPtr BeginTableAt(int elem, const XmlNodeRef& definition) = 0;
    virtual bool SetValue(const char* name, const TValue& value, const XmlNodeRef& definition) = 0;
    virtual bool EndTableAt(int elem) = 0;
    virtual bool EndTable(const char* name) = 0;

    virtual IReadXMLSinkPtr BeginArray(const char* name, const XmlNodeRef& definition) = 0;
    virtual bool SetAt(int elem, const TValue& value, const XmlNodeRef& definition) = 0;
    virtual bool EndArray(const char* name) = 0;

    virtual bool Complete() = 0;

    virtual bool IsCreationMode() = 0;
    virtual XmlNodeRef GetCreationNode() = 0;
    virtual void SetCreationNode(XmlNodeRef definition) = 0;
    // </interfuscator:shuffle>
};


TYPEDEF_AUTOPTR(IWriteXMLSource);
typedef IWriteXMLSource_AutoPtr IWriteXMLSourcePtr;

// this interface allows customization of the data write routines
struct IWriteXMLSource
    : public SReadWriteXMLCommon
{
    // <interfuscator:shuffle>
    virtual ~IWriteXMLSource(){}
    // reference counting
    virtual void AddRef() = 0;
    virtual void Release() = 0;

    virtual IWriteXMLSourcePtr BeginTable(const char* name) = 0;
    virtual IWriteXMLSourcePtr BeginTableAt(int elem) = 0;
    virtual bool HaveValue(const char* name) = 0;
    virtual bool GetValue(const char* name, TValue& value, const XmlNodeRef& definition) = 0;
    virtual bool EndTableAt(int elem) = 0;
    virtual bool EndTable(const char* name) = 0;

    virtual IWriteXMLSourcePtr BeginArray(const char* name, size_t* numElems, const XmlNodeRef& definition) = 0;
    virtual bool HaveElemAt(int elem) = 0;
    virtual bool GetAt(int elem, TValue& value, const XmlNodeRef& definition) = 0;
    virtual bool EndArray(const char* name) = 0;

    virtual bool Complete() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IREADWRITEXMLSINK_H

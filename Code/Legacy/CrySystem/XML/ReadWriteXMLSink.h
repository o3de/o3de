/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_XML_READWRITEXMLSINK_H
#define CRYINCLUDE_CRYSYSTEM_XML_READWRITEXMLSINK_H
#pragma once


#include <ISystem.h>
#include <IReadWriteXMLSink.h>

class CReadWriteXMLSink
    : public IReadWriteXMLSink
{
public:
    bool ReadXML(const char* definitionFile, const char* dataFile, IReadXMLSink* pSink);
    bool ReadXML(const char* definitionFile, XmlNodeRef node, IReadXMLSink* pSink);
    bool ReadXML(XmlNodeRef definition, const char* dataFile, IReadXMLSink* pSink);
    bool ReadXML(XmlNodeRef definition, XmlNodeRef node, IReadXMLSink* pSink);

    XmlNodeRef CreateXMLFromSource(const char* definitionFile, IWriteXMLSource* pSource);
    bool WriteXML(const char* definitionFile, const char* dataFile, IWriteXMLSource* pSource);
};


// helper to define the if/else chain that we need in a few locations...
// types must match IReadXMLSink::TValueTypes
#define XML_SET_PROPERTY_HELPER(ELSE_LOAD_PROPERTY) \
    if (false) {; }                                 \
    ELSE_LOAD_PROPERTY(Vec3);                       \
    ELSE_LOAD_PROPERTY(int);                        \
    ELSE_LOAD_PROPERTY(float);                      \
    ELSE_LOAD_PROPERTY(string);                     \
    ELSE_LOAD_PROPERTY(bool);


#endif // CRYINCLUDE_CRYSYSTEM_XML_READWRITEXMLSINK_H

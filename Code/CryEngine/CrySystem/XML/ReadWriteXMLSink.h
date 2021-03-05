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

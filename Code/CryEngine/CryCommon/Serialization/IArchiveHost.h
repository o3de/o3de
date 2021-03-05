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
// IArchiveHost serves a purpose of sharing IArchive implementations among
// diffferent modules.
//
// Example of usage:
//
//  struct SType
//  {
//     void Serialize(Serialization::IArchive& ar);
//  };
//
//  SType instanceToSave;
//  bool saved = Serialization::SaveJsonFile("Scripts/instance.json", instanceToSave);
//
//  SType instanceToLoad;
//  bool loaded = Serialization::LoadJsonFile(instanceToLoad, "Scripts/instance.json");
//
#include <CryArray.h>
#include <Serialization/Serializer.h>

namespace Serialization
{
    struct IArchiveHost
    {
        virtual ~IArchiveHost() {}
        virtual bool LoadJsonFile(const SStruct& outObj, const char* filename) = 0;
        virtual bool SaveJsonFile(const char* filename, const SStruct& obj) = 0;
        virtual bool LoadJsonBuffer(const SStruct& outObj, const char* buffer, size_t bufferLength) = 0;
        virtual bool SaveJsonBuffer(DynArray<char>& outBuffer, const SStruct& obj) = 0;

        virtual bool LoadBinaryFile(const SStruct& outObj, const char* filename) = 0;
        virtual bool SaveBinaryFile(const char* filename, const SStruct& obj) = 0;
        virtual bool LoadBinaryBuffer(const SStruct& outObj, const char* buffer, size_t bufferLength) = 0;
        virtual bool SaveBinaryBuffer(DynArray<char>& outBuffer, const SStruct& obj) = 0;
        virtual bool CloneBinary(const SStruct& dest, const SStruct& source) = 0;
        // Compares two instances in serialized form through binary archive
        virtual bool CompareBinary(const SStruct& lhs, const SStruct& rhs) = 0;

        virtual bool LoadXmlFile(const SStruct& outObj, const char* filename) = 0;
        virtual bool SaveXmlFile(const char* filename, const SStruct& obj, const char* rootNodeName) = 0;
        virtual bool LoadXmlNode(const SStruct& outObj, const XmlNodeRef& node) = 0;
        virtual XmlNodeRef SaveXmlNode(const SStruct& obj, const char* nodeName) = 0;
        virtual bool SaveXmlNode(XmlNodeRef& node, const SStruct& obj) = 0;
    };

    // Syntactic sugar
    template<class T>
    bool LoadJsonFile(T& instance, const char* filename)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadJsonFile(Serialization::SStruct(instance), filename);
    }

    template<class T>
    bool SaveJsonFile(const char* filename, const T& instance)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveJsonFile(filename, Serialization::SStruct(instance));
    }

    template<class T>
    bool LoadJsonBuffer(T& instance, const char* buffer, size_t bufferLength)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadJsonBuffer(Serialization::SStruct(instance), buffer, bufferLength);
    }

    template<class T>
    bool SaveJsonBuffer(DynArray<char>& outBuffer, const T& instance)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveJsonBuffer(outBuffer, Serialization::SStruct(instance));
    }

    // ---------------------------------------------------------------------------

    template<class T>
    bool LoadBinaryFile(T& outInstance, const char* filename)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadBinaryFile(Serialization::SStruct(outInstance), filename);
    }

    template<class T>
    bool SaveBinaryFile(const char* filename, const T& instance)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveBinaryFile(filename, Serialization::SStruct(instance));
    }

    template<class T>
    bool LoadBinaryBuffer(T& outInstance, const char* buffer, size_t bufferLength)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadBinaryBuffer(Serialization::SStruct(outInstance), buffer, bufferLength);
    }

    template<class T>
    bool SaveBinaryBuffer(DynArray<char>& outBuffer, const T& instance)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveBinaryBuffer(outBuffer, Serialization::SStruct(instance));
    }

    template<class T>
    bool CloneBinary(T& outInstance, const T& inInstance)
    {
        return gEnv->pSystem->GetArchiveHost()->CloneBinary(Serialization::SStruct(outInstance), Serialization::SStruct(inInstance));
    }

    template<class T>
    bool CompareBinary(const T& lhs, const T& rhs)
    {
        return gEnv->pSystem->GetArchiveHost()->CompareBinary(Serialization::SStruct(lhs), Serialization::SStruct(rhs));
    }

    // ---------------------------------------------------------------------------

    template<class T>
    bool LoadXmlFile(T& outInstance, const char* filename)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadXmlFile(Serialization::SStruct(outInstance), filename);
    }

    template<class T>
    bool SaveXmlFile(const char* filename, const T& instance, const char* rootNodeName)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveXmlFile(filename, Serialization::SStruct(instance), rootNodeName);
    }

    template<class T>
    bool LoadXmlNode(T& outInstance, const XmlNodeRef& node)
    {
        return gEnv->pSystem->GetArchiveHost()->LoadXmlNode(Serialization::SStruct(outInstance), node);
    }

    template<class T>
    XmlNodeRef SaveXmlNode(const T& instance, const char* nodeName)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveXmlNode(Serialization::SStruct(instance), nodeName);
    }

    template<class T>
    bool SaveXmlNode(XmlNodeRef& node, const T& instance)
    {
        return gEnv->pSystem->GetArchiveHost()->SaveXmlNode(node, Serialization::SStruct(instance));
    }
}

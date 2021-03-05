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
#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_IXMLARCHIVE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_IXMLARCHIVE_H
#pragma once


#include "Serialization/IArchive.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/CryCreateClassInstance.h"

namespace Serialization
{
    struct IXmlArchive
        : public ICryUnknown
        , public IArchive
    {
    public:
        CRYINTERFACE_DECLARE(IXmlArchive, 0x1386c94ded174f96, 0xab14d20e1b616588);

        using IArchive::operator();

        virtual void SetXmlNode(XmlNodeRef pRootNode) = 0;
        virtual XmlNodeRef GetXmlNode() const = 0;

    protected:
        IXmlArchive(int caps)
            : IArchive(caps | IArchive::NO_EMPTY_NAMES) {}
    };


    typedef AZStd::shared_ptr< IXmlArchive > IXmlArchivePtr;


    inline IXmlArchivePtr CreateXmlInputArchive()
    {
        IXmlArchivePtr pArchive;
        CryCreateClassInstance("CXmlIArchive", pArchive);
        return pArchive;
    }


    inline IXmlArchivePtr CreateXmlInputArchive(XmlNodeRef pXmlNode)
    {
        if (pXmlNode)
        {
            IXmlArchivePtr pArchive = CreateXmlInputArchive();
            if (pArchive)
            {
                pArchive->SetXmlNode(pXmlNode);
            }
            return pArchive;
        }
        return IXmlArchivePtr();
    }


    inline IXmlArchivePtr CreateXmlInputArchive(const char* const filename)
    {
        XmlNodeRef pXmlNode  = gEnv->pSystem->LoadXmlFromFile(filename);
        return CreateXmlInputArchive(pXmlNode);
    }


    inline IXmlArchivePtr CreateXmlOutputArchive()
    {
        IXmlArchivePtr pArchive;
        CryCreateClassInstance("CXmlOArchive", pArchive);
        return pArchive;
    }


    inline IXmlArchivePtr CreateXmlOutputArchive(XmlNodeRef pXmlNode)
    {
        if (pXmlNode)
        {
            IXmlArchivePtr pArchive = CreateXmlOutputArchive();
            if (pArchive)
            {
                pArchive->SetXmlNode(pXmlNode);
            }
            return pArchive;
        }
        return IXmlArchivePtr();
    }


    inline IXmlArchivePtr CreateXmlOutputArchive(const char* const xmlRootElementName)
    {
        XmlNodeRef pXmlNode = gEnv->pSystem->CreateXmlNode(xmlRootElementName);
        return CreateXmlOutputArchive(pXmlNode);
    }


    template< typename T >
    bool StructFromXml(const char* const filename, T& dataOut)
    {
        Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlInputArchive(filename);
        if (pXmlArchive)
        {
            Serialization::SStruct serializer = Serialization::SStruct(dataOut);
            const bool success = serializer(*pXmlArchive);
            return success;
        }
        return false;
    }


    template< typename T >
    bool StructFromXml(XmlNodeRef pXmlNode, T& dataOut)
    {
        Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlInputArchive(pXmlNode);
        if (pXmlArchive)
        {
            Serialization::SStruct serializer = Serialization::SStruct(dataOut);
            const bool success = serializer(*pXmlArchive);
            return success;
        }
        return false;
    }


    template< typename T >
    XmlNodeRef StructToXml(const char* const xmlRootElementName, const T& dataIn)
    {
        Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlOutputArchive(xmlRootElementName);
        if (pXmlArchive)
        {
            Serialization::SStruct serializer = Serialization::SStruct(const_cast< T& >(dataIn));
            const bool success = serializer(*pXmlArchive);
            if (success)
            {
                return pXmlArchive->GetXmlNode();
            }
        }
        return XmlNodeRef();
    }


    template< typename T >
    bool StructToXml(XmlNodeRef pXmlNode, const T& dataIn)
    {
        Serialization::IXmlArchivePtr pXmlArchive = Serialization::CreateXmlOutputArchive(pXmlNode);
        if (pXmlArchive)
        {
            Serialization::SStruct serializer = Serialization::SStruct(const_cast< T& >(dataIn));
            const bool success = serializer(*pXmlArchive);
            return success;
        }
        return false;
    }


    template< typename T >
    bool StructToXml(const char* const filename, const char* const xmlRootElementName, const T& dataIn)
    {
        XmlNodeRef pXmlNode = Serialization::StructToXml(xmlRootElementName, dataIn);
        if (pXmlNode)
        {
            return pXmlNode->saveToFile(filename);
        }
        return false;
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_IXMLARCHIVE_H

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

#include "Util/Variable.h"
#include "Serialization.h"


namespace Serialization
{
    struct IResourceSelector;

    class CVariableOArchive
        : public IArchive
    {
    public:
        CVariableOArchive();
        virtual ~CVariableOArchive();

        _smart_ptr< IVariable > GetIVariable() const;
        CVarBlockPtr GetVarBlock() const;

        // IArchive
        virtual bool operator()(bool& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(IString& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(IWString& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(float& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(double& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(int16& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(uint16& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(int32& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(uint32& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(int64& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(uint64& value, const char* name = "", const char* label = 0) override;

        virtual bool operator()(int8& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(uint8& value, const char* name = "", const char* label = 0) override;
        virtual bool operator()(char& value, const char* name = "", const char* label = 0);

        virtual bool operator()(const SStruct& ser, const char* name = "", const char* label = 0) override;
        virtual bool operator()(IContainer& ser, const char* name = "", const char* label = 0) override;
        //virtual bool operator()( IPointer& ptr, const char* name = "", const char* label = 0 ) override;
        // ~IArchive

        using IArchive::operator();

    private:
        bool SerializeStruct(const SStruct& ser, const char* name, const char* label);
        bool SerializeStringListStaticValue(const SStruct& ser, const char* name, const char* label);
        bool SerializeRangeFloat(const SStruct& ser, const char* name, const char* label);
        bool SerializeRangeInt(const SStruct& ser, const char* name, const char* label);
        bool SerializeRangeUInt(const SStruct& ser, const char* name, const char* label);
        bool SerializeIResourceSelector(const SStruct& ser, const char* name, const char* label);

        bool SerializeAnimationName(const IResourceSelector* pSelector, const char* name, const char* label);
        bool SerializeSoundName(const IResourceSelector* pSelector, const char* name, const char* label);
        bool SerializeObjectFilename(const IResourceSelector* pSelector, const char* name, const char* label);

        void CreateChildEnumVariable(const QStringList& enumValues, const QString& value, const char* name, const char* label);

    private:
        _smart_ptr< IVariable > m_pVariable;

        typedef bool ( CVariableOArchive::* StructHandlerFunctionPtr )(const SStruct&, const char*, const char*);
        typedef std::map< string, StructHandlerFunctionPtr > HandlersMap;
        HandlersMap m_structHandlers; // TODO: have only one of these.

        typedef bool ( CVariableOArchive::* ResourceHandlerFunctionPtr )(const IResourceSelector*, const char*, const char*);
        typedef std::map< string, ResourceHandlerFunctionPtr > ResourceHandlersMap;
        ResourceHandlersMap m_resourceHandlers;
    };
}

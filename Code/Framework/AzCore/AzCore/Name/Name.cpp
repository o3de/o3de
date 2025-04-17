/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/NameSerializer.h>
#include <AzCore/Name/NameJsonSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace AZ
{
    Name* Name::s_staticNameBegin = nullptr;

    NameRef::NameRef(Name name)
        : m_data(AZStd::move(name.m_data))
    {
    }

    NameRef::NameRef(const AZStd::intrusive_ptr<Internal::NameData>& nameData)
        : m_data(nameData)
    {
    }

    NameRef::NameRef(AZStd::intrusive_ptr<Internal::NameData>&& nameData)
        : m_data(nameData)
    {
    }

    NameRef& NameRef::operator=(Name name)
    {
        m_data = AZStd::move(name.m_data);
        return *this;
    }

    bool NameRef::operator==(const NameRef& other) const
    {
        return m_data == other.m_data;
    }

    bool NameRef::operator!=(const NameRef& other) const
    {
        return m_data != other.m_data;
    }

    bool NameRef::operator==(const Name& other) const
    {
        return m_data == other.m_data;
    }

    bool NameRef::operator!=(const Name& other) const
    {
        return m_data != other.m_data;
    }

    AZStd::string_view NameRef::GetStringView() const
    {
        return m_data == nullptr ? "" : m_data->GetName();
    }

    const char* NameRef::GetCStr() const
    {
        return m_data == nullptr ? "" : m_data->GetName().data();
    }

    Internal::NameData::Hash NameRef::GetHash() const
    {
        return m_data == nullptr ? 0 : m_data->GetHash();
    }

    Name::Name() = default;

    Name::Name(AZStd::string_view name)
    {
        SetName(name);
    }
    Name::Name(AZStd::string_view name, NameDictionary& nameDictionary)
    {
        SetName(name, nameDictionary);
    }

    Name::Name(Hash hash)
    {
        auto nameDictionary = AZ::Interface<NameDictionary>::Get();
        AZ_Assert(nameDictionary != nullptr, "hash value %u cannot be looked up before the global before the NameDictionary is ready.\n"
            "If an explicit name dictionary is available, it can be passed to the Name(Hash, NameDictionary&) overload instead.", hash);
        *this = nameDictionary->FindName(hash);
    }

    Name::Name(Hash hash, NameDictionary& nameDictionary)
    {
        *this = nameDictionary.FindName(hash);
    }

    Name::Name(Internal::NameData* data)
        : m_data{data}
        , m_view{data->GetName()}
        , m_hash{data->GetHash()}
    {}

    Name::Name(const Name& rhs)
    {
        *this = rhs;
    }

    Name::Name(NameRef name)
        : m_data(AZStd::move(name.m_data))
        , m_hash(name.GetHash())
        , m_view(name.GetStringView())
    {
    }

    Name Name::FromStringLiteral(AZStd::string_view name, NameDictionary* nameDictionary)
    {
        Name literalName;
        literalName.SetNameLiteral(name, nameDictionary);
        return literalName;
    }

    Name& Name::operator=(const Name& rhs)
    {
        // If we're copying a string literal and it's not yet initialized,
        // we need to flag ourselves as a string literal
        if (this != &rhs)
        {
            if (rhs.m_supportsDeferredLoad && rhs.m_data == nullptr)
            {
                m_hash = rhs.m_hash;
                m_data = rhs.m_data;
                AZ::NameDictionary* nameDictionary = m_data != nullptr ? m_data->m_nameDictionary : nullptr;
                SetNameLiteral(rhs.m_view, nameDictionary);
            }
            else
            {
                m_data = rhs.m_data;
                m_hash = rhs.m_hash;
                m_view = rhs.m_view;
            }
        }
        return *this;
    }

    Name& Name::operator=(Name&& rhs)
    {
        // If we're moving a string literal (generally from FromStringLiteral)
        // we promote this instance to a string literal so that we respect all deferred initialization
        // This covers cases like a static Name being created at function scope while the dictionary is
        // active, when a test then destroys and recreates the name dictionary, meaning the Name needs to be
        // restored via the deferred initialization logic.
        if (rhs.m_supportsDeferredLoad)
        {
            m_hash = rhs.m_hash;
            m_data = rhs.m_data;
            AZ::NameDictionary* nameDictionary = m_data != nullptr ? m_data->m_nameDictionary : nullptr;
            SetNameLiteral(rhs.m_view, nameDictionary);
        }
        else
        {
            m_data = AZStd::move(rhs.m_data);
            m_view = rhs.m_view;
            m_hash = rhs.m_hash;
            rhs.m_view = "";
        }

        return *this;
    }

    Name& Name::operator=(AZStd::string_view name)
    {
        SetName(name);
        return *this;
    }

    Name::Name(Name&& rhs)
    {
        *this = AZStd::move(rhs);
    }

    Name::~Name()
    {
        if (m_supportsDeferredLoad)
        {
            UnlinkStaticName();
        }
    }

    void Name::SetName(AZStd::string_view name)
    {
        if (name.empty())
        {
            *this = Name();
            return;
        }

        auto nameDictionary = AZ::Interface<NameDictionary>::Get();
        AZ_Assert(nameDictionary != nullptr, "Attempted to initialize Name '%.*s' using the global NameDictionary before it is ready.\n"
            "If this name is being constructed from a string literal, consider using AZ::Name::FromStringLiteral instead.\n"
            "Alternatively the SetName(string_view, NameDictionary&) reference overload can be used to supply an explicit name dictionary reference", AZ_STRING_ARG(name));

        SetName(name, *nameDictionary);
    }

    void Name::SetName(AZStd::string_view name, NameDictionary& nameDictionary)
    {
        *this = nameDictionary.MakeName(name);
    }


    void Name::SetNameLiteral(AZStd::string_view name, NameDictionary* nameDictionary)
    {
        if (name.empty())
        {
            *this = Name();
            return;
        }

        m_view = name;
        if (nameDictionary != nullptr)
        {
            nameDictionary->LoadDeferredName(*this);
        }
        else if (!m_supportsDeferredLoad)
        {
            // Link ourselves into the deferred list if we're not already in there
            LinkStaticName(&s_staticNameBegin);
        }

        m_supportsDeferredLoad = true;
    }


    AZStd::string_view Name::GetStringView() const
    {
        return m_view;
    }

    const char* Name::GetCStr() const
    {
        return m_view.data();
    }

    bool Name::IsEmpty() const
    {
        return m_view.empty();
    }

    void Name::LinkStaticName(Name** name)
    {
        if ((*name) == nullptr)
        {
            m_nextName = m_previousName = nullptr;
            *name = this;
        }
        else
        {
            if ((*name)->m_linkedToDictionary)
            {
                m_linkedToDictionary = true;
            }
            m_nextName = *name;
            m_previousName = (*name)->m_previousName;
            (*name)->m_previousName = this;
            if (m_previousName != nullptr)
            {
                m_previousName->m_nextName = this;
            }
            *name = this;
        }
    }

    void Name::UnlinkStaticName()
    {
        if (s_staticNameBegin == this)
        {
            s_staticNameBegin = m_nextName;
        }
        if (m_nextName != nullptr)
        {
            m_nextName->m_previousName = m_previousName;
        }
        if (m_previousName != nullptr)
        {
            m_previousName->m_nextName = m_nextName;
        }
        m_nextName = m_previousName = nullptr;
        m_linkedToDictionary = false;
    }

    void Name::ScriptConstructor(Name* thisPtr, ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();
        switch (numArgs)
        {
        case 1:
        {
            if (dc.IsString(0, false))
            {
                const char* name = nullptr;
                dc.ReadArg<const char*>(0, name);
                new (thisPtr) Name(name);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "String argument expected for Name constructor!");
                new (thisPtr) Name();
            }
        }
        break;
        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Unexpected argument count for Name constructor!");
            new (thisPtr) Name();
        }
        break;
        }
    }

    void Name::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Name>()->
                Serializer<NameSerializer>();
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Name>("Name")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "name")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &Name::ScriptConstructor)
                ->Constructor()
                ->Constructor<AZStd::string_view>()
                ->Method("ToString", &Name::GetCStr)
                ->Method("__repr__", &Name::GetCStr)
                ->Method("Set", [](Name* thisPtr, AZStd::string_view name) { thisPtr->SetName(name); })
                ->Method("IsEmpty", &Name::IsEmpty)
                ->Method("Equal", static_cast<bool(Name::*)(const Name&)const>(&Name::operator==))
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;
        }
        if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<NameJsonSerializer>()->HandlesType<Name>();
        }
    }

} // namespace AZ


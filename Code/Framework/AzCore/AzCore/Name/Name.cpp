/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

    Name::Name()
        : m_view("")
    {
    }

    Name::Name(AZStd::string_view name)
    {
        SetName(name);
    }

    Name::Name(Hash hash)
    {
        *this = NameDictionary::Instance().FindName(hash);
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
        : m_data(AZStd::move(name))
        , m_hash(name->GetHash())
        , m_view(name->GetName())
    {
    }

    Name Name::FromStringLiteral(AZStd::string_view name)
    {
        Name literalName;
        literalName.SetNameLiteral(name);
        return literalName;
    }

    Name& Name::operator=(const Name& rhs)
    {
        if (rhs.m_supportsDeferredLoad && rhs.m_data == nullptr)
        {
            m_hash = rhs.m_hash;
            m_data = rhs.m_data;
            SetNameLiteral(rhs.GetStringView());
        }
        else
        {
            m_data = rhs.m_data;
            m_hash = rhs.m_hash;
            m_view = rhs.m_view;
        }
        return *this;
    }

    Name& Name::operator=(Name&& rhs)
    {
        if (rhs.m_supportsDeferredLoad && rhs.m_data == nullptr)
        {
            m_hash = rhs.m_hash;
            m_data = rhs.m_data;
            SetNameLiteral(rhs.m_view);
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

    void Name::SetName(AZStd::string_view name, bool isLiteral)
    {
        if (isLiteral && !NameDictionary::IsReady(false))
        {
            SetNameLiteral(name);
            return;
        }

        if (!name.empty())
        {
            AZ_Assert(NameDictionary::IsReady(), "Attempted to initialize Name '%.*s' before the NameDictionary is ready.", AZ_STRING_ARG(name));

            *this = AZStd::move(NameDictionary::Instance().MakeName(name));
        }
        else
        {
            *this = Name();
        }
    }

    void Name::SetNameLiteral(AZStd::string_view name)
    {
        if (name[0] != '\0')
        {
            m_view = name;
            if (!NameDictionary::IsReady(false))
            {
                // Link ourselves into the deferred list if we're not already in there
                if (!m_supportsDeferredLoad)
                {
                    LinkStaticName(&s_staticNameBegin);
                }
            }
            else
            {
                NameDictionary::Instance().LoadDeferredNames(this);
            }

            m_supportsDeferredLoad = true;
        }
        else
        {
            *this = Name();
        }
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
            if (m_previousName != nullptr)
            {
                m_previousName->m_nextName = this;
            }
            (*name)->m_previousName = this;
            *name = this;
        }
    }

    void Name::UnlinkStaticName()
    {
        if (NameDictionary::IsReady(false))
        {
            NameDictionary::Instance().UnregisterDeferredName(*this);
        }

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
                ->Method("Set", &Name::SetName)
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


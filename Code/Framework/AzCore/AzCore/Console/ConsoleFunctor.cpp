/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>


namespace AZ
{
    ConsoleFunctorBase* ConsoleFunctorBase::s_deferredHead = nullptr;

    // Needed to guard against calling Interface<T>::Get(), it's not safe to call Interface<T>::Get() prior to Az Environment attach
    // Otherwise this could trigger environment construction, which can assert on Gems
    bool ConsoleFunctorBase::s_deferredHeadInvoked = false;

    const char* GetEnumString(GetValueResult value)
    {
        switch (value)
        {
        case GetValueResult::Success:
            return "GetValueResult::Success";
        case GetValueResult::NotImplemented:
            return "GetValueResult::NotImplemented : ConsoleFunctor object does not implement the GetAs function";
        case GetValueResult::TypeNotConvertible:
            return "GetValueResult::TypeNotConvertible : ConsoleFunctor contained type is not convertible to a ConsoleVar type";
        case GetValueResult::ConsoleVarNotFound:
            return "GetValueResult::ConsoleVarNotFound : ConsoleFunctor command could not be found";
        default:
            break;
        }
        return "";
    }

    ConsoleFunctorBase::ConsoleFunctorBase(const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId)
        : m_name(name)
        , m_desc(desc)
        , m_flags(flags)
        , m_typeId(typeId)
        , m_console(nullptr)
        , m_prev(nullptr)
        , m_next(nullptr)
        , m_isDeferred(true)
    {
        if (s_deferredHeadInvoked)
        {
            m_console = AZ::Interface<IConsole>::Get();
            if (m_console)
            {
                ConsoleFunctorBase* functorPtr = this;
                m_console->LinkDeferredFunctors(functorPtr);
                m_console->GetConsoleCommandRegisteredEvent().Signal(this);
            }
        }
        else
        {
            Link(s_deferredHead);
        }
    }

    ConsoleFunctorBase::ConsoleFunctorBase(AZ::IConsole& console, const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId)
        : m_name(name)
        , m_desc(desc)
        , m_flags(flags)
        , m_typeId(typeId)
        , m_console(&console)
        , m_prev(nullptr)
        , m_next(nullptr)
        , m_isDeferred(false)
    {
        m_console->RegisterFunctor(this);
    }

    ConsoleFunctorBase::ConsoleFunctorBase(ConsoleFunctorBase&& other)
        : m_name(other.m_name)
        , m_desc(other.m_desc)
        , m_flags(other.m_flags)
        , m_console(other.m_console)
        , m_isDeferred(other.m_isDeferred)
    {
        if (m_console)
        {
            m_console->UnregisterFunctor(&other);
            m_console->RegisterFunctor(this);
        }
    }

    ConsoleFunctorBase& ConsoleFunctorBase::operator=(ConsoleFunctorBase&& other)
    {
        if (m_console)
        {
            // Unlink assigned to instance from any registered consoles
            m_console->UnregisterFunctor(this);
        }

        m_name = other.m_name;
        m_desc = other.m_desc;
        m_flags = other.m_flags;
        m_console = other.m_console;
        m_isDeferred = other.m_isDeferred;

        if (m_console)
        {
            // Unlink the other instance from the its console and link
            // the assigned to instance in its place
            m_console->UnregisterFunctor(&other);
            m_console->RegisterFunctor(this);
        }

        return *this;
    }

    ConsoleFunctorBase::~ConsoleFunctorBase()
    {
        if (m_console != nullptr)
        {
            m_console->UnregisterFunctor(this);
        }
        else if (m_isDeferred)
        {
            Unlink(s_deferredHead);
        }
    }

    void ConsoleFunctorBase::Link(ConsoleFunctorBase*& head)
    {
        m_next = head;

        if (head != nullptr)
        {
            head->m_prev = this;
        }

        head = this;
    }

    void ConsoleFunctorBase::Unlink(ConsoleFunctorBase*& head)
    {
        if (head == nullptr)
        {
            // The head has already been destroyed, no need to unlink
            return;
        }

        if (head == this)
        {
            head = m_next;
        }

        ConsoleFunctorBase* prev = m_prev;
        ConsoleFunctorBase* next = m_next;

        if (m_prev != nullptr)
        {
            m_prev->m_next = next;
            m_prev = nullptr;
        }

        if (m_next != nullptr)
        {
            m_next->m_prev = prev;
            m_next = nullptr;
        }
    }

    GetValueResult ConsoleFunctorBase::GetValueAsString(CVarFixedString&) const
    {
        return GetValueResult::NotImplemented;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "Command.h"
#include <AzCore/std/numeric.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace MCore
{
    // constructor
    Command::Callback::Callback(bool executePreUndo, bool executePreCommand)
    {
        m_preUndoExecute     = executePreUndo;
        m_preCommandExecute  = executePreCommand;
    }


    // destructor
    Command::Callback::~Callback()
    {
    }


    // constructor
    Command::Command(AZStd::string commandName, Command* originalCommand)
        : m_orgCommand(originalCommand)
        , m_commandName(AZStd::move(commandName))
    {
    }


    // destructor
    Command::~Command()
    {
        RemoveAllCallbacks();
    }


    void Command::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Command>()
            ->Version(1)
            ;
    }


    const char* Command::GetName() const
    {
        return m_commandName.c_str();
    }


    const AZStd::string& Command::GetNameString() const
    {
        return m_commandName;
    }


    // default return value for is undoable
    bool Command::GetIsUndoable() const
    {
        return false;
    }


    // initialize the default syntax
    void Command::InitSyntax()
    {
    }


    size_t Command::GetNumCallbacks() const
    {
        return m_callbacks.size();
    }


    void Command::AddCallback(Command::Callback* callback)
    { 
        m_callbacks.push_back(callback);
    }


    bool Command::CheckIfHasCallback(Command::Callback* callback) const
    {
        return (AZStd::find(m_callbacks.begin(), m_callbacks.end(), callback) != m_callbacks.end());
    }


    void Command::RemoveCallback(Command::Callback* callback, bool delFromMem)
    {
        m_callbacks.erase(AZStd::remove(m_callbacks.begin(), m_callbacks.end(), callback), m_callbacks.end());
        if (delFromMem)
        {
            delete callback;
        }
    }


    void Command::RemoveAllCallbacks()
    {
        const size_t numCallbacks = m_callbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            // If it crashes here, you probably created your callback in another dll and didn't remove it from memory there as well.
            delete m_callbacks[i];
        }

        m_callbacks.clear();
    }


    // calculate the number of registered pre-execute callbacks
    size_t Command::CalcNumPreCommandCallbacks() const
    {
        return AZStd::accumulate(begin(m_callbacks), end(m_callbacks), size_t{0}, [](size_t total, const Callback* callback)
        {
            return callback->GetExecutePreCommand() ? total + 1 : total;
        });
    }


    // calculate the number of registered post-execute callbacks
    size_t Command::CalcNumPostCommandCallbacks() const
    {
        return m_callbacks.size() - CalcNumPreCommandCallbacks();
    }
} // namespace MCore

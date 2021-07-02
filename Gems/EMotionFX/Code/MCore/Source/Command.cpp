/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "Command.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace MCore
{
    // constructor
    Command::Callback::Callback(bool executePreUndo, bool executePreCommand)
    {
        mPreUndoExecute     = executePreUndo;
        mPreCommandExecute  = executePreCommand;
    }


    // destructor
    Command::Callback::~Callback()
    {
    }


    // constructor
    Command::Command(const char* commandName, Command* originalCommand)
    {
        mCommandName = commandName;
        mOrgCommand  = originalCommand;
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
        return mCommandName.c_str();
    }


    const AZStd::string& Command::GetNameString() const
    {
        return mCommandName;
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


    uint32 Command::GetNumCallbacks() const
    {
        return static_cast<uint32>(mCallbacks.size());
    }


    void Command::AddCallback(Command::Callback* callback)
    { 
        mCallbacks.push_back(callback);
    }


    bool Command::CheckIfHasCallback(Command::Callback* callback) const
    {
        return (AZStd::find(mCallbacks.begin(), mCallbacks.end(), callback) != mCallbacks.end());
    }


    void Command::RemoveCallback(Command::Callback* callback, bool delFromMem)
    {
        mCallbacks.erase(AZStd::remove(mCallbacks.begin(), mCallbacks.end(), callback), mCallbacks.end());
        if (delFromMem)
        {
            delete callback;
        }
    }


    void Command::RemoveAllCallbacks()
    {
        const size_t numCallbacks = mCallbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            // If it crashes here, you probably created your callback in another dll and didn't remove it from memory there as well.
            delete mCallbacks[i];
        }

        mCallbacks.clear();
    }


    // calculate the number of registered pre-execute callbacks
    uint32 Command::CalcNumPreCommandCallbacks() const
    {
        uint32 result = 0;

        const size_t numCallbacks = mCallbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            if (mCallbacks[i]->GetExecutePreCommand())
            {
                result++;
            }
        }

        return result;
    }


    // calculate the number of registered post-execute callbacks
    uint32 Command::CalcNumPostCommandCallbacks() const
    {
        uint32 result = 0;

        const size_t numCallbacks = mCallbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            if (mCallbacks[i]->GetExecutePreCommand() == false)
            {
                result++;
            }
        }

        return result;
    }
} // namespace MCore

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UndoSystem.h"

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        URSequencePoint::URSequencePoint(const AZStd::string& friendlyName, URCommandID id)
        {
            m_isPosted = false;
            m_parent = nullptr;
            m_friendlyName = friendlyName;
            m_id = id;

            //AZ_TracePrintf("Undo System", "New Root Point %d\n", id);
        }
        URSequencePoint::URSequencePoint(URCommandID id)
        {
            m_isPosted = false;
            m_parent = nullptr;
            m_id = id;
            m_friendlyName = AZStd::string("Unknown Undo Command");
        }
        URSequencePoint::~URSequencePoint()
        {
            for (ChildVec::iterator it = m_children.begin(); it != m_children.end(); ++it)
            {
                URSequencePoint* childPtr = *it;
                delete childPtr;
            }
            m_children.clear();
        }

        void URSequencePoint::RunUndo()
        {
            // reversed children, then me
            for (ChildVec::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it)
            {
                (*it)->RunUndo();
            }

            Undo();
        }
        void URSequencePoint::RunRedo()
        {
            // me, then children forward
            Redo();

            for (ChildVec::iterator it = m_children.begin(); it != m_children.end(); ++it)
            {
                (*it)->RunRedo();
            }
        }

        void URSequencePoint::Undo()
        {
        }

        void URSequencePoint::Redo()
        {
        }

        URSequencePoint* URSequencePoint::Find(URCommandID id, const AZ::Uuid& typeOfCommand)
        {
            if (*this == id && this->RTTI_IsTypeOf(typeOfCommand))
            {
                return this;
            }

            for (ChildVec::iterator it = m_children.begin(); it != m_children.end(); ++it)
            {
                URSequencePoint* cmd = (*it)->Find(id, typeOfCommand);
                if (cmd)
                {
                    return cmd;
                }
            }

            return nullptr;
        }

        // does it have children objects that do anything?
        bool URSequencePoint::HasRealChildren() const
        {
            if (m_children.empty())
            {
                return false;
            }

            for (auto it = m_children.begin(); it != m_children.end(); ++it)
            {
                URSequencePoint* pChild = *it;
                if ((pChild->RTTI_GetType() != AZ::AzTypeInfo<URSequencePoint>::Uuid()) || (pChild->HasRealChildren()))
                {
                    return true;
                }
            }
            return false;
        }

        void URSequencePoint::SetParent(URSequencePoint* parent)
        {
            if (m_parent != nullptr)
            {
                m_parent->RemoveChild(this);
            }

            m_parent = parent;
            m_parent->AddChild(this);
        }

        void URSequencePoint::SetName(const AZStd::string& friendlyName)
        {
            m_friendlyName = friendlyName;
        }
        void URSequencePoint::AddChild(URSequencePoint* child)
        {
            m_children.push_back(child);
        }

        void URSequencePoint::RemoveChild(URSequencePoint* child)
        {
            const auto it = AZStd::find(m_children.begin(), m_children.end(), child);
            if (it != m_children.end())
            {
                m_children.erase(it);
            }
        }

        AZStd::string& URSequencePoint::GetName()
        {
            return m_friendlyName;
        }

        void URSequencePoint::ApplyToTree(const ApplyOperationCB& applyCB)
        {
            applyCB(this);

            for (ChildVec::iterator it = m_children.begin(); it != m_children.end(); ++it)
            {
                (*it)->ApplyToTree(applyCB);
            }
        }

        //--------------------------------------------------------------------
        UndoStack::UndoStack(int /* no longer used */, IUndoNotify* notify)
            : UndoStack(notify)
        {
        }

        UndoStack::UndoStack(IUndoNotify* notify)
            : m_SequencePointsBuffer()
        {
            m_notify = notify;
            reentryGuard = false;
            m_Cursor = m_CleanPoint = -1;
#ifdef _DEBUG
            CleanCheck();
#endif
        }
        UndoStack::~UndoStack()
        {
            for (int idx = 0; idx < int(m_SequencePointsBuffer.size()); ++idx)
            {
                delete m_SequencePointsBuffer[idx];
                m_SequencePointsBuffer[idx] = nullptr;
            }
        }

        URSequencePoint* UndoStack::Post(URSequencePoint* cmd)
        {
            //AZ_TracePrintf("Undo System", "New command\n");
            AZ_Assert(cmd, "UndoStack Post(nullptr)");
            AZ_Assert(cmd->GetParent() == nullptr, "You may not add undo commands with parents to the undo stack.");
            AZ_Assert(!cmd->IsPosted(), "The given command is posted to the Undo Queue already");
            cmd->m_isPosted = true;

            // this is a new command at the cursor
            // any commands beyond the cursor are invalidated thereby
            Slice();

            m_SequencePointsBuffer.push_back(cmd);
            m_Cursor = int(m_SequencePointsBuffer.size()) - 1;
#ifdef _DEBUG
            CleanCheck();
#endif

            if (m_notify)
            {
                m_notify->OnUndoStackChanged();
            }

            return cmd;
        }

        // by doing this, you take ownership of the memory!
        URSequencePoint* UndoStack::PopTop()
        {
            if (m_SequencePointsBuffer.empty())
            {
                return nullptr;
            }

            //CHB: Slice will notify if there is something to slice,
            //so we may not want to call it again below
            //however if it does not slice we just notify again below
            //so this may generate two calls so we may want to optimize this into one call
            //or something... remember if sliced notified then dont notify again maybe
            Slice();

            URSequencePoint* returned = m_SequencePointsBuffer[m_Cursor];
            m_SequencePointsBuffer.pop_back();
            returned->m_isPosted = false;
            m_Cursor = int(m_SequencePointsBuffer.size()) - 1;

            if (m_notify)
            {
                m_notify->OnUndoStackChanged();
            }

            return returned;
        }

        void UndoStack::SetClean()
        {
            m_CleanPoint = m_Cursor;

            if (m_notify)
            {
                m_notify->OnUndoStackChanged();
            }
#ifdef _DEBUG
            CleanCheck();
#endif
        }

        void UndoStack::Reset()
        {
            m_Cursor = m_CleanPoint = -1;
            for (AZStd::size_t idx = 0; idx < m_SequencePointsBuffer.size(); ++idx)
            {
                if (m_SequencePointsBuffer[idx])
                {
                    delete m_SequencePointsBuffer[idx];
                }
            }
            m_SequencePointsBuffer.clear();

            if (m_notify)
            {
                m_notify->OnUndoStackChanged();
            }

#ifdef _DEBUG
            CleanCheck();
#endif
        }

        URSequencePoint* UndoStack::Undo()
        {
            // AZ_TracePrintf("Undo System", "Undo operation at cursor = %d and buffer size = %d\n", m_Cursor, int(m_SequencePointsBuffer.size()));

            AZ_Assert(!reentryGuard, "UndoStack operations are not reentrant");
            reentryGuard = true;

            if (m_Cursor >= 0)
            {
                m_SequencePointsBuffer[m_Cursor]->RunUndo();
                --m_Cursor;
                if (m_notify)
                {
                    m_notify->OnUndoStackChanged();
                }
            }
#ifdef _DEBUG
            CleanCheck();
#endif

            reentryGuard = false;
            return m_Cursor >= 0 ? m_SequencePointsBuffer[m_Cursor] : nullptr;
        }
        URSequencePoint* UndoStack::Redo()
        {
            // AZ_TracePrintf("Undo System", "Redo operation at cursor = %d and buffer size %d\n", m_Cursor, int(m_SequencePointsBuffer.size()));

            AZ_Assert(!reentryGuard, "UndoStack operations are not reentrant");
            reentryGuard = true;

            if (m_Cursor < int(m_SequencePointsBuffer.size()) - 1)
            {
                ++m_Cursor;
                m_SequencePointsBuffer[m_Cursor]->RunRedo();
#ifdef _DEBUG
                CleanCheck();
#endif
                if (m_notify)
                {
                    m_notify->OnUndoStackChanged();
                }
                reentryGuard = false;
                return m_SequencePointsBuffer[m_Cursor];
            }
#ifdef _DEBUG
            CleanCheck();
#endif

            reentryGuard = false;


            return nullptr;
        }
        void UndoStack::Slice()
        {
            int difference = int(m_SequencePointsBuffer.size()) - 1 - m_Cursor;
            if (difference > 0)
            {
                for (int idx = m_Cursor + 1; idx < int(m_SequencePointsBuffer.size()); ++idx)
                {
                    delete m_SequencePointsBuffer[idx];
                    m_SequencePointsBuffer[idx] = nullptr;
                }
                for (int idx = m_Cursor + 1; idx < int(m_SequencePointsBuffer.size()); )
                {
                    m_SequencePointsBuffer.pop_back();
                }

                if (m_CleanPoint > m_Cursor)
                {
                    // magic number deeper negative than the minimum -1 the cursor can reach
                    m_CleanPoint = -2;
#ifdef _DEBUG
                    CleanCheck();
#endif
                }
                if (m_notify)
                {
                    m_notify->OnUndoStackChanged();
                }
            }
        }

        URSequencePoint* UndoStack::Find(URCommandID id, const AZ::Uuid& typeOfCommand)
        {
            for (int idx = 0; idx < int(m_SequencePointsBuffer.size()); ++idx)
            {
                URSequencePoint* cPtr = m_SequencePointsBuffer[idx]->Find(id, typeOfCommand);
                if (cPtr)
                {
                    return cPtr;
                }
            }
            return nullptr;
        }

        URSequencePoint* UndoStack::GetTop()
        {
            if (m_Cursor >= 0)
            {
                return m_SequencePointsBuffer[m_Cursor];
            }

            return nullptr;
        }

        bool UndoStack::IsClean() const
        {
            return m_Cursor == m_CleanPoint;
        }

        bool UndoStack::CanUndo() const
        {
            return (m_Cursor >= 0);
        }

        bool UndoStack::CanRedo() const
        {
            return (m_Cursor < int(m_SequencePointsBuffer.size()) - 1);
        }


        const char* UndoStack::GetRedoName() const
        {
            if (!CanRedo())
            {
                return "";
            }

            return m_SequencePointsBuffer[m_Cursor + 1]->GetName().c_str();
        }
        const char* UndoStack::GetUndoName() const
        {
            if (!CanUndo())
            {
                return "";
            }

            return m_SequencePointsBuffer[m_Cursor]->GetName().c_str();
        }

#ifdef _DEBUG
        void UndoStack::CleanCheck()
        {
            if (m_Cursor == m_CleanPoint)
            {
                // message CLEAN
                //AZ_TracePrintf("Undo System", "Clean undo cursor\n");
            }
            else
            {
                // message DIRTY
                //AZ_TracePrintf("Undo System", "Dirty undo cursor\n");
            }
        }
#endif
    }
} // namespace AzToolsFramework

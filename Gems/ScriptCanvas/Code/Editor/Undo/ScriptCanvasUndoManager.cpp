/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace ScriptCanvasEditor
{
    static const int c_undoLimit = 100;

    // ScopedUndoBatch

    ScopedUndoBatch::ScopedUndoBatch(AZStd::string_view label)
    {
        UndoRequestBus::Broadcast(&UndoRequests::BeginUndoBatch, label);
    }

    ScopedUndoBatch::~ScopedUndoBatch()
    {
        UndoRequestBus::Broadcast(&UndoRequests::EndUndoBatch);
    }

    // SceneUndoState

    SceneUndoState::SceneUndoState(AzToolsFramework::UndoSystem::IUndoNotify* undoNotify)
    {
        m_undoStack = AZStd::make_unique<AzToolsFramework::UndoSystem::UndoStack>(c_undoLimit, undoNotify);
        m_undoCache = AZStd::make_unique<UndoCache>();
    }

    void SceneUndoState::BeginUndoBatch(AZStd::string_view label)
    {
        if (!m_currentUndoBatch)
        {
            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::BatchCommand(label, 0);
        }
        else
        {
            auto parentUndoBatch = m_currentUndoBatch;

            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::BatchCommand(label, 0);
            m_currentUndoBatch->SetParent(parentUndoBatch);
        }
    }

    void SceneUndoState::EndUndoBatch()
    {
        if (!m_currentUndoBatch)
        {
            return;
        }

        if (m_currentUndoBatch->GetParent())
        {
            // pop one up
            m_currentUndoBatch = m_currentUndoBatch->GetParent();
        }
        else
        {
            // we're at root
            if (m_currentUndoBatch->HasRealChildren() && m_undoStack)
            {
                m_undoStack->Post(m_currentUndoBatch);
            }
            else
            {
                delete m_currentUndoBatch;
            }

            m_currentUndoBatch = nullptr;
        }
    }

    SceneUndoState::~SceneUndoState()
    {
        delete m_currentUndoBatch;
    }

    // UndoCache

    void UndoCache::Clear()
    {
        m_dataMap.clear();
    }

    void UndoCache::PurgeCache(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        m_dataMap.erase(scriptCanvasId);
    }

    void UndoCache::PopulateCache(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        UpdateCache(scriptCanvasId);
    }

    void UndoCache::UpdateCache(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        // Lookup the graph item and perform a snapshot of all it's serialization elements
        UndoData undoData;
        UndoRequestBus::EventResult(undoData, scriptCanvasId, &UndoRequests::CreateUndoData);

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZStd::vector<AZ::u8>& newData = m_dataMap[scriptCanvasId];
        newData.clear();
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&newData);
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *serializeContext, AZ::DataStream::ST_BINARY);
        if (!objStream->WriteClass(&undoData))
        {
            AZ_Assert(false, "Unable to serialize Script Canvas scene and graph data for undo/redo");
            return;
        }

        objStream->Finalize();
    }

    const AZStd::vector<AZ::u8>& UndoCache::Retrieve(ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        auto it = m_dataMap.find(scriptCanvasId);

        if (it == m_dataMap.end())
        {
            return m_emptyData;
        }

        return it->second;
    }
}

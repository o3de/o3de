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

#pragma once

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    template<typename Vertex>
    static const Vertex& ScriptIndexRead(VertexContainer<Vertex>* thisPtr, int index)
    {
        return (*thisPtr)[index - 1];
    }

    template<typename Vertex>
    static void ScriptUpdateVertex(VertexContainer<Vertex>* thisPtr, int index, const Vertex& vertex)
    {
        thisPtr->UpdateVertex(index - 1, vertex);
    }

    template<typename Vertex>
    static void ScriptInsertVertex(VertexContainer<Vertex>* thisPtr, int index, const Vertex& vertex)
    {
        thisPtr->InsertVertex(index - 1, vertex);
    }

    template<typename Vertex>
    static void ScriptRemoveVertex(VertexContainer<Vertex>* thisPtr, int index)
    {
        thisPtr->RemoveVertex(index - 1);
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<VertexContainer<Vertex> >()
                ->Field("Vertices", &VertexContainer<Vertex>::m_vertices)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VertexContainer<Vertex> >("Vertices", "Vertex data")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                    ->DataElement(0, &VertexContainer<Vertex>::m_vertices, "Vertices", "List of vertices.")
                        ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::Hide) // Temporarily hidden for performance reasons.
                        ->Attribute(Edit::Attributes::AddNotify, &VertexContainer<Vertex>::AddNotify)
                        ->Attribute(Edit::Attributes::RemoveNotify, &VertexContainer<Vertex>::RemoveNotify)
                        ->Attribute(Edit::Attributes::ChangeNotify, &VertexContainer<Vertex>::UpdateNotify)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<VertexContainer<Vertex>>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::RuntimeOwn)
                ->Method("IndexRead", &VertexContainer::operator[])
                    ->Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::IndexRead)
                    ->Attribute(Script::Attributes::MethodOverride, &ScriptIndexRead<Vertex>)
                ->Method("Size", &VertexContainer<Vertex>::Size)
                ->Method("AddVertex", &VertexContainer<Vertex>::AddVertex)
                ->Method("UpdateVertex", &VertexContainer<Vertex>::UpdateVertex)
                    ->Attribute(Script::Attributes::MethodOverride, &ScriptUpdateVertex<Vertex>)
                ->Method("InsertVertex", &VertexContainer<Vertex>::InsertVertex)
                    ->Attribute(Script::Attributes::MethodOverride, &ScriptInsertVertex<Vertex>)
                ->Method("RemoveVertex", &VertexContainer<Vertex>::RemoveVertex)
                    ->Attribute(Script::Attributes::MethodOverride, &ScriptRemoveVertex<Vertex>)
                ->Method("Clear", &VertexContainer<Vertex>::Clear)
                ->Method("Empty", &VertexContainer<Vertex>::Empty);
        }
    }

    template<typename Vertex>
    VertexContainer<Vertex>::VertexContainer(
        const IndexFunction& addCallback, const IndexFunction& removeCallback,
        const IndexFunction& updateCallback, const VoidFunction& setCallback,
        const VoidFunction& clearCallback)
            : m_addCallback(addCallback)
            , m_removeCallback(removeCallback)
            , m_updateCallback(updateCallback)
            , m_setCallback(setCallback)
            , m_clearCallback(clearCallback)
    {
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::AddVertex(const Vertex& vertex)
    {
        m_vertices.push_back(vertex);

        if (m_addCallback)
        {
            m_addCallback(m_vertices.size() - 1);
        }
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::UpdateVertex(const size_t index, const Vertex& vertex)
    {
        AZ_Warning("VertexContainer", index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices[index] = vertex;

            if (m_updateCallback)
            {
                m_updateCallback(index);
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::InsertVertex(const size_t index, const Vertex& vertex)
    {
        AZ_Warning("VertexContainer", index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices.insert(m_vertices.data() + index, vertex);

            if (m_addCallback)
            {
                m_addCallback(index);
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::RemoveVertex(const size_t index)
    {
        AZ_Warning("VertexContainer", index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            m_vertices.erase(m_vertices.data() + index);

            if (m_removeCallback)
            {
                m_removeCallback(index);
            }

            return true;
        }

        return false;
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::GetVertex(const size_t index, Vertex& vertex) const
    {
        AZ_Warning("VertexContainer", index < Size(), "Invalid vertex index in %s", __FUNCTION__);
        if (index < Size())
        {
            vertex = m_vertices[index];
            return true;
        }

        return false;
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::GetLastVertex(Vertex& vertex) const
    {
        if (Size() > 0)
        {
            vertex = m_vertices.back();
            return true;
        }

        return false;
    }

    template<typename Vertex>
    template<typename Vertices>
    void VertexContainer<Vertex>::SetVertices(Vertices&& vertices)
    {
        m_vertices = AZStd::forward<Vertices>(vertices);

        if (m_setCallback)
        {
            m_setCallback();
        }
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::Clear()
    {
        m_vertices.clear();

        if (m_clearCallback)
        {
            m_clearCallback();
        }
    }

    template<typename Vertex>
    size_t VertexContainer<Vertex>::Size() const
    {
        return m_vertices.size();
    }

    template<typename Vertex>
    bool VertexContainer<Vertex>::Empty() const
    {
        return m_vertices.empty();
    }

    template<typename Vertex>
    const AZStd::vector<Vertex>& VertexContainer<Vertex>::GetVertices() const
    {
        return m_vertices;
    }

    template<typename Vertex>
    const Vertex& VertexContainer<Vertex>::operator[](const size_t index) const
    {
        return m_vertices[index];
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::SetCallbacks(
        const IndexFunction& addCallback, const IndexFunction& removeCallback,
        const IndexFunction& updateCallback, const VoidFunction& setCallback,
        const VoidFunction& clearCallback)
    {
        m_addCallback = addCallback;
        m_removeCallback = removeCallback;
        m_updateCallback = updateCallback;
        m_setCallback = setCallback;
        m_clearCallback = clearCallback;
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::AddNotify()
    {
        const size_t vertexCount = Size();
        if (vertexCount > 0)
        {
            const size_t lastVertex = vertexCount - 1;
            if (vertexCount > 1)
            {
                m_vertices[lastVertex] = m_vertices[vertexCount - 2];
            }
            else
            {
                m_vertices[lastVertex] = Vertex::CreateZero();
            }

            if (m_addCallback)
            {
                m_addCallback(lastVertex);
            }
        }
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::RemoveNotify(const size_t index) const
    {
        if (m_removeCallback)
        {
            m_removeCallback(index);
        }
    }

    template<typename Vertex>
    void VertexContainer<Vertex>::UpdateNotify(const size_t index) const
    {
        if (m_updateCallback)
        {
            m_updateCallback(index);
        }
    }
}
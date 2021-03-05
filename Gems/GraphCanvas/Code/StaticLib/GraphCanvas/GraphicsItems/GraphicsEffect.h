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

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphicsItems/GraphicsEffectBus.h>

namespace GraphCanvas
{
    class GraphicsEffectInterface
        : public GraphicsEffectRequestBus::Handler
    {
    public:
        GraphicsEffectInterface()
            : m_id(AZ::Entity::MakeId())
        {
            GraphicsEffectRequestBus::Handler::BusConnect(GetEffectId());
        }

        GraphicsEffectId GetEffectId() const override final
        {
            return m_id;
        }

        void SetEditorId(EditorId editorId)
        {
            m_editorId = editorId;
            OnEditorIdSet();
        }

        EditorId GetEditorId() const
        {
            return m_editorId;
        }

        void SetGraphId(const GraphId& graphId)
        {
            m_graphId = graphId;
        }

        GraphId GetGraphId() const
        {
            return m_graphId;
        }

    protected:

        virtual void OnEditorIdSet() {};

    private:

        GraphId          m_graphId;
        EditorId         m_editorId;
        GraphicsEffectId m_id;
    };

    template<class GraphicsClass>
    class GraphicsEffect
        : public GraphicsClass
        , public GraphicsEffectInterface
    {
    public:        
        // GraphicsEffectRequestBus
        virtual QGraphicsItem* AsQGraphicsItem()
        {
            return this;
        }
        
        virtual void OnGraphicsEffectCancelled()
        {            
        }
        ////
    };
}
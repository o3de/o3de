/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        virtual void PrepareGeometryChange() 
        {
            GraphicsClass::prepareGeometryChange();
        }
        
        virtual void OnGraphicsEffectCancelled()
        {            
        }
        ////
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StringIdPool.h>
#include "../StandardPluginsConfig.h"
#include <QItemSelectionModel>
#include <QPainter>
#include <QModelIndex>
#include <QPixmap>
#include <QStaticText>


// specify the maximum node with here, if the node headers or info text gets longer than this they will get elided
#define MAX_NODEWIDTH 180
#define BORDER_RADIUS 7.0

namespace EMotionFX
{
    class AnimGraphInstance;
}

namespace EMStudio
{
    // forward declarations
    class NodeConnection;
    class NodeGraph;
    class GraphNode;


    class NodePort
    {
        MCORE_MEMORYOBJECTCATEGORY(NodePort, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        NodePort()
            : m_isHighlighted(false)  { m_node = nullptr; m_nameId = MCORE_INVALIDINDEX32; m_color.setRgb(50, 150, 250); }

        MCORE_INLINE void SetName(const char* name)         { m_nameId = MCore::GetStringIdPool().GenerateIdForString(name); OnNameChanged(); }
        MCORE_INLINE const char* GetName() const            { return MCore::GetStringIdPool().GetName(m_nameId).c_str(); }
        MCORE_INLINE void SetNameID(uint32 id)              { m_nameId = id; }
        MCORE_INLINE uint32 GetNameID() const               { return m_nameId; }
        MCORE_INLINE void SetRect(const QRect& rect)        { m_rect = rect; }
        MCORE_INLINE const QRect& GetRect() const           { return m_rect; }
        MCORE_INLINE void SetColor(const QColor& color)     { m_color = color; }
        MCORE_INLINE const QColor& GetColor() const         { return m_color; }
        MCORE_INLINE void SetNode(GraphNode* node)          { m_node = node; }
        MCORE_INLINE bool GetIsHighlighted() const          { return m_isHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)    { m_isHighlighted = enabled; }

        void OnNameChanged();

    private:
        QRect           m_rect;
        QColor          m_color;
        GraphNode*      m_node;
        uint32          m_nameId;
        bool            m_isHighlighted;
    };


    class GraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphNode, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        GraphNode(const QModelIndex& modelIndex, const char* name, AZ::u16 numInputs = 0, AZ::u16 numOutputs = 0);
        virtual ~GraphNode();

        const QModelIndex& GetModelIndex() const                            { return m_modelIndex; }

        MCORE_INLINE void UpdateNameAndPorts()                              { m_nameAndPortsUpdated = false; }
        MCORE_INLINE AZStd::vector<NodeConnection*>& GetConnections()        { return m_connections; }
        MCORE_INLINE size_t GetNumConnections()                             { return m_connections.size(); }
        MCORE_INLINE NodeConnection* GetConnection(size_t index)            { return m_connections[index]; }
        MCORE_INLINE NodeConnection* AddConnection(NodeConnection* con)     { m_connections.emplace_back(con); return con; }
        MCORE_INLINE void SetParentGraph(NodeGraph* graph)                  { m_parentGraph = graph; }
        MCORE_INLINE NodeGraph* GetParentGraph()                            { return m_parentGraph; }
        MCORE_INLINE NodePort* GetInputPort(AZ::u16 index)                  { return &m_inputPorts[index]; }
        MCORE_INLINE NodePort* GetOutputPort(AZ::u16 index)                 { return &m_outputPorts[index]; }
        MCORE_INLINE const QRect& GetRect() const                           { return m_rect; }
        MCORE_INLINE const QRect& GetFinalRect() const                      { return m_finalRect; }
        MCORE_INLINE const QRect& GetVizRect() const                        { return m_visualizeRect; }
        MCORE_INLINE void SetBaseColor(const QColor& color)                 { m_baseColor = color; }
        MCORE_INLINE QColor GetBaseColor() const                            { return m_baseColor; }
        MCORE_INLINE bool GetIsVisible() const                              { return m_isVisible; }
        MCORE_INLINE const char* GetName() const                            { return m_name.c_str(); }
        MCORE_INLINE const AZStd::string& GetNameString() const             { return m_name; }

        MCORE_INLINE bool GetCreateConFromOutputOnly() const                { return m_conFromOutputOnly; }
        MCORE_INLINE void SetCreateConFromOutputOnly(bool enable)           { m_conFromOutputOnly = enable; }
        MCORE_INLINE bool GetIsDeletable() const                            { return m_isDeletable; }
        MCORE_INLINE bool GetIsCollapsed() const                            { return m_isCollapsed; }
        void SetIsCollapsed(bool collapsed);
        MCORE_INLINE void SetDeletable(bool deletable)                      { m_isDeletable = deletable; }
        void SetSubTitle(const char* subTitle, bool updatePixmap = true);
        MCORE_INLINE const char* GetSubTitle() const                        { return m_subTitle.c_str(); }
        MCORE_INLINE bool GetIsInsideArrowRect(const QPoint& point) const   { return m_arrowRect.contains(point, true); }

        MCORE_INLINE void SetVisualizeColor(const QColor& color)            { m_visualizeColor = color; }
        MCORE_INLINE const QColor& GetVisualizeColor() const                { return m_visualizeColor; }

        MCORE_INLINE void SetHasChildIndicatorColor(const QColor& color)    { m_hasChildIndicatorColor = color; }
        MCORE_INLINE const QColor& GetHasChildIndicatorColor() const        { return m_hasChildIndicatorColor; }

        MCORE_INLINE bool GetIsHighlighted() const                          { return m_isHighlighted; }
        MCORE_INLINE bool GetIsVisualizedHighlighted() const                { return m_visualizeHighlighted; }
        MCORE_INLINE bool GetIsInsideVisualizeRect(const QPoint& point) const   { return m_visualizeRect.contains(point, true); }

        MCORE_INLINE void SetIsVisualized(bool enabled)                     { m_visualize = enabled; }
        MCORE_INLINE bool GetIsVisualized() const                           { return m_visualize; }

        MCORE_INLINE void SetIsEnabled(bool enabled)                        { m_isEnabled = enabled; }
        MCORE_INLINE bool GetIsEnabled() const                              { return m_isEnabled; }

        MCORE_INLINE void SetCanVisualize(bool canViz)                      { m_canVisualize = canViz; }
        MCORE_INLINE bool GetCanVisualize() const                           { return m_canVisualize; }

        MCORE_INLINE float GetOpacity() const                               { return m_opacity; }
        MCORE_INLINE void SetOpacity(float opacity)                         { m_opacity = opacity; }

        AZ::u16 GetNumInputPorts() const                                    { return aznumeric_caster(m_inputPorts.size()); }
        AZ::u16 GetNumOutputPorts() const                                   { return aznumeric_caster(m_outputPorts.size()); }

        NodePort* AddInputPort(bool updateTextPixMap);
        NodePort* AddOutputPort(bool updateTextPixMap);

        void RemoveAllInputPorts();
        void RemoveAllOutputPorts();
        void RemoveAllConnections();

        bool RemoveConnection(const QModelIndex& modelIndex, bool removeFromMemory = true);
        bool RemoveConnection(const void* connection, bool removeFromMemory = true);

        virtual int32 CalcRequiredHeight() const;
        virtual int32 CalcRequiredWidth();

        virtual int CalcMaxInputPortWidth() const;
        virtual int CalcMaxOutputPortWidth() const;

        bool GetIsInside(const QPoint& globalPoint) const;
        bool GetIsSelected() const;

        void MoveRelative(const QPoint& deltaMove);
        void MoveAbsolute(const QPoint& newUpperLeft);
        virtual void Update(const QRect& visibleRect, const QPoint& mousePos);
        void UpdateRects();

        void RenderConnections(const QItemSelectionModel& selectionModel, QPainter& painter, QPen* pen, QBrush* brush, const QRect& invMappedVisibleRect, int32 stepSize);

        virtual void SetName(const char* name, bool updatePixmap = true);

        void SetNodeInfo(const AZStd::string& info);

        virtual uint32 GetType() const      { return GraphNode::TYPE_ID; }

        virtual void Render(QPainter& painter, QPen* pen, bool renderShadow);
        virtual void RenderHasChildsIndicator(QPainter& painter, QPen* pen, QColor borderColor, QColor bgColor);
        virtual void RenderVisualizeRect(QPainter& painter, const QColor& bgColor, const QColor& bgColor2);

        virtual QRect CalcInputPortRect(AZ::u16 portNr);
        virtual QRect CalcOutputPortRect(AZ::u16 portNr);
        virtual NodePort* FindPort(int32 x, int32 y, AZ::u16* outPortNr, bool* outIsInputPort, bool includeInputPorts);

        virtual bool GetAlwaysColor() const                                                     { return true; }
        virtual bool GetHasError() const                                                        { return true; }

        MCORE_INLINE bool GetIsProcessed() const                                                { return m_isProcessed; }
        MCORE_INLINE void SetIsProcessed(bool processed)                                        { m_isProcessed = processed; }

        MCORE_INLINE bool GetIsUpdated() const                                                  { return m_isUpdated; }
        MCORE_INLINE void SetIsUpdated(bool updated)                                            { m_isUpdated = updated; }

        virtual void Sync() {}

        void CalcOutputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local = false);
        void CalcInputPortTextRect(AZ::u16 portNr, QRect& outRect, bool local = false);
        void CalcInfoTextRect(QRect& outRect, bool local = false);

        MCORE_INLINE void SetHasVisualOutputPorts(bool hasVisualOutputPorts)                    { m_hasVisualOutputPorts = hasVisualOutputPorts; }
        MCORE_INLINE bool GetHasVisualOutputPorts() const                                       { return m_hasVisualOutputPorts; }

        const QColor& GetBorderColor() const                                                    { return m_borderColor; }
        void SetBorderColor(const QColor& color)                                                { m_borderColor = color; }
        void ResetBorderColor()                                                                 { m_borderColor = QColor(0, 0, 0); }

        virtual void UpdateTextPixmap();
        static void RenderText(QPainter& painter, const QString& text, const QColor& textColor, const QFont& font, const QFontMetrics& fontMetrics, Qt::Alignment textAlignment, const QRect& rect);

    protected:
        void RenderShadow(QPainter& painter);

        void GetNodePortColors(NodePort* nodePort, const QColor& borderColor, const QColor& headerBgColor, QColor* outBrushColor, QColor* outPenColor);

        QPersistentModelIndex           m_modelIndex;
        AZStd::string                   m_name;
        QString                         m_elidedName;

        QPainter                        m_textPainter;
        AZStd::string                   m_subTitle;
        QString                         m_elidedSubTitle;
        AZStd::string                   m_nodeInfo;
        QString                         m_elidedNodeInfo;
        QBrush                          m_brush;
        QColor                          m_baseColor;
        QRect                           m_rect;
        QRect                           m_finalRect;
        QRect                           m_arrowRect;
        QRect                           m_visualizeRect;
        QColor                          m_borderColor;
        QColor                          m_visualizeColor;
        QColor                          m_hasChildIndicatorColor;
        AZStd::vector<NodeConnection*>   m_connections;
        float                           m_opacity;
        bool                            m_isVisible;
        static QColor                   s_portHighlightColo;
        static QColor                   s_portHighlightBGColor;

        // font stuff
        QFont                           m_headerFont;
        QFont                           m_portNameFont;
        QFont                           m_subTitleFont;
        QFont                           m_infoTextFont;
        QFontMetrics*                   m_portFontMetrics;
        QFontMetrics*                   m_headerFontMetrics;
        QFontMetrics*                   m_infoFontMetrics;
        QFontMetrics*                   m_subTitleFontMetrics;
        QTextOption                     m_textOptionsCenter;
        QTextOption                     m_textOptionsAlignLeft;
        QTextOption                     m_textOptionsAlignRight;
        QTextOption                     m_textOptionsCenterHv;

        QStaticText                     m_titleText;
        QStaticText                     m_subTitleText;
        QStaticText                     m_infoText;

        AZStd::vector<QStaticText>       m_inputPortText;
        AZStd::vector<QStaticText>       m_outputPortText;

        int32                           m_requiredWidth;
        bool                            m_nameAndPortsUpdated;

        NodeGraph*                      m_parentGraph;
        AZStd::vector<NodePort>          m_inputPorts;
        AZStd::vector<NodePort>          m_outputPorts;
        bool                            m_conFromOutputOnly;
        bool                            m_isDeletable;
        bool                            m_isCollapsed;
        bool                            m_isProcessed;
        bool                            m_isUpdated;
        bool                            m_visualize;
        bool                            m_canVisualize;
        bool                            m_visualizeHighlighted;
        bool                            m_isEnabled;
        bool                            m_isHighlighted;
        bool                            m_canHaveChildren;
        bool                            m_hasVisualGraph;
        bool                            m_hasVisualOutputPorts;

        int                          m_maxInputWidth; // will be calculated automatically in CalcRequiredWidth()
        int                          m_maxOutputWidth; // will be calculated automatically in CalcRequiredWidth()

        // has child node indicator
        QPolygonF                       m_substPoly;
    };
}   // namespace EMStudio

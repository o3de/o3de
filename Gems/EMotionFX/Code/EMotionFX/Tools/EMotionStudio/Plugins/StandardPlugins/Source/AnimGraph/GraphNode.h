/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Color.h>
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
            : mIsHighlighted(false)  { mNode = nullptr; mNameID = MCORE_INVALIDINDEX32; mColor.setRgb(50, 150, 250); }
        ~NodePort() {}

        MCORE_INLINE void SetName(const char* name)         { mNameID = MCore::GetStringIdPool().GenerateIdForString(name); OnNameChanged(); }
        MCORE_INLINE const char* GetName() const            { return MCore::GetStringIdPool().GetName(mNameID).c_str(); }
        MCORE_INLINE void SetNameID(uint32 id)              { mNameID = id; }
        MCORE_INLINE uint32 GetNameID() const               { return mNameID; }
        MCORE_INLINE void SetRect(const QRect& rect)        { mRect = rect; }
        MCORE_INLINE const QRect& GetRect() const           { return mRect; }
        MCORE_INLINE void SetColor(const QColor& color)     { mColor = color; }
        MCORE_INLINE const QColor& GetColor() const         { return mColor; }
        MCORE_INLINE void SetNode(GraphNode* node)          { mNode = node; }
        MCORE_INLINE bool GetIsHighlighted() const          { return mIsHighlighted; }
        MCORE_INLINE void SetIsHighlighted(bool enabled)    { mIsHighlighted = enabled; }

        void OnNameChanged();

    private:
        QRect           mRect;
        QColor          mColor;
        GraphNode*      mNode;
        uint32          mNameID;
        bool            mIsHighlighted;
    };


    class GraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphNode, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        GraphNode(const QModelIndex& modelIndex, const char* name, uint32 numInputs = 0, uint32 numOutputs = 0);
        virtual ~GraphNode();

        const QModelIndex& GetModelIndex() const                            { return m_modelIndex; }

        MCORE_INLINE void UpdateNameAndPorts()                              { mNameAndPortsUpdated = false; }
        MCORE_INLINE MCore::Array<NodeConnection*>& GetConnections()        { return mConnections; }
        MCORE_INLINE uint32 GetNumConnections()                             { return mConnections.GetLength(); }
        MCORE_INLINE NodeConnection* GetConnection(uint32 index)            { return mConnections[index]; }
        MCORE_INLINE NodeConnection* AddConnection(NodeConnection* con)     { mConnections.Add(con); return con; }
        MCORE_INLINE void SetParentGraph(NodeGraph* graph)                  { mParentGraph = graph; }
        MCORE_INLINE NodeGraph* GetParentGraph()                            { return mParentGraph; }
        MCORE_INLINE NodePort* GetInputPort(uint32 index)                   { return &mInputPorts[index]; }
        MCORE_INLINE NodePort* GetOutputPort(uint32 index)                  { return &mOutputPorts[index]; }
        MCORE_INLINE const QRect& GetRect() const                           { return mRect; }
        MCORE_INLINE const QRect& GetFinalRect() const                      { return mFinalRect; }
        MCORE_INLINE const QRect& GetVizRect() const                        { return mVisualizeRect; }
        MCORE_INLINE void SetBaseColor(const QColor& color)                 { mBaseColor = color; }
        MCORE_INLINE QColor GetBaseColor() const                            { return mBaseColor; }
        MCORE_INLINE bool GetIsVisible() const                              { return mIsVisible; }
        MCORE_INLINE const char* GetName() const                            { return mName.c_str(); }
        MCORE_INLINE const AZStd::string& GetNameString() const             { return mName; }

        MCORE_INLINE bool GetCreateConFromOutputOnly() const                { return mConFromOutputOnly; }
        MCORE_INLINE void SetCreateConFromOutputOnly(bool enable)           { mConFromOutputOnly = enable; }
        MCORE_INLINE bool GetIsDeletable() const                            { return mIsDeletable; }
        MCORE_INLINE bool GetIsCollapsed() const                            { return mIsCollapsed; }
        void SetIsCollapsed(bool collapsed);
        MCORE_INLINE void SetDeletable(bool deletable)                      { mIsDeletable = deletable; }
        void SetSubTitle(const char* subTitle, bool updatePixmap = true);
        MCORE_INLINE const char* GetSubTitle() const                        { return mSubTitle.c_str(); }
        //MCORE_INLINE const AZStd::string& GetSubTitleString() const           { return mSubTitle; }
        MCORE_INLINE bool GetIsInsideArrowRect(const QPoint& point) const   { return mArrowRect.contains(point, true); }

        MCORE_INLINE void SetVisualizeColor(const QColor& color)            { mVisualizeColor = color; }
        MCORE_INLINE const QColor& GetVisualizeColor() const                { return mVisualizeColor; }

        MCORE_INLINE void SetHasChildIndicatorColor(const QColor& color)    { mHasChildIndicatorColor = color; }
        MCORE_INLINE const QColor& GetHasChildIndicatorColor() const        { return mHasChildIndicatorColor; }

        MCORE_INLINE bool GetIsHighlighted() const                          { return mIsHighlighted; }
        MCORE_INLINE bool GetIsVisualizedHighlighted() const                { return mVisualizeHighlighted; }
        MCORE_INLINE bool GetIsInsideVisualizeRect(const QPoint& point) const   { return mVisualizeRect.contains(point, true); }

        MCORE_INLINE void SetIsVisualized(bool enabled)                     { mVisualize = enabled; }
        MCORE_INLINE bool GetIsVisualized() const                           { return mVisualize; }

        MCORE_INLINE void SetIsEnabled(bool enabled)                        { mIsEnabled = enabled; }
        MCORE_INLINE bool GetIsEnabled() const                              { return mIsEnabled; }

        MCORE_INLINE void SetCanVisualize(bool canViz)                      { mCanVisualize = canViz; }
        MCORE_INLINE bool GetCanVisualize() const                           { return mCanVisualize; }

        MCORE_INLINE float GetOpacity() const                               { return mOpacity; }
        MCORE_INLINE void SetOpacity(float opacity)                         { mOpacity = opacity; }

        uint32 GetNumInputPorts() const                                     { return mInputPorts.GetLength(); }
        uint32 GetNumOutputPorts() const                                    { return mOutputPorts.GetLength(); }

        NodePort* AddInputPort(bool updateTextPixMap);
        NodePort* AddOutputPort(bool updateTextPixMap);

        void RemoveAllInputPorts();
        void RemoveAllOutputPorts();
        void RemoveAllConnections();

        bool RemoveConnection(const QModelIndex& modelIndex, bool removeFromMemory = true);
        bool RemoveConnection(const void* connection, bool removeFromMemory = true);

        virtual int32 CalcRequiredHeight() const;
        virtual int32 CalcRequiredWidth();

        virtual uint32 CalcMaxInputPortWidth() const;
        virtual uint32 CalcMaxOutputPortWidth() const;

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

        virtual QRect CalcInputPortRect(uint32 portNr);
        virtual QRect CalcOutputPortRect(uint32 portNr);
        virtual NodePort* FindPort(int32 x, int32 y, uint32* outPortNr, bool* outIsInputPort, bool includeInputPorts);

        virtual bool GetAlwaysColor() const                                                     { return true; }
        virtual bool GetHasError() const                                                        { return true; }

        MCORE_INLINE bool GetIsProcessed() const                                                { return mIsProcessed; }
        MCORE_INLINE void SetIsProcessed(bool processed)                                        { mIsProcessed = processed; }

        MCORE_INLINE bool GetIsUpdated() const                                                  { return mIsUpdated; }
        MCORE_INLINE void SetIsUpdated(bool updated)                                            { mIsUpdated = updated; }

        virtual void Sync() {}

        void CalcOutputPortTextRect(uint32 portNr, QRect& outRect, bool local = false);
        void CalcInputPortTextRect(uint32 portNr, QRect& outRect, bool local = false);
        void CalcInfoTextRect(QRect& outRect, bool local = false);

        MCORE_INLINE void SetHasVisualOutputPorts(bool hasVisualOutputPorts)                    { mHasVisualOutputPorts = hasVisualOutputPorts; }
        MCORE_INLINE bool GetHasVisualOutputPorts() const                                       { return mHasVisualOutputPorts; }

        const QColor& GetBorderColor() const                                                    { return mBorderColor; }
        void SetBorderColor(const QColor& color)                                                { mBorderColor = color; }
        void ResetBorderColor()                                                                 { mBorderColor = QColor(0, 0, 0); }

        virtual void UpdateTextPixmap();
        static void RenderText(QPainter& painter, const QString& text, const QColor& textColor, const QFont& font, const QFontMetrics& fontMetrics, Qt::Alignment textAlignment, const QRect& rect);

    protected:
        void RenderShadow(QPainter& painter);

        void GetNodePortColors(NodePort* nodePort, const QColor& borderColor, const QColor& headerBgColor, QColor* outBrushColor, QColor* outPenColor);

        QPersistentModelIndex           m_modelIndex;
        AZStd::string                   mName;
        QString                         mElidedName;

        QPainter                        mTextPainter;
        //QPixmap                           mTextPixmap;
        AZStd::string                   mSubTitle;
        QString                         mElidedSubTitle;
        AZStd::string                   mNodeInfo;
        QString                         mElidedNodeInfo;
        QBrush                          mBrush;
        QColor                          mBaseColor;
        QRect                           mRect;
        QRect                           mFinalRect;
        QRect                           mArrowRect;
        QRect                           mVisualizeRect;
        QColor                          mBorderColor;
        QColor                          mVisualizeColor;
        QColor                          mHasChildIndicatorColor;
        MCore::Array<NodeConnection*>   mConnections;
        float                           mOpacity;
        bool                            mIsVisible;
        static QColor                   mPortHighlightColor;
        static QColor                   mPortHighlightBGColor;

        // font stuff
        QFont                           mHeaderFont;
        QFont                           mPortNameFont;
        QFont                           mSubTitleFont;
        QFont                           mInfoTextFont;
        QFontMetrics*                   mPortFontMetrics;
        QFontMetrics*                   mHeaderFontMetrics;
        QFontMetrics*                   mInfoFontMetrics;
        QFontMetrics*                   mSubTitleFontMetrics;
        QTextOption                     mTextOptionsCenter;
        QTextOption                     mTextOptionsAlignLeft;
        QTextOption                     mTextOptionsAlignRight;
        QTextOption                     mTextOptionsCenterHV;

        QStaticText                     mTitleText;
        QStaticText                     mSubTitleText;
        QStaticText                     mInfoText;

        MCore::Array<QStaticText>       mInputPortText;
        MCore::Array<QStaticText>       mOutputPortText;

        int32                           mRequiredWidth;
        bool                            mNameAndPortsUpdated;

        NodeGraph*                      mParentGraph;
        MCore::Array<NodePort>          mInputPorts;
        MCore::Array<NodePort>          mOutputPorts;
        bool                            mConFromOutputOnly;
        bool                            mIsDeletable;
        bool                            mIsCollapsed;
        bool                            mIsProcessed;
        bool                            mIsUpdated;
        bool                            mVisualize;
        bool                            mCanVisualize;
        bool                            mVisualizeHighlighted;
        bool                            mIsEnabled;
        bool                            mIsHighlighted;
        bool                            mCanHaveChildren;
        bool                            mHasVisualGraph;
        bool                            mHasVisualOutputPorts;

        uint32                          mMaxInputWidth; // will be calculated automatically in CalcRequiredWidth()
        uint32                          mMaxOutputWidth; // will be calculated automatically in CalcRequiredWidth()

        // has child node indicator
        QPolygonF                       mSubstPoly;
    };
}   // namespace EMStudio

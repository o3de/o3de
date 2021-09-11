/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#if !defined(Q_MOC_RUN)
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QPlainTextEdit>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <Widgets/GraphCanvasLabel.h>
#endif

namespace GraphCanvas
{
    namespace Internal
    {
        // Need to know when the text edit gets focus in order to
        // manage the layout display when the mouse hovers off, but the
        // widget still has focus. Qt does not expose focus events in any
        // signal way, so this exposes that functionality for me.
        class FocusableTextEdit
            : public QTextEdit
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(FocusableTextEdit, AZ::SystemAllocator, 0);
            FocusableTextEdit()
                : m_eatEnterKey(false)
            {
                setContextMenuPolicy(Qt::ContextMenuPolicy::PreventContextMenu);
            }

            ~FocusableTextEdit() = default;

        signals:
            void OnFocusIn();
            void OnFocusOut();

            void EnterPressed();

        private:

            void focusInEvent(QFocusEvent* focusEvent) override
            {
                QTextEdit::focusInEvent(focusEvent);
                emit OnFocusIn();
            }

            void focusOutEvent(QFocusEvent* focusEvent) override
            {
                QTextEdit::focusOutEvent(focusEvent);
                emit OnFocusOut();
            }

            void keyPressEvent(QKeyEvent* keyEvent) override
            {
                if (keyEvent->key() == Qt::Key_Enter
                    || keyEvent->key() == Qt::Key_Return)
                {
                    if (keyEvent->modifiers() == Qt::KeyboardModifier::NoModifier)
                    {
                        m_eatEnterKey = true;
                        return;
                    }
                }

                QTextEdit::keyPressEvent(keyEvent);
            }

            void keyReleaseEvent(QKeyEvent* keyEvent) override
            {
                if (keyEvent->key() == Qt::Key_Enter
                    || keyEvent->key() == Qt::Key_Return)
                {
                    if (m_eatEnterKey)
                    {
                        emit EnterPressed();
                        m_eatEnterKey = false;
                    }
                }

                QTextEdit::keyReleaseEvent(keyEvent);
            }

            bool m_eatEnterKey;
        };
    }

    //! The QGraphicsWidget for displaying the comment text
    // This class is not meant to be serializable
    class CommentTextGraphicsWidget
        : public QGraphicsWidget
        , public CommentLayoutRequestBus::Handler
        , public CommentUIRequestBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(CommentTextGraphicsWidget, "{1779F401-6A9F-42A8-B4B7-F7732DBEC462}");
        AZ_CLASS_ALLOCATOR(CommentTextGraphicsWidget, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context) = delete;

        CommentTextGraphicsWidget(const AZ::EntityId& targetId);
        ~CommentTextGraphicsWidget() override = default;

        void Activate();
        void Deactivate();

        void OnAddedToScene();

        void SetStyle(const AZStd::string& style);

        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();

        void SetComment(const AZStd::string& comment);
        AZStd::string GetComment() const;

        // NOTE: Currently the style helper does not signal out when it's value has changed.
        //       As such, any modifications to the style helper will need to call OnStyleChanged in order
        //       to propogate those changes.
        Styling::StyleHelper& GetStyleHelper();

        const Styling::StyleHelper& GetStyleHelper() const;

        void SetCommentMode(CommentMode commentMode);
        CommentMode GetCommentMode() const;

        // CommentUIRequestBus
        void SetEditable(bool editable) override;
        ////

        // CommentLayoutRequestBus
        QGraphicsLayoutItem* GetGraphicsLayoutItem() override;
        ////
        
        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

    protected:        

        void UpdateSizing();
        void SubmitValue();
        void UpdateSizePolicies();
        
        bool sceneEventFilter(QGraphicsItem*, QEvent* event) override;

        const AZ::EntityId& GetEntityId() const { return m_entityId; }
        void SetupProxyWidget();
        void CleanupProxyWidget();

    private:
        CommentTextGraphicsWidget(const CommentTextGraphicsWidget&) = delete;

        CommentMode m_commentMode;
        AZStd::string m_commentText;

        bool m_editable;
        bool m_layoutLock;

        QGraphicsLinearLayout* m_layout;

        GraphCanvasLabel*               m_displayLabel;
        Internal::FocusableTextEdit*    m_textEdit;
        QGraphicsProxyWidget*           m_proxyWidget;

        Styling::StyleHelper m_styleHelper;
        AZStd::string        m_style;
        
        QPointF m_initialClick;
        bool m_pressed;

        AZ::EntityId m_entityId;
    };
}

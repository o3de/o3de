/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#endif
#include <Editor/ObjectEditorCardPool.h>
#include <Editor/InspectorBus.h>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QScrollArea)

namespace EMStudio
{
    class ContentWidget;
    class NoSelectionWidget;

    //! Unified inspector window
    //! This plugin handles requests from the inspector bus and is used to show properties of
    //! selected objects in the Animation Editor.
    class InspectorWindow
        : public DockWidgetPlugin
        , public InspectorRequestBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x20201006
        };

        InspectorWindow() = default;
        ~InspectorWindow() override;

        // DockWidgetPlugin overrides...
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new InspectorWindow(); }
        const char* GetName() const override { return "Inspector"; }
        uint32 GetClassID() const override { return CLASS_ID; }

        // InspectorRequestBus overrides...
        void Update(const QString& headerTitle, const QString& iconFilename, QWidget* widget) override;
        void UpdateWithRpe(const QString& headerTitle, const QString& iconFilename, const AZStd::vector<CardElement>& cardElements) override;
        void Clear() override;

    private:
        void InternalShow(QWidget* widget);

        AZ::SerializeContext* GetSerializeContext();
        AZ::SerializeContext* m_serializeContext = nullptr;

        QScrollArea* m_scrollArea = nullptr;

        ContentWidget* m_contentWidget = nullptr;
        NoSelectionWidget* m_noSelectionWidget = nullptr;

        ObjectEditorCardPool m_objectEditorCardPool;
    };
} // namespace EMStudio

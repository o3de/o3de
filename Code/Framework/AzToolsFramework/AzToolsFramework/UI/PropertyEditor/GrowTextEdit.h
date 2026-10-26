/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
 
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QTextEdit>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    class AZTF_API GrowTextEdit : public QTextEdit
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit GrowTextEdit(QWidget* parent = nullptr);

        void SetText(const AZStd::string& text);
        AZStd::string GetText() const;

        void setVisible(bool visible) override;
        QSize sizeHint() const override;

        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        bool event(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
    signals:
        void EditCompleted();
    private:
        static const int s_padding;
        static const int s_minHeight;
        static const int s_maxHeight;
        QString m_textOnFocusIn;
        bool m_textChanged;
    };
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiGem.h"
#include <ImGuiBus.h>

#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QAbstractSpinBox>
#include <QComboBox>

namespace ImGui
{
    // ==================================================================================
    // Editor Console Key Suppression Handler
    // Prevents the ImGui console key (Home) from toggling the ImGui overlay
    // when a text-editing widget has keyboard focus. This lets Home/End keys
    // work normally for cursor navigation in spinboxes, line edits, text areas, etc.
    // ==================================================================================
    class EditorConsoleKeySuppression
        : public IImGuiConsoleKeySuppression::Bus::Handler
    {
    public:
        EditorConsoleKeySuppression()
        {
            IImGuiConsoleKeySuppression::Bus::Handler::BusConnect();
        }

        ~EditorConsoleKeySuppression() override
        {
            IImGuiConsoleKeySuppression::Bus::Handler::BusDisconnect();
        }

        bool ShouldSuppressConsoleKeyToggle() const override
        {
            QWidget* focusWidget = QApplication::focusWidget();
            if (!focusWidget)
            {
                return false;
            }

            // Suppress if any text-editing widget has keyboard focus
            if (qobject_cast<QLineEdit*>(focusWidget) ||
                qobject_cast<QAbstractSpinBox*>(focusWidget) ||
                qobject_cast<QTextEdit*>(focusWidget) ||
                qobject_cast<QPlainTextEdit*>(focusWidget))
            {
                return true;
            }

            // Editable combo boxes contain a QLineEdit -- suppress for those too
            if (auto* combo = qobject_cast<QComboBox*>(focusWidget))
            {
                if (combo->isEditable())
                {
                    return true;
                }
            }

            return false;
        }
    };

    // ==================================================================================
    // Editor Window Module
    // ==================================================================================
    class ImGuiEditorWindowModule
        : public ImGuiModule
    {
    public:
        AZ_RTTI(ImGuiEditorWindowModule, "{DDC7A763-A36F-46D8-9885-43E0293C1D03}", ImGuiModule);

        ImGuiEditorWindowModule()
            : ImGuiModule()
        {
            m_consoleKeySuppression = AZStd::make_unique<EditorConsoleKeySuppression>();
        }

    private:
        AZStd::unique_ptr<EditorConsoleKeySuppression> m_consoleKeySuppression;
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImGui::ImGuiEditorWindowModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImGuiEditorWindow, ImGui::ImGuiEditorWindowModule)
#endif

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#define CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Component.h>

#include <functional>
#include <QSharedPointer>
#endif

// CViewportTitleDlg dialog
class CLayoutViewPane;
class CPopupMenuItem;

class QAbstractButton;
class QMenu;

struct ICVar;

namespace Ui
{
    class ViewportTitleDlg;
}

//////////////////////////////////////////////////////////////////////////
class CViewportTitleDlg
    : public QWidget
{
    Q_OBJECT
public:
    CViewportTitleDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CViewportTitleDlg();

    void SetViewPane(CLayoutViewPane* pViewPane);
    void SetTitle(const QString& title);

    static void LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets);
    static void SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets);
    static void UpdateCustomPresets(const QString& text, QStringList& custompresets);

    bool eventFilter(QObject* object, QEvent* event) override;

    void SetNoViewportInfo();
    void SetNormalViewportInfo();
    void SetFullViewportInfo();
    void SetCompactViewportInfo();
    void OnToggleDisplayInfo();

Q_SIGNALS:
    void ActionTriggered(int command);

protected:
    virtual void OnInitDialog();

    void OnMaximize();

    QString m_title;

    CLayoutViewPane* m_pViewPane;

    static const int MAX_NUM_CUSTOM_PRESETS = 10;
    QStringList m_customResPresets;
    QStringList m_customFOVPresets;
    QStringList m_customAspectRatioPresets;

    float m_prevMoveSpeedScale;

    // Speed combobox/lineEdit settings
    double m_speedScaleMin = 0.001;
    double m_speedScaleMax = 100.0;
    double m_speedScaleStep = 0.01;
    int m_speedScaleDecimalCount = 3;

    void OnMenuFOVCustom();
    void OnMenuAspectRatioCustom();
    void OnMenuResolutionCustom();
    
    void OnBnClickedGotoPosition();
    void OnBnClickedMuteAudio();

    QScopedPointer<Ui::ViewportTitleDlg> m_ui;

    //! The different prefab edit mode effects available in the Edit mode menu.
    enum class PrefabEditModeUXSetting
    {
        Normal, //!< No effect.
        Monochromatic //!< Monochromatic effect.
    };

    //! The currently active edit mode effect.
    PrefabEditModeUXSetting m_prefabEditMode = PrefabEditModeUXSetting::Monochromatic;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor.
    class ViewportTitleDlgPythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ViewportTitleDlgPythonFuncsHandler, "{2D686C2D-04F0-4C96-B432-0702E774062E}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework


#endif // CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QStack>
#include <QString>
#include <QStyle>
#include <QVector>
#include <QWidget>

#endif

class QSettings;
class QToolButton;
class QLabel;

namespace AzQtComponents
{
    class Style;

    template <typename E>
    constexpr auto EnumToConstExprInt(E e) noexcept
    {
        return static_cast<typename AZStd::underlying_type<E>::type>(e);
    }

    //! Enum listing the type of buttons 
    enum class NavigationButton
    {
        Back,       //!< Back button    - restores previous path when clicked.
        Forward,    //!< Forward button - navigates to latest path before the back button was clicked.
        Browse,     //!< Browse button  - can be used to add browsing capabilities to select a path from disk.

        Count       //!< Counter for the number of possible values.
    };

    using BreadCrumbButtonStates = bool[EnumToConstExprInt(NavigationButton::Count)];

    //! Specialized widget to create html-style clickable links for each part of a path.
    //! Provides state management and signals for handling common path navigation functionality (back, forward and up).
    //! Can be styled by modifying BreadCrumbs.qss and BreadCrumbsConfig.ini.
    //! Note that Qt doesn't handle HTML embedded in QLabel widgets gracefully - stylesheets are not shared between those
    //! and Qt objects. As a result, the color of the HTML styled links in the BreadCrumbs is specified in BreadCrumbsConfig.ini.
    class AZ_QT_COMPONENTS_API BreadCrumbs
        : public QWidget
    {
        Q_OBJECT

    public:
        //! Style configuration for the Breadcrumbs class.
        struct Config
        {
            QString linkColor;      //!< Color for links. Must be a string using the hex format #rrggbb.
            float optimalPathWidth; //!< The portion of the total width used to display the path. Must be a value between 0.0 and 1.0.
        };

        explicit BreadCrumbs(QWidget* parent = nullptr);
        ~BreadCrumbs();

        //! Returns true if it is possible to move back in the navigation stack, false otherwise.
        bool isBackAvailable() const;
        //! Returns true if it is possible to move forward in the navigation stack, false otherwise.
        bool isForwardAvailable() const;
        //! Returns true if the current breadcrumb path has a parent that can be added to the navigation stack, false otherwise.
        bool isUpAvailable() const;

        //! Returns a string with the current breadcrumb path.
        QString currentPath() const;
        //! Sets the current breadcrumb path without updating the navigation stack.
        void setCurrentPath(const QString& newPath);

        //! Returns true if activating a link should automatically push a new path to the navigation stack.
        bool getPushPathOnLinkActivation() const;
        //! Sets whether activating a link should automatically push a new path to the navigation stack.
        void setPushPathOnLinkActivation(bool pushPath);

        //! Creates a button of the type specified.
        //! @param type The type of button to create.
        //! @return The pointer to the newly created QToolButton.
        //! Note: the button will need to be added to a layout manually.
        QToolButton* createButton(NavigationButton type);
        //! Creates a widget containing the back and forward buttons.
        //! @return The pointer to the newly created QWidget.
        //! Note: the widget will need to be added to a layout manually.
        QWidget* createBackForwardToolBar();
        //! Creates a separator styled to match the Breadcrumbs widget.
        //! @return The pointer to the newly created separator.
        //! Note: the separator will need to be added to a layout manually.
        QWidget* createSeparator();

        //! Sets the Breadcrumbs style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Breadcrumbs.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Breadcrumbs style configuration.
        static Config defaultConfig();

    public Q_SLOTS:
        //! Pushes a path to be shown in the Breadcrumbs widget.
        void pushPath(const QString& fullPath);
        //! Restores the previous breadcrumb path from the navigation stack if it exists.
        bool back();
        //! Restores the next breadcrumb path from the navigation stack if it exists.
        bool forward();

    Q_SIGNALS:
        //! Triggered when the currently displayed path changes via any of the slots.
        //! @return fullPath The new path after the change.
        void pathChanged(const QString& fullPath);
        //! Triggered when a link is clicked.
        //! @param linkPath The path of the clicked link.
        //! @param linkIndex The index of the link.
        void linkClicked(const QString& linkPath, int linkIndex);
        //! Triggered after a change to the navigation stack, if back is available. Used to update back buttons.
        //! @param enabled The new back availability state after the change.
        void backAvailabilityChanged(bool enabled);
        //! Triggered after a change to the navigation stack, if forward is available. Used to update forward buttons.
        //! @param enabled The new forward availability state after the change.
        void forwardAvailabilityChanged(bool enabled);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private Q_SLOTS:
        void onLinkActivated(const QString& link);

    private:
        void fillLabel();
        void changePath(const QString& newPath);

        void getButtonStates(BreadCrumbButtonStates buttonStates);
        void emitButtonSignals(BreadCrumbButtonStates previousButtonStates);

        void showTruncatedPathsMenu();

        static QString buildPathFromList(const QStringList& fullPath, int pos);

        QLabel* m_label = nullptr;
        QToolButton* m_menuButton = nullptr;

        QString m_currentPath;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::BreadCrumbs::m_backPaths': class 'QStack<QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::BreadCrumbs'
        QStack<QString> m_backPaths;
        QStack<QString> m_forwardPaths;
        Config m_config;
        QStringList m_truncatedPaths;
        AZ_POP_DISABLE_WARNING
        bool m_pushPathOnLinkActivation = true;

        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents

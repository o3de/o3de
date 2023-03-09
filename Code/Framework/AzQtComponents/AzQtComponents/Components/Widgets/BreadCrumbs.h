/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QMenu>
#include <QPointer>
#include <QStack>
#include <QString>
#include <QStyle>
#include <QVector>
#include <QFrame>

#endif

class QSettings;
class QToolButton;
class QLabel;
class QLineEdit;
class QStackedWidget;

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
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(bool editable READ isEditable WRITE setEditable)

    public:
        //! Style configuration for the Breadcrumbs class.
        struct Config
        {
            QString disabledLinkColor;  //!< Color for disabled links. Must be a string using the hex format #rrggbb.
            QString linkColor;          //!< Color for links. Must be a string using the hex format #rrggbb.
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
        //! Returns the full path if the current path is not the full path.
        QString fullPath() const;
        //! Sets the current breadcrumb path without updating the navigation stack.
        void setCurrentPath(const QString& newPath);

        //! Sets a default icon for path elements.
        void setDefaultIcon(const QString& iconPath);
        //! Sets an icon for the path element at index.
        void setIconAt(int index, const QString& iconPath);
        //! Gets the icon for the path element at index.
        QIcon iconAt(int index);

        bool isEditable() const;
        void setEditable(bool editable);

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

        //! Size hint reports the space that would be taken if the whole
        //! path would be shown.
        QSize sizeHint() const override;

        //! Sets the Breadcrumbs style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Breadcrumbs.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Breadcrumbs style configuration.
        static Config defaultConfig();

    public Q_SLOTS:
        //! Pushes a path to be shown in the Breadcrumbs widget.
        void pushPath(const QString& fullPath);
        //! Pushes a new full path to be displayed in the editable breadcrumbs,
        //! and a new path to be shown in the breadcrumbs navigation bar.
        void pushFullPath(const QString& newFullPath, const QString& newPath);
        //! Restores the previous breadcrumb path from the navigation stack if it exists.
        bool back();
        //! Restores the next breadcrumb path from the navigation stack if it exists.
        bool forward();
        //! Initiate editing of the path in BreadCrumbs.
        //!
        //! This could be called when e.g. user presses some combination like Ctrl+L in
        //! an appropriate context.
        //! It will work only for editable instances (@see isEditable()).
        //! Calling this on BreadCrumbs that are already edited is a no-op.
        void startEditing();

    Q_SIGNALS:
        //! Triggered when the currently displayed path changes via any of the slots.
        //! @return fullPath The new path after the change.
        void pathChanged(const QString& fullPath);
        //! Triggered after user changes the path in editable BreadCrumbs.
        //!
        //! @warning the actual path won't be pushed and the BreadCrumbs won't change. It's up to the user of this class to first check
        //! the validity of the path provided by the user and call e.g. pushPath() to actually change what is stored in the breadcrumbs
        void pathEdited(const QString& requestedPath);
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
        void changeEvent(QEvent* event) override;
        bool eventFilter(QObject* obj, QEvent* ev) override;

    private Q_SLOTS:
        void onLinkActivated(const QString& link);
        void confirmEdit();
        void cancelEdit();

    private:
        QString generateIconHtml(int index);
        void fillLabel();
        void changePath(const QString& newPath);

        void getButtonStates(BreadCrumbButtonStates buttonStates);
        void emitButtonSignals(BreadCrumbButtonStates previousButtonStates);

        void showTruncatedPathsMenu();
        bool isEditing() const;

        static QString buildPathFromList(const QStringList& fullPath, int pos);


        QLabel* m_label = nullptr;
        QLineEdit* m_lineEdit = nullptr;
        QToolButton* m_menuButton = nullptr;
        QStackedWidget* m_labelEditStack = nullptr;

        QString m_currentPath;
        QString m_fullPath;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::BreadCrumbs::m_backPaths': class 'QStack<QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::BreadCrumbs'
        QStack<QString> m_backPaths;
        QStack<QString> m_forwardPaths;
        Config m_config;
        QStringList m_truncatedPaths;
        QPointer<QMenu> m_contextMenu = nullptr;
        AZ_POP_DISABLE_WARNING
        bool m_pushPathOnLinkActivation = true;
        bool m_editable = false;
        int m_currentPathSize = 0;
        QString m_defaultIcon;
        AZ_PUSH_DISABLE_WARNING(
            4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::BreadCrumbs::m_currentPathIcons': class 'QVector<QIcon>' needs to have
                                              // dll-interface to be used by clients of class 'AzQtComponents::BreadCrumbs'
        QVector<QString> m_currentPathIcons;
        AZ_POP_DISABLE_WARNING

        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents

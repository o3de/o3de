/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/LogicalTabOrderingWidget.h>

#include <QColor>
#include <QHash>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QVector>
#endif

class QAction;
class QActionGroup;
class QCheckBox;
class QDialogButtonBox;
class QGridLayout;
class QHBoxLayout;
class QMenu;
class QScrollArea;
class QSettings;
class QToolButton;
class QUndoStack;
class QLabel;
class QLineEdit;

namespace AzQtComponents
{
    class ColorHexEdit;
    class ColorPreview;
    class ColorRGBAEdit;
    class ColorValidator;
    class ColorWarning;
    class DoubleSpinBox;
    class Eyedropper;
    class GammaEdit;
    class GradientSlider;
    class HSLSliders;
    class HSVSliders;
    class PaletteCard;
    class PaletteCardCollection;
    class PaletteModel;
    class PaletteView;
    class QuickPaletteCard;
    class RGBSliders;
    class Style;
    class TabWidget;

    namespace Internal
    {
        class ColorController;
        struct ColorLibrarySettings
        {
            bool expanded;
        };
    }

    //! Allows the user to select a color via a dialog window.
    AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::LogicalTabOrderingWidget<AzQtComponents::StyledDialog>::m_entries': class 'QMap<QObject *,AzQtComponents::LogicalTabOrderingInternal::TabKeyEntry>' needs to have dll-interface to be used by clients of class 'AzQtComponents::LogicalTabOrderingWidget<AzQtComponents::StyledDialog>'
    class AZ_QT_COMPONENTS_API ColorPicker
        : public LogicalTabOrderingWidget<StyledDialog>
    {
        Q_OBJECT
        
        //! The previously selected color in the dialog. It is updated to the current color when the OK button is pressed.
        Q_PROPERTY(AZ::Color selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged USER true)
        //! The current color in the dialog. Updates in real time when the user changes settings in the dialog.
        Q_PROPERTY(AZ::Color currentColor READ currentColor WRITE setCurrentColor NOTIFY currentColorChanged USER true)

    public:
        //! Available color configurations.
        enum class Configuration
        {
            RGBA,           
            RGB,
            HueSaturation   //!< Simplified mode for picking lighting related values.
        };

        //! Style configuration for the color grid.
        struct ColorGridConfig
        {
            QSize minimumSize;                      //!< Minimum size for the color grid widget, in pixels.
        };

        //! Style configuration for the dialog buttons.
        struct DialogButtonsConfig
        {
            int topPadding;                         //!< Top padding for the dialog buttons, in pixels.
        };

        //! Style configuration for the ColorPicker class.
        struct Config
        {
            int padding;                            //!< Horizontal padding of the dialog window, in pixels.
            int spacing;                            //!< Spacing between layout elements, in pixels.
            ColorGridConfig colorGrid;              //!< Color grid style configuration.
            DialogButtonsConfig dialogButtons;      //!< Dialog buttons style configuration.
        };

        //! Sets the ColorPicker style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ColorPicker.
        static Config loadConfig(QSettings& settings);
        //! Gets the default ColorPicker style configuration.
        static Config defaultConfig();

        //! Constructor for the ColorPicker class.
        //! @param configuration The color Configuration for the ColorPicker.
        //! @param context Context name, used to store settings and window sizing. Leave empty to use the default context.
        //! @param parent Pointer to the parent widget.
        explicit ColorPicker(Configuration configuration, const QString& context = QString(), QWidget* parent = nullptr);
        ~ColorPicker() override;

        //! Returns the previously selected color in the dialog. It is updated to the current color when the OK button is pressed.
        AZ::Color selectedColor() const;
        //! Returns the current color in the dialog. Updates in real time when the user changes settings in the dialog.
        AZ::Color currentColor() const;

        //! Sets a comment string that will be included in the UI as a custom message that may provide some context for the user
        void setComment(QString comment);

        //! Sets up some read-only output for displaying color values in an alternate color space.
        void setAlternateColorspaceEnabled(bool enabled);
        void setAlternateColorspaceName(const QString& name);
        void setAlternateColorspaceValue(const AZ::Color& color);

        //! Populates the palette list with the palettes stored at the folder path provided.
        void importPalettesFromFolder(const QString& path);

        //! Utility function to quickly generate a ColorPicker dialog.
        //! @param configuration The color Configuration for the ColorPicker.
        //! @param initial The color selected by default when the dialog is opened.
        //! @param title The title of the dialog window.
        //! @param context Context name, used to store settings and window sizing. Leave empty to use the default context.
        //! @param palettePaths Paths to folders containing palette files. If not empty, all files in every folder will be loaded in the dialog alongside the default palette.
        //! @param parent Pointer to the parent widget.
        //! @return The color selected by the user.
        static AZ::Color getColor(Configuration configuration, const AZ::Color& initial, const QString& title, const QString& context = QString(), const QStringList& palettePaths = QStringList(), QWidget* parent = nullptr);

    Q_SIGNALS:
        //! Triggered when the selected color is changed.
        //! @param color The new selected color.
        void selectedColorChanged(const AZ::Color& color);
        //! Triggered when the current color is changed.
        //! @param color The new current color.
        void currentColorChanged(const AZ::Color& color);

    public Q_SLOTS:
        // Sets the selected color to the one provided.
        void setSelectedColor(const AZ::Color& color);
        // Sets the current color to the one provided.
        void setCurrentColor(const AZ::Color& color);

    private Q_SLOTS:
        void importPalette();
        void newPalette();
        void removePaletteCardRequested(QSharedPointer<PaletteCard> card);

    protected:
        bool eventFilter(QObject* o, QEvent* e) override;
        void hideEvent(QHideEvent* event) override;
        void done(int result) override;

        void contextMenuEvent(QContextMenuEvent* e) override;

    private:
        struct ColorLibrary
        {
            QString fileName;
            QSharedPointer<Palette> palette;
        };

        class CurrentColorChangedCommand;
        class PaletteAddedCommand;
        class PaletteRemovedCommand;

        friend class Style;
        friend class CurrentColorChangedCommand;
        friend class PaletteAddedCommand;
        friend class PaletteRemovedCommand;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        void warnColorAdjusted(const QString& message);

        void setConfiguration(Configuration configuration);
        void applyConfigurationRGBA(); 
        void applyConfigurationRGB();
        void applyConfigurationHueSaturation();
        void polish(const Config& config);
        void refreshCardMargins();

        void setColorGridMode(ColorGrid::Mode mode);
        void initContextMenu(Configuration configuration);

        void readSettings();
        void writeSettings() const;

        void savePalette(QSharedPointer<PaletteCard> palette, bool queryFileName);
        bool saveColorLibrary(ColorLibrary& colorLibrary, bool queryFileName);

        bool importPaletteFromFile(const QString& fileName, const Internal::ColorLibrarySettings& settings);
        int colorLibraryIndex(const Palette* palette) const;

        void addPalette(QSharedPointer<Palette> palette, const QString& fileName, const QString& title, const Internal::ColorLibrarySettings& settings);
        void addPaletteCard(QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary);
        void removePaletteCard(QSharedPointer<PaletteCard> card);
        bool saveChangedPalettes();

        void beginDynamicColorChange();
        void endDynamicColorChange();

        void initializeValidation(ColorValidator* validator);
        void showPreviewContextMenu(const QPoint& p, const AZ::Color& selectedColor);

        void swatchSizeActionToggled(bool checked, int newSize);
        void setQuickPaletteVisibility(bool show);

        void paletteContextMenuRequested(QSharedPointer<PaletteCard> paletteCard, const QPoint& point);
        void quickPaletteContextMenuRequested(const QPoint& point);

        QWidget* makeSeparator(QWidget* parent);
        QWidget* makePaddedSeparator(QWidget* parent);

        Configuration m_configuration;
        QString m_context;
        Config m_config;
        Internal::ColorController* m_currentColorController = nullptr;
        AZ::Color m_selectedColor;
        
        QSharedPointer<Palette> m_quickPalette;
        QuickPaletteCard* m_quickPaletteCard;

        QHash<QSharedPointer<PaletteCard>, ColorLibrary> m_colorLibraries;

        QUndoStack* m_undoStack = nullptr;

        QScrollArea* m_scrollArea = nullptr;
        QWidget* m_containerWidget = nullptr;
        
        QGridLayout* m_hsvPickerLayout = nullptr;
        QHBoxLayout* m_rgbLayout = nullptr;
        QHBoxLayout* m_quickPaletteLayout = nullptr;

        GradientSlider* m_alphaSlider = nullptr;
        ColorGrid* m_colorGrid = nullptr;
        GradientSlider* m_hueSlider = nullptr;
        GradientSlider* m_valueSlider = nullptr;
        QToolButton* m_eyedropperButton = nullptr;
        QToolButton* m_toggleHueGridButton = nullptr;
        ColorPreview* m_preview = nullptr;
        ColorWarning* m_warning = nullptr;
        ColorRGBAEdit* m_rgbaEdit = nullptr;
        ColorHexEdit* m_hexEdit = nullptr;
        TabWidget* m_slidersTabWidget = nullptr;
        HSLSliders* m_hslSliders = nullptr;
        HSVSliders* m_hsvSliders = nullptr;
        RGBSliders* m_rgbSliders = nullptr;
        QWidget* m_quickPaletteSeparator = nullptr;
        QWidget* m_paletteCardSeparator = nullptr;
        PaletteCardCollection* m_paletteCardCollection = nullptr;
        QMenu* m_menu = nullptr;
        Eyedropper* m_eyedropper = nullptr;
        QAction* m_showRgbValuesAction = nullptr;
        QAction* m_showHexValueAction = nullptr;
        QActionGroup* m_swatchSizeGroup = nullptr;

        bool m_dynamicColorChange = false;
        AZ::Color m_previousColor;
        QAction* m_undoAction = nullptr;
        QAction* m_redoAction = nullptr;

        QAction* m_importPaletteAction = nullptr;
        QAction* m_newPaletteAction = nullptr;
        QAction* m_toggleQuickPaletteAction = nullptr;
        QDialogButtonBox* m_dialogButtonBox = nullptr;
        qreal m_defaultVForHsMode = 0.0f;
        qreal m_defaultLForHsMode = 0.0f;

        QWidget* m_floatEditSeparator = nullptr;

        QWidget* m_commentSeparator = nullptr;
        QLabel* m_commentLabel = nullptr;

        QGridLayout* m_alternateColorSpaceInfoLayout = nullptr;
        QLabel* m_alternateColorSpaceIntLabel = nullptr;
        QLabel* m_alternateColorSpaceFloatLabel = nullptr;
        QLineEdit* m_alternateColorSpaceIntValue = nullptr;
        QLineEdit* m_alternateColorSpaceFloatValue = nullptr;

        QString m_lastSaveDirectory;
        QVector<QWidget*> m_separators;
    };
    AZ_POP_DISABLE_WARNING

} // namespace AzQtComponents

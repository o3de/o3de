/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/ColorPicker.h>

#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorValidator.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorComponentSliders.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorRGBAEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCardCollection.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorWarning.h>
#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QSettings>
#include <QToolButton>
#include <QDirIterator>
#include <QUndoStack>
#include <QTimer>
#include <QCursor>
#include <QLabel>
#include <QTextEdit>
AZ_POP_DISABLE_WARNING

// settings keys
static const char* g_colorPickerSection = "ColorPicker";
static const char* g_showRgbValuesKey = "RgbValues";
static const char* g_showHexValuesKey = "HexValues";
static const char* g_lastSliderTabIndexKey = "LastSliderTabIndex";
static const char* g_quickPaletteKey = "QuickPalette";
static const char* g_colorLibrariesKey = "ColorLibraries";
static const char* g_colorLibraryNameKey = "Name";
static const char* g_colorLibraryExpandedKey = "Expanded";
static const char* g_swatchSizeKey = "SwatchSize";
static const char* g_windowGeometryKey = "WindowGeometry";
static const char* g_paletteFileDialogKey = "PaletteFileDialogPath";
static const char* g_showQuickPaletteKey = "ShowQuickPalette";

namespace AzQtComponents
{

    namespace ColorPickerHelpers
    {
        const QString SEPARATOR_CLASS = QStringLiteral("HorizontalSeparator");
        const QString SEPARATOR_CONTAINER_CLASS = QStringLiteral("HorizontalSeparatorContainer");
        const AZ::Color INVALID_COLOR = AZ::Color{ -1.0f, -1.0f, -1.0f, -1.0f };

        const int SPIN_BOX_MARGIN_FUDGE = 1;

        Internal::ColorLibrarySettings defaultColorLibrarySettings()
        {
            return Internal::ColorLibrarySettings{true};
        }

        void RemoveAllWidgets(QLayout* layout)
        {
            for (int i = 0; i < layout->count(); ++i)
            {
                layout->removeWidget(layout->itemAt(i)->widget());
            }
        }

        const char* ConfigurationName(ColorPicker::Configuration configuration)
        {
            switch (configuration)
            {
            case ColorPicker::Configuration::RGBA:
                return "ConfigurationRGBA";
            case ColorPicker::Configuration::RGB:
                return "ConfigurationRGB";
            case ColorPicker::Configuration::HueSaturation:
                return "ConfigurationHueSaturation";
            }

            Q_UNREACHABLE();
        }

        void ReadColorGridConfig(QSettings& settings, const QString& name, ColorPicker::ColorGridConfig& colorGrid)
        {
            ConfigHelpers::GroupGuard guard(&settings, name);
            ConfigHelpers::read<QSize>(settings, QStringLiteral("MinimumSize"), colorGrid.minimumSize);
        }

        void ReadDialoButtonsConfig(QSettings& settings, const QString& name, ColorPicker::DialogButtonsConfig& dialogButtons)
        {
            ConfigHelpers::GroupGuard guard(&settings, name);
            ConfigHelpers::read<int>(settings, QStringLiteral("TopPadding"), dialogButtons.topPadding);
        }

        QString rgbToolTip(const QString& relevantInfo, const QColor& color)
        {
            return QStringLiteral("%0\nRGB: %1, %2, %3").arg(relevantInfo).arg(color.red()).arg(color.green()).arg(color.blue());
        }
    } // namespace ColorPickerHelpers

    using namespace ColorPickerHelpers; // only for the end of this scope, which is the AzQtComponents namespace
    
    class ColorPicker::CurrentColorChangedCommand : public QUndoCommand
    {
    public:
        CurrentColorChangedCommand(ColorPicker* picker, const AZ::Color& previousColor, const AZ::Color& newColor, QUndoCommand* parent = nullptr);
        ~CurrentColorChangedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        const AZ::Color m_previousColor;
        const AZ::Color m_newColor;
    };

    ColorPicker::CurrentColorChangedCommand::CurrentColorChangedCommand(ColorPicker* picker, const AZ::Color& previousColor, const AZ::Color& newColor, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_previousColor(previousColor)
        , m_newColor(newColor)
    {
    }

    ColorPicker::CurrentColorChangedCommand::~CurrentColorChangedCommand()
    {
    }

    void ColorPicker::CurrentColorChangedCommand::undo()
    {
        m_picker->m_previousColor = m_previousColor;
        m_picker->m_currentColorController->setColor(m_previousColor);
    }

    void ColorPicker::CurrentColorChangedCommand::redo()
    {
        m_picker->m_previousColor = m_newColor;
        m_picker->m_currentColorController->setColor(m_newColor);
    }

    class ColorPicker::PaletteAddedCommand : public QUndoCommand
    {
    public:
        PaletteAddedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent = nullptr);
        ~PaletteAddedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        QSharedPointer<PaletteCard> m_card;
        ColorLibrary m_colorLibrary;
    };

    ColorPicker::PaletteAddedCommand::PaletteAddedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_card(card)
        , m_colorLibrary(colorLibrary)
    {
        QObject::connect(card.data(), &PaletteCard::contextMenuRequested, picker, [picker, card](const QPoint& point)
        {
            picker->paletteContextMenuRequested(card, point);
        });
    }

    ColorPicker::PaletteAddedCommand::~PaletteAddedCommand()
    {
    }

    void ColorPicker::PaletteAddedCommand::undo()
    {
        m_picker->removePaletteCard(m_card);
    }

    void ColorPicker::PaletteAddedCommand::redo()
    {
        m_picker->addPaletteCard(m_card, m_colorLibrary);
    }

    class ColorPicker::PaletteRemovedCommand : public QUndoCommand
    {
    public:
        PaletteRemovedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent = nullptr);
        ~PaletteRemovedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        QSharedPointer<PaletteCard> m_card;
        ColorLibrary m_colorLibrary;
    };

    ColorPicker::PaletteRemovedCommand::PaletteRemovedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_card(card)
        , m_colorLibrary(colorLibrary)
    {
    }

    ColorPicker::PaletteRemovedCommand::~PaletteRemovedCommand()
    {
    }

    void ColorPicker::PaletteRemovedCommand::undo()
    {
        m_picker->addPaletteCard(m_card, m_colorLibrary);
    }

    void ColorPicker::PaletteRemovedCommand::redo()
    {
        m_picker->removePaletteCard(m_card);
    }

ColorPicker::Config ColorPicker::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();
    ConfigHelpers::read<int>(settings, QStringLiteral("Padding"), config.padding);
    ConfigHelpers::read<int>(settings, QStringLiteral("Spacing"), config.spacing);
    ReadColorGridConfig(settings, "ColorGrid", config.colorGrid);
    ReadDialoButtonsConfig(settings, "DialogButtons", config.dialogButtons);
    return config;
}

ColorPicker::Config ColorPicker::defaultConfig()
{
    return ColorPicker::Config{
        16,  // Padding
        8,   // Spacing;
        {    // ColorGrid
            {194, 150} // ColorGrid/MinimumSize
        },
        {    // DialogButtons
            12 // DialogButtons/TopPadding
        }
    };
}

bool ColorPicker::polish(Style* style, QWidget* widget, const Config& config)
{
    auto colorPicker = qobject_cast<ColorPicker*>(widget);
    if (!colorPicker)
    {
        return false;
    }

    colorPicker->polish(config);
    style->repolishOnSettingsChange(colorPicker);
    return true;
}

void ColorPicker::polish(const ColorPicker::Config& config)
{
    m_config = config;

    // Outer layout
    layout()->setSpacing(0);
    layout()->setContentsMargins({ 0, m_config.padding, 0, m_config.padding });

    // Container layout
    m_containerWidget->layout()->setSpacing(m_config.spacing);

    // The scroll bar shows and hides on hover, but the space is taken up by it always
    // so we have to account for that by adjusting the right padding by that much.
    m_containerWidget->layout()->setContentsMargins({ m_config.padding, 0, m_config.padding - m_scrollArea->verticalScrollBar()->sizeHint().width(), m_config.padding });

    // Color grid, preview, eyedropper
    m_colorGrid->setMinimumSize(m_config.colorGrid.minimumSize);

    m_hsvPickerLayout->setContentsMargins({ 0, 0, 0, 0 });
    m_hsvPickerLayout->setSpacing(m_config.spacing);

    m_rgbaEdit->setHorizontalSpacing(m_config.spacing - (SPIN_BOX_MARGIN_FUDGE * 2));
    m_rgbLayout->setContentsMargins({ 0, m_config.spacing, 0, 0 });
    m_rgbLayout->setSpacing(m_config.spacing - (SPIN_BOX_MARGIN_FUDGE * 2));

    m_quickPaletteLayout->setContentsMargins(QMargins{ 0, 0, 0, 0 });

    m_paletteCardCollection->setContentsMargins(QMargins{ 0, 0, 0, 0 });

    // Special case as the artboards show a 12px padding above the buttons
    m_dialogButtonBox->setContentsMargins(QMargins{ m_config.padding, m_config.dialogButtons.topPadding, m_config.padding, 0 });

    m_hslSliders->layout()->setContentsMargins(QMargins{ 0, 0, 0, 0 });
    m_hslSliders->layout()->setSpacing(m_config.spacing);
    m_hsvSliders->layout()->setContentsMargins(QMargins{ 0, 0, 0, 0 });
    m_hsvSliders->layout()->setSpacing(m_config.spacing);
    m_rgbSliders->layout()->setContentsMargins(QMargins{ 0, 0, 0, 0 });
    m_rgbSliders->layout()->setSpacing(m_config.spacing);

    for (QWidget* separator : m_separators)
    {
        separator->layout()->setContentsMargins(QMargins{ 0, 0, 0, 0 });
    }

    m_paletteCardCollection->setCardContentMargins(QMargins{ 0, 0, 0, 0 });
    m_quickPaletteCard->setContentsMargins(QMargins{ 0, 0, 0, 0 });
}

ColorPicker::ColorPicker(ColorPicker::Configuration configuration, const QString& context, QWidget* parent)
    : LogicalTabOrderingWidget<StyledDialog>(parent)
    , m_configuration(configuration)
    , m_context(context.isEmpty() ? g_colorPickerSection : context)
    , m_undoStack(new QUndoStack(this))
    , m_previousColor(INVALID_COLOR)
    , m_defaultVForHsMode(0.85)
    , m_defaultLForHsMode(0.85)
{
    qRegisterMetaTypeStreamOperators<Palette>("AzQtComponents::Palette");

    setFocusPolicy(Qt::NoFocus);

    m_currentColorController = new Internal::ColorController(this);
    connect(m_currentColorController, &Internal::ColorController::colorChanged, this, &ColorPicker::currentColorChanged);
    connect(m_currentColorController, &Internal::ColorController::colorChanged, this, [this]() {
        if (!m_dynamicColorChange)
        {
            endDynamicColorChange();
        }
    });

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(m_scrollArea);

    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_containerWidget = new QWidget(this);
    m_containerWidget->setObjectName(QStringLiteral("Container"));
    m_containerWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_containerWidget->installEventFilter(this);
    m_scrollArea->setWidget(m_containerWidget);

    auto containerLayout = new QVBoxLayout(m_containerWidget);
    containerLayout->setSizeConstraint(QLayout::SetFixedSize);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // warning widget

    m_warning = new ColorWarning(ColorWarning::Mode::Warning, {}, tr("Selected color is the closest available"), this);
    m_warning->hide();

    // alpha slider + color grid + lightness slider + color preview

    m_hsvPickerLayout = new QGridLayout();
    containerLayout->addLayout(m_hsvPickerLayout);

    // alpha slider

    m_alphaSlider = new GradientSlider(Qt::Vertical, this);
    m_alphaSlider->SetIgnoreWheelEvents(true);
    Style::addClass(m_alphaSlider, "AlphaGradient");
    m_alphaSlider->setColorFunction([this](qreal position) {
        auto color = ToQColor(m_currentColorController->color());
        color.setAlphaF(position);
        return color;
    });
    m_alphaSlider->setToolTipFunction([this](qreal position) {
        return QStringLiteral("Alpha: %0").arg(toString(position, m_alphaSlider->decimals(), m_alphaSlider->locale()));
    });
    m_alphaSlider->setMinimum(0);
    m_alphaSlider->setMaximum(255);

    // Set the default value on the slider.
    m_alphaSlider->setValue(qRound(m_currentColorController->alpha() * 255.0f));

    connect(m_alphaSlider, &QSlider::valueChanged, this, [this](int alpha) {
        m_currentColorController->setAlpha(aznumeric_cast<float>(alpha)/255.0f);
    });
    connect(m_currentColorController, &Internal::ColorController::colorChanged, m_alphaSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::alphaChanged, this, [this](qreal alpha) {
        const QSignalBlocker b(m_alphaSlider);
        m_alphaSlider->setValue(qRound(alpha*255.0));
    });

    // color grid

    m_colorGrid = new ColorGrid(this);
    m_colorGrid->setDefaultVForHsMode(m_defaultVForHsMode);

    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_colorGrid, &ColorGrid::setHue);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_colorGrid, &ColorGrid::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, m_colorGrid, &ColorGrid::setValue);
    connect(m_colorGrid, &ColorGrid::hsvChanged, m_currentColorController, &Internal::ColorController::setHSV);

    connect(m_colorGrid, &ColorGrid::gridPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_colorGrid, &ColorGrid::gridReleased, this, &ColorPicker::endDynamicColorChange);

    // hue slider

    m_hueSlider = new GradientSlider(Qt::Vertical, this);
    m_hueSlider->SetIgnoreWheelEvents(true);
    m_hueSlider->setColorFunction([](qreal position) {
        return toQColor(Internal::ColorController::fromHsv(position, 1.0, 1.0));
    });
    m_hueSlider->setToolTipFunction([this](qreal position) {
        QString prefix = QStringLiteral("Hue: %0").arg(qRound(position * m_hueSlider->maximum()));
        return rgbToolTip(prefix, toQColor(Internal::ColorController::fromHsv(position, m_currentColorController->hsvSaturation(), m_currentColorController->value())));
    });
    m_hueSlider->setMinimum(0);
    m_hueSlider->setMaximum(360);

    connect(m_hueSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentColorController->setHsvHue(aznumeric_cast<float>(value)/360.0f);
    });
    connect(m_hueSlider, &GradientSlider::sliderPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hueSlider, &GradientSlider::sliderReleased, this, &ColorPicker::endDynamicColorChange);
    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, this, [this](qreal hue) {
        const QSignalBlocker b(m_hueSlider);
        m_hueSlider->setValue(qRound(hue*360.0));
    });

    // value slider

    m_valueSlider = new GradientSlider(Qt::Vertical, this);
    m_valueSlider->SetIgnoreWheelEvents(true);
    m_valueSlider->setColorFunction([this](qreal position) {
        return toQColor(Internal::ColorController::fromHsv(m_currentColorController->hsvHue(), m_currentColorController->hsvSaturation(), position));
    });
    m_valueSlider->setToolTipFunction([this](qreal position) {
        QString prefix = QStringLiteral("V: %0").arg(qRound(position * m_valueSlider->maximum()));
        return rgbToolTip(prefix, toQColor(Internal::ColorController::fromHsv(m_currentColorController->hsvHue(), m_currentColorController->hsvSaturation(), position)));
    });
    m_valueSlider->setMinimum(0);
    m_valueSlider->setMaximum(255);

    connect(m_valueSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentColorController->setValue(aznumeric_cast<float>(value)/255.0f);
    });
    connect(m_valueSlider, &GradientSlider::sliderPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_valueSlider, &GradientSlider::sliderReleased, this, &ColorPicker::endDynamicColorChange);
    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_valueSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_valueSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, this, [this](qreal value) {
        const QSignalBlocker b(m_valueSlider);
        m_valueSlider->setValue(qRound(value*255.0));
    });

    // eyedropper button

    m_eyedropperButton = new QToolButton(this);
    m_eyedropperButton->setFocusPolicy(Qt::NoFocus);
    QIcon eyedropperIcon(QStringLiteral(":/ColorPickerDialog/ColorGrid/eyedropper-normal.svg"));
    m_eyedropperButton->setIcon(eyedropperIcon);
    m_eyedropperButton->setAutoRaise(true);
    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_eyedropperButton->setToolTip(tr("Left click on this and hold the button down. On left mouse button release, the color under the mouse cursor will be picked."));
    m_eyedropper = new Eyedropper(this, m_eyedropperButton);
    connect(m_eyedropper, &Eyedropper::colorSelected, this, [this](const QColor& color) { setCurrentColor(FromQColor(color)); });
    connect(m_eyedropperButton, &QToolButton::pressed, m_eyedropper, &Eyedropper::show);

    // preview

    m_preview = new ColorPreview(this);
    connect(this, &ColorPicker::currentColorChanged, m_preview, &ColorPreview::setCurrentColor);
    connect(this, &ColorPicker::selectedColorChanged, m_preview, &ColorPreview::setSelectedColor);

    connect(m_preview, &ColorPreview::colorContextMenuRequested, this, &ColorPicker::showPreviewContextMenu);

    // toggle hue/saturation and saturation/value color grid button

    m_toggleHueGridButton = new QToolButton(this);
    m_toggleHueGridButton->setFocusPolicy(Qt::NoFocus);
    QIcon toggleHueGridIcon(QStringLiteral(":/ColorPickerDialog/ColorGrid/toggle-normal-on.svg"));
    m_toggleHueGridButton->setIcon(toggleHueGridIcon);
    m_toggleHueGridButton->setAutoRaise(true);
    m_toggleHueGridButton->setCheckable(true);
    m_toggleHueGridButton->setChecked(true);
    m_toggleHueGridButton->setToolTip("Click this to toggle the color grid between Saturation/Value mode and Hue/Saturation mode");
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);

    connect(m_toggleHueGridButton, &QAbstractButton::toggled, this, [this](bool checked) {
        setColorGridMode(checked ? ColorGrid::Mode::SaturationValue : ColorGrid::Mode::HueSaturation);
    });

    // RGBA edit

    m_rgbLayout = new QHBoxLayout();

    m_rgbLayout->addStretch();

    m_rgbaEdit = new ColorRGBAEdit(this);
    m_rgbLayout->addWidget(m_rgbaEdit);

    m_rgbLayout->addStretch();

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_rgbaEdit, &ColorRGBAEdit::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_rgbaEdit, &ColorRGBAEdit::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_rgbaEdit, &ColorRGBAEdit::setBlue);
    connect(m_currentColorController, &Internal::ColorController::alphaChanged, m_rgbaEdit, &ColorRGBAEdit::setAlpha);
    connect(m_rgbaEdit, &ColorRGBAEdit::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_rgbaEdit, &ColorRGBAEdit::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_rgbaEdit, &ColorRGBAEdit::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_rgbaEdit, &ColorRGBAEdit::alphaChanged, m_currentColorController, &Internal::ColorController::setAlpha);
    connect(m_rgbaEdit, &ColorRGBAEdit::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_rgbaEdit, &ColorRGBAEdit::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // hex
    m_hexEdit = new ColorHexEdit(this);

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_hexEdit, &ColorHexEdit::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_hexEdit, &ColorHexEdit::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_hexEdit, &ColorHexEdit::setBlue);
    connect(m_currentColorController, &Internal::ColorController::alphaChanged, m_hexEdit, &ColorHexEdit::setAlpha);
    connect(m_hexEdit, &ColorHexEdit::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_hexEdit, &ColorHexEdit::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_hexEdit, &ColorHexEdit::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_hexEdit, &ColorHexEdit::alphaChanged, m_currentColorController, &Internal::ColorController::setAlpha);
    connect(m_hexEdit, &ColorHexEdit::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hexEdit, &ColorHexEdit::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // HSL sliders

    m_hslSliders = new HSLSliders();
    m_hslSliders->setDefaultLForHsMode(m_defaultLForHsMode);
    m_hslSliders->hide();

    connect(m_currentColorController, &Internal::ColorController::hslHueChanged, m_hslSliders, &HSLSliders::setHue);
    connect(m_currentColorController, &Internal::ColorController::hslSaturationChanged, m_hslSliders, &HSLSliders::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::lightnessChanged, m_hslSliders, &HSLSliders::setLightness);
    connect(m_hslSliders, &HSLSliders::hueChanged, m_currentColorController, &Internal::ColorController::setHslHue);
    connect(m_hslSliders, &HSLSliders::saturationChanged, m_currentColorController, &Internal::ColorController::setHslSaturation);
    connect(m_hslSliders, &HSLSliders::lightnessChanged, m_currentColorController, &Internal::ColorController::setLightness);
    connect(m_hslSliders, &HSLSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hslSliders, &HSLSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // HSV sliders

    m_hsvSliders = new HSVSliders();
    m_hsvSliders->setDefaultVForHsMode(this->m_defaultVForHsMode);
    m_hsvSliders->hide();

    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_hsvSliders, &HSVSliders::setHue);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_hsvSliders, &HSVSliders::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, m_hsvSliders, &HSVSliders::setValue);
    connect(m_hsvSliders, &HSVSliders::hueChanged, m_currentColorController, &Internal::ColorController::setHsvHue);
    connect(m_hsvSliders, &HSVSliders::saturationChanged, m_currentColorController, &Internal::ColorController::setHsvSaturation);
    connect(m_hsvSliders, &HSVSliders::valueChanged, m_currentColorController, &Internal::ColorController::setValue);
    connect(m_hsvSliders, &HSVSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hsvSliders, &HSVSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // RGB sliders

    m_rgbSliders = new RGBSliders();
    m_rgbSliders->hide();

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_rgbSliders, &RGBSliders::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_rgbSliders, &RGBSliders::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_rgbSliders, &RGBSliders::setBlue);
    connect(m_rgbSliders, &RGBSliders::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_rgbSliders, &RGBSliders::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_rgbSliders, &RGBSliders::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_rgbSliders, &RGBSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_rgbSliders, &RGBSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // HSL/HSV/RGB slider tab group
    auto tabWidgetSeparator = makePaddedSeparator(this);
    containerLayout->addWidget(tabWidgetSeparator);

    m_slidersTabWidget = new TabWidget(this);
    m_slidersTabWidget->addTab(m_rgbSliders, QObject::tr("RGB"));
    m_slidersTabWidget->addTab(m_hslSliders, QObject::tr("HSL"));
    m_slidersTabWidget->addTab(m_hsvSliders, QObject::tr("HSV"));

    TabWidget::applySecondaryStyle(m_slidersTabWidget, false);
    containerLayout->addWidget(m_slidersTabWidget);

    // Place the hex edit beneath the color slider tab group
    containerLayout->addWidget(m_hexEdit);

    // Place the RGB float input fields

    m_floatEditSeparator = makePaddedSeparator(this);
    containerLayout->addWidget(m_floatEditSeparator);

    containerLayout->addLayout(m_rgbLayout);

    // quick palette

    m_quickPaletteSeparator = makePaddedSeparator(this);
    containerLayout->addWidget(m_quickPaletteSeparator);

    m_quickPalette = QSharedPointer<Palette>::create();
    m_quickPaletteCard = new QuickPaletteCard(m_quickPalette, m_currentColorController, m_undoStack, this);

    connect(m_quickPaletteCard, &QuickPaletteCard::selectedColorsChanged, this, [this](const QVector<AZ::Color>& selectedColors) {
        if (selectedColors.size() == 1)
        {
            m_currentColorController->setColor(selectedColors[0]);
        }
    });

    connect(m_quickPaletteCard, &QuickPaletteCard::contextMenuRequested, this, &ColorPicker::quickPaletteContextMenuRequested);

    connect(m_preview, &ColorPreview::colorSelected, m_quickPaletteCard, [this](const AZ::Color& color) {
        if (!m_quickPaletteCard->contains(color))
        {
            m_quickPaletteCard->tryAdd(color);
        }
    });

    m_quickPaletteLayout = new QHBoxLayout();
    containerLayout->addLayout(m_quickPaletteLayout);
    m_quickPaletteLayout->addWidget(m_quickPaletteCard);

    // color libraries

    m_paletteCardSeparator = makePaddedSeparator(this);
    m_paletteCardSeparator->hide();
    containerLayout->addWidget(m_paletteCardSeparator);

    m_paletteCardCollection = new PaletteCardCollection(m_currentColorController, m_undoStack, this);
    m_paletteCardCollection->hide();
    connect(m_paletteCardCollection, &PaletteCardCollection::removePaletteClicked, this, &ColorPicker::removePaletteCardRequested);
    connect(m_paletteCardCollection, &PaletteCardCollection::savePaletteClicked, this, [this](QSharedPointer<PaletteCard> palette) {
        savePalette(palette, false);
    });
    connect(m_paletteCardCollection, &PaletteCardCollection::savePaletteAsClicked, this, [this](QSharedPointer<PaletteCard> palette) {
        savePalette(palette, true);
    });
    connect(m_paletteCardCollection, &PaletteCardCollection::paletteCountChanged, this, [this] {
        const bool visible = !m_paletteCardCollection->isEmpty();
        m_paletteCardSeparator->setVisible(visible);
        m_paletteCardCollection->setVisible(visible);
    });
    containerLayout->addWidget(m_paletteCardCollection);

    // Final color space comment
    m_commentSeparator = makePaddedSeparator(this);
    containerLayout->addWidget(m_commentSeparator);
    m_commentLabel = new QLabel(QObject::tr("Color space: sRGB"), this);
    containerLayout->addWidget(m_commentLabel);

    // Alternate color space info

    // These widgets will be updated as needed in setAlternateColorspace* functions
    m_alternateColorSpaceIntLabel = new QLabel(QObject::tr("Alternate Int"), this);
    m_alternateColorSpaceFloatLabel = new QLabel(QObject::tr("Alternate Float"), this);
    m_alternateColorSpaceIntValue = new QLineEdit(QObject::tr("Unspecified"), this);
    m_alternateColorSpaceFloatValue = new QLineEdit(QObject::tr("Unspecified"), this);
    m_alternateColorSpaceIntValue->setDisabled(true);
    m_alternateColorSpaceFloatValue->setDisabled(true);

    m_alternateColorSpaceInfoLayout = new QGridLayout();
    m_alternateColorSpaceInfoLayout->addWidget(m_alternateColorSpaceIntLabel, 0, 0);
    m_alternateColorSpaceInfoLayout->addWidget(m_alternateColorSpaceFloatLabel, 1, 0);
    m_alternateColorSpaceInfoLayout->addWidget(m_alternateColorSpaceIntValue, 0, 1);
    m_alternateColorSpaceInfoLayout->addWidget(m_alternateColorSpaceFloatValue, 1, 1);
    containerLayout->addLayout(m_alternateColorSpaceInfoLayout);

    setAlternateColorspaceEnabled(false);

    // buttons

    mainLayout->addWidget(makeSeparator(this));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), this);
    m_dialogButtonBox->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(m_dialogButtonBox);

    connect(m_dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    initContextMenu(configuration);

    // Add a settings menu button on the slider tab widget
    QAction* settingsMenuAction = new QAction(QIcon(":/stylesheet/img/UI20/menu-centered.svg"), QObject::tr("Settings"), this);
    settingsMenuAction->setMenu(m_menu);
    m_slidersTabWidget->setActionToolBarVisible();
    m_slidersTabWidget->addAction(settingsMenuAction);

    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_undoAction, &QAction::triggered, m_undoStack, &QUndoStack::undo);
    addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_redoAction, &QAction::triggered, m_undoStack, &QUndoStack::redo);
    addAction(m_redoAction);

    setConfiguration(configuration);

    enableSaveRestoreGeometry(QStringLiteral("%1/%2/%3").arg(m_context).arg(ConfigurationName(m_configuration)).arg(g_windowGeometryKey));
    
    // restore the settings after a delay. There's some weirdness going on with restoring the
    // geometry too early
    QTimer::singleShot(0, this, &StyledDialog::restoreGeometryFromSettings);

    readSettings();
}

void ColorPicker::setComment(QString comment)
{
    m_commentLabel->setText(comment);
}

void ColorPicker::setAlternateColorspaceEnabled(bool enabled)
{
    m_alternateColorSpaceIntLabel->setVisible(enabled);
    m_alternateColorSpaceFloatLabel->setVisible(enabled);
    m_alternateColorSpaceIntValue->setVisible(enabled);
    m_alternateColorSpaceFloatValue->setVisible(enabled);
}

void ColorPicker::setAlternateColorspaceName(const QString& name)
{
    m_alternateColorSpaceIntLabel->setText(name + " Int");
    m_alternateColorSpaceFloatLabel->setText(name + " Float");
}

void ColorPicker::setAlternateColorspaceValue(const AZ::Color& color)
{
    QColor qColor = AzQtComponents::toQColor(color);

    bool alphaChannelIncluded = m_configuration == Configuration::RGBA;
    m_alternateColorSpaceIntValue->setText(AzQtComponents::MakePropertyDisplayStringInts(qColor, alphaChannelIncluded));
    m_alternateColorSpaceFloatValue->setText(AzQtComponents::MakePropertyDisplayStringFloats(qColor, alphaChannelIncluded));
}

void ColorPicker::warnColorAdjusted(const QString& message)
{
    m_warning->setToolTip(message);
    m_warning->show();
}

void ColorPicker::setConfiguration(Configuration configuration)
{
    m_configuration = configuration;

    switch (configuration)
    {
    case Configuration::RGBA:
        applyConfigurationRGBA();
        break;
    case Configuration::RGB:
        applyConfigurationRGB();
        break;
    case Configuration::HueSaturation:
        applyConfigurationHueSaturation();
        break;
    }
}

void ColorPicker::applyConfigurationRGBA()
{
    m_alphaSlider->show();
    m_toggleHueGridButton->show();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgba);

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_alphaSlider, 0, 0);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 1);
    m_hsvPickerLayout->addWidget(m_hueSlider, 0, 2);
    m_hsvPickerLayout->addWidget(m_valueSlider, 0, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 3);

    setColorGridMode(ColorGrid::Mode::SaturationValue);

    m_hslSliders->setMode(HSLSliders::Mode::Hsl);
}

void ColorPicker::applyConfigurationRGB()
{
    m_alphaSlider->hide();
    m_toggleHueGridButton->show();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgb);

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 0, 1, 2);
    m_hsvPickerLayout->addWidget(m_hueSlider, 0, 2);
    m_hsvPickerLayout->addWidget(m_valueSlider, 0, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 3);

    setColorGridMode(ColorGrid::Mode::SaturationValue);

    initializeValidation(new RGBColorValidator(this));
}

void ColorPicker::applyConfigurationHueSaturation()
{
    m_alphaSlider->hide();
    m_hueSlider->hide();
    m_valueSlider->hide();
    m_toggleHueGridButton->hide();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgb);
    m_rgbaEdit->setReadOnly(true);

    m_rgbaEdit->hide();
    m_hexEdit->hide();

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 0, 1, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 2);

    m_colorGrid->setMode(ColorGrid::Mode::HueSaturation);

    m_hslSliders->setMode(HSLSliders::Mode::Hs);

    initializeValidation(new HueSaturationValidator(aznumeric_cast<float>(m_defaultVForHsMode), this));
}

ColorPicker::~ColorPicker()
{
}

AZ::Color ColorPicker::currentColor() const
{
    return m_currentColorController->color();
}

void ColorPicker::setCurrentColor(const AZ::Color& color)
{
    if (m_previousColor.IsClose(INVALID_COLOR))
    {
        m_previousColor = color;
    }
    m_currentColorController->setColor(color);
    m_currentColorController->setAlpha(color.GetA());
}

AZ::Color ColorPicker::selectedColor() const
{
    return m_selectedColor;
}

void ColorPicker::setSelectedColor(const AZ::Color& color)
{
    if (AreClose(color, m_selectedColor))
    {
        return;
    }
    m_selectedColor = color;
    emit selectedColorChanged(m_selectedColor);
}

AZ::Color ColorPicker::getColor(ColorPicker::Configuration configuration, const AZ::Color& initial, const QString& title, const QString& context, const QStringList& palettePaths, QWidget* parent)
{
    ColorPicker dialog(configuration, context, parent);
    dialog.setWindowTitle(title);
    dialog.setCurrentColor(initial);
    dialog.setSelectedColor(initial);
    for (const QString& path : palettePaths)
    {
        dialog.importPalettesFromFolder(path);
    }

    if (dialog.exec() == QDialog::Accepted)
    {
        return dialog.currentColor();
    }

    return initial;
}

void ColorPicker::done(int result)
{
    const auto changedCards = std::any_of(m_colorLibraries.keyBegin(), m_colorLibraries.keyEnd(),
        [](const QSharedPointer<PaletteCard>& card) { return card->modified(); });
    if (changedCards)
    {
        QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        QMessageBox::StandardButton defaultButton = QMessageBox::Yes;
        int messageBoxResult = QMessageBox::question(this, tr("Color Picker"), tr("There are palettes with unsaved changes. Would you like to save them now?"), buttons, defaultButton);
        switch (messageBoxResult)
        {
            case QMessageBox::Yes:
            {
                bool userSaveCancelledOrError = !saveChangedPalettes();
                if (userSaveCancelledOrError)
                {
                    return;
                }
                break;
            }

            case QMessageBox::Cancel:
                return;

            case QMessageBox::No:
                // do nothing but continue closing
                break;
        }
    }

    writeSettings();
    QDialog::done(result);
}

void ColorPicker::contextMenuEvent(QContextMenuEvent* e)
{
    static bool recursionGuardValue = false;

    QPoint globalPosition = e->globalPos();

    // When triggered from the keyboard on Windows, with the context menu key, the position in the event
    // can't be trusted.
    // Also, it defaults to sending the event to the color picker container, instead of to the widget
    // under the cursor.
    if ((e->reason() == QContextMenuEvent::Keyboard) && !recursionGuardValue)
    {
        // This can be called recursively, so we have to watch out
        QScopedValueRollback<bool> recursionGuard(recursionGuardValue, true);

        // Change the position, even if we're going to fall through to using the color picker's context menu
        globalPosition = QCursor::pos();

        // Manually look for the widget under the cursor and attempt to send it the event
        QWidget* widgetUnderCursor = QApplication::widgetAt(globalPosition);
        if (widgetUnderCursor && (widgetUnderCursor != this))
        {
            QContextMenuEvent eventCopy(e->reason(), widgetUnderCursor->mapFromGlobal(globalPosition), globalPosition, e->modifiers());
            if (QApplication::sendEvent(widgetUnderCursor, &eventCopy))
            {
                e->accept();
                return;
            }
        }

        // Intentionally fall-through if there wasn't a different widget under the cursor
    }

    m_menu->exec(globalPosition);

    e->accept();
}

bool ColorPicker::saveChangedPalettes()
{
    for (auto it = m_colorLibraries.begin(); it != m_colorLibraries.end(); ++it)
    {
        auto& card = it.key();
        if (card->modified())
        {
            auto& colorLibrary = it.value();
            if (!saveColorLibrary(colorLibrary, false))
            {
                return false;
            }
        }
    }

    return true;
}

void ColorPicker::setColorGridMode(ColorGrid::Mode mode)
{
    m_colorGrid->setMode(mode);

    if (m_hueSlider->layout() && !m_valueSlider->layout())
    {
        // The HueSaturation configuration has neither of these along the color grid so we shouldn't change their visibility
        return;
    }

    if (mode == ColorGrid::Mode::SaturationValue)
    {
        m_hueSlider->show();
        m_valueSlider->hide();
    }
    else
    {
        m_hueSlider->hide();
        m_valueSlider->show();
    }
}

void ColorPicker::initContextMenu(ColorPicker::Configuration configuration)
{
    m_menu = new QMenu(this);

    if (configuration != Configuration::HueSaturation)
    {
        m_showRgbValuesAction = m_menu->addAction(tr("Show sRGB Float"));
        m_showRgbValuesAction->setCheckable(true);
        m_showRgbValuesAction->setChecked(true);
        QObject::connect(m_showRgbValuesAction, &QAction::toggled, [this](bool checked) {
            m_rgbaEdit->setVisible(checked);
            m_commentSeparator->setVisible(checked);
            m_commentLabel->setVisible(checked);
        });

        m_showHexValueAction = m_menu->addAction(tr("Show hex value"));
        m_showHexValueAction->setCheckable(true);
        m_showHexValueAction->setChecked(true);
        connect(m_showHexValueAction, &QAction::toggled, m_hexEdit, &QWidget::setVisible);

        m_menu->addSeparator();
    }

    m_swatchSizeGroup = new QActionGroup(this);

    auto smallSwatches = m_menu->addAction(tr("Small swatches"));
    smallSwatches->setCheckable(true);
    connect(smallSwatches, &QAction::toggled, this, [this](bool checked) {
        const int swatchSizeSmall = 16;
        swatchSizeActionToggled(checked, swatchSizeSmall);
    });
    m_swatchSizeGroup->addAction(smallSwatches);

    auto mediumSwatches = m_menu->addAction(tr("Medium swatches"));
    mediumSwatches->setCheckable(true);
    connect(mediumSwatches, &QAction::toggled, this, [this](bool checked) {
        const int swatchSizeMedium = 24;
        swatchSizeActionToggled(checked, swatchSizeMedium);
    });
    m_swatchSizeGroup->addAction(mediumSwatches);

    auto largeSwatches = m_menu->addAction(tr("Large swatches"));
    largeSwatches->setCheckable(true);
    connect(largeSwatches, &QAction::toggled, this, [this](bool checked) {
        const int swatchSizeLarge = 32;
        swatchSizeActionToggled(checked, swatchSizeLarge);
    });
    m_swatchSizeGroup->addAction(largeSwatches);

    smallSwatches->setChecked(true);

    m_menu->addSeparator();

    m_toggleQuickPaletteAction = m_menu->addAction(tr("Hide Quick Palette"));
    connect(m_toggleQuickPaletteAction, &QAction::triggered, this, [this]() {
        bool wasVisible = m_quickPaletteCard->isVisible();
        setQuickPaletteVisibility(!wasVisible);
    });

    m_menu->addSeparator();

    m_importPaletteAction = m_menu->addAction(tr("Import color palette..."), this, &ColorPicker::importPalette);
    m_newPaletteAction = m_menu->addAction(tr("New color palette"), this, &ColorPicker::newPalette);
}

void ColorPicker::importPalettesFromFolder(const QString& path)
{
    if (path.isEmpty())
    {
        return;
    }

    QDirIterator it(path, QStringList() << "*.pal", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        importPaletteFromFile(it.next(), defaultColorLibrarySettings());
    }
}

bool ColorPicker::importPaletteFromFile(const QString& fileName, const Internal::ColorLibrarySettings& settings)
{
    if (fileName.isEmpty())
    {
        return false;
    }

    auto palette = QSharedPointer<Palette>::create();
    if (!palette->load(fileName))
    {
        return false;
    }

    addPalette(palette, fileName, QFileInfo(fileName).baseName(), settings);
    return true;
}

void ColorPicker::importPalette()
{
    QString lastDirectory = m_lastSaveDirectory;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Color Palette"), lastDirectory, tr("Color Palettes (*.pal)"));
    if (!fileName.isEmpty() && !importPaletteFromFile(fileName, defaultColorLibrarySettings()))
    {
        QMessageBox::critical(this, tr("Color Palette Import Error"), tr("Failed to import \"%1\"").arg(fileName));
    }

    if (!fileName.isEmpty())
    {
        m_lastSaveDirectory = fileName;
    }

    refreshCardMargins();
}

void ColorPicker::newPalette()
{
    addPalette(QSharedPointer<Palette>::create(), {}, tr("Untitled"), defaultColorLibrarySettings());
    refreshCardMargins();
}

void ColorPicker::refreshCardMargins()
{
    // refresh margins after a small delay
    QTimer::singleShot(0, this, [this] {
        m_paletteCardCollection->setCardContentMargins(QMargins(0, 0, 0, 0));
    });
}

void ColorPicker::removePaletteCardRequested(QSharedPointer<PaletteCard> card)
{
    if (card->modified())
    {
        int result = QMessageBox::question(this, tr("Color Picker"), tr("There are unsaved changes to your palette. Are you sure you want to close?"));
        if (result != QMessageBox::Yes)
        {
            return;
        }
    }

    PaletteRemovedCommand* removed = new PaletteRemovedCommand(this, card, m_colorLibraries[card]);
    m_undoStack->push(removed);

    markToRecalculateTabKeyOrdering();
}

void ColorPicker::addPalette(QSharedPointer<Palette> palette, const QString& fileName, const QString& title, const Internal::ColorLibrarySettings& settings)
{
    QSharedPointer<PaletteCard> card = m_paletteCardCollection->makeCard(palette, title);
    card->setExpanded(settings.expanded);

    PaletteAddedCommand* added = new PaletteAddedCommand(this, card, ColorLibrary{ fileName, palette });
    m_undoStack->push(added);

    markToRecalculateTabKeyOrdering();
}

void ColorPicker::addPaletteCard(QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary)
{        
    Palette loader;
    QString fileName = colorLibrary.fileName;
    bool loaded = loader.load(fileName);

    // If we can't load the palette file, then mark the palette as modified
    // If we loaded the data and it was different to what we know OR
    // Either way, we only mark it as modified if it's non-empty
    if (!loaded)
    {

        card->setModified(!card->palette()->colors().empty());
    }
    else
    {
        QVector<AZ::Color> loadedColors = loader.colors();
        QVector<AZ::Color> paletteColors = card->palette()->colors();
        card->setModified(!paletteColors.empty() && !AZStd::ranges::equal(loadedColors, paletteColors));
    }

    m_colorLibraries[card] = colorLibrary;
    m_paletteCardCollection->addCard(card);

    markToRecalculateTabKeyOrdering();
}

void ColorPicker::removePaletteCard(QSharedPointer<PaletteCard> card)
{
    if (!m_paletteCardCollection->containsCard(card) || !m_colorLibraries.contains(card))
    {
        return;
    }
    
    m_colorLibraries.remove(card);
    m_paletteCardCollection->removeCard(card);

    markToRecalculateTabKeyOrdering();
}

void ColorPicker::beginDynamicColorChange()
{
    m_dynamicColorChange = true;
}

void ColorPicker::endDynamicColorChange()
{
    AZ::Color newColor = m_currentColorController->color();
    if (!m_previousColor.IsClose(newColor))
    {
        auto command = new CurrentColorChangedCommand(this, m_previousColor, newColor);
        m_undoStack->push(command);
    }
    m_dynamicColorChange = false;
}

void ColorPicker::initializeValidation(ColorValidator* validator)
{
    m_currentColorController->setValidator(validator);
    connect(validator, &ColorValidator::colorWarning, this, &ColorPicker::warnColorAdjusted);
    connect(validator, &ColorValidator::colorAccepted, m_warning, &QWidget::hide);
}

void ColorPicker::showPreviewContextMenu(const QPoint& p, const AZ::Color& selectedColor)
{
    QMenu previewMenu;

    QAction* quickPaletteAction = previewMenu.addAction(tr("Add to Quick palette"));
    connect(quickPaletteAction, &QAction::triggered, this, [this, selectedColor]() {
        m_quickPaletteCard->tryAdd(selectedColor);
    });
    quickPaletteAction->setEnabled(!m_quickPaletteCard->contains(selectedColor));

    int paletteCount = m_paletteCardCollection->count();
    if (paletteCount > 0)
    {
        previewMenu.addSeparator();

        for (int paletteIndex = 0; paletteIndex < paletteCount; paletteIndex++)
        {
            QSharedPointer<PaletteCard> paletteCard = m_paletteCardCollection->paletteCard(paletteIndex);
            QAction* namedPaletteAction = previewMenu.addAction(tr("Add to %1 palette").arg(paletteCard->title()));
            connect(namedPaletteAction, &QAction::triggered, [paletteCard, selectedColor]() {
                paletteCard->tryAdd(selectedColor);
            });
            namedPaletteAction->setEnabled(!paletteCard->contains(selectedColor));
        }
    }

    previewMenu.exec(m_preview->mapToGlobal(p));
}

void ColorPicker::swatchSizeActionToggled(bool checked, int newSize)
{
    if (checked)
    {
        const QSize size{ newSize, newSize };
        m_quickPaletteCard->setSwatchSize(size);
        m_paletteCardCollection->setSwatchSize(size);
    }
}

void ColorPicker::setQuickPaletteVisibility(bool show)
{
    m_quickPaletteCard->setVisible(show);
    m_quickPaletteSeparator->setVisible(show);

    m_toggleQuickPaletteAction->setText(tr(show ? "Hide Quick Palette" : "Show Quick Palette"));
}

void ColorPicker::paletteContextMenuRequested(QSharedPointer<PaletteCard> paletteCard, const QPoint& point)
{
    QMenu menu;
    menu.addAction(tr("Save palette"), paletteCard.data(), &PaletteCard::saveClicked);
    menu.addAction(tr("Save palette as..."), paletteCard.data(), &PaletteCard::saveAsClicked);
    menu.addAction(tr("Close palette"), paletteCard.data(), &PaletteCard::removeClicked);
    menu.addSeparator();
    menu.addAction(m_importPaletteAction);
    menu.addAction(m_newPaletteAction);
    menu.addSeparator();
    menu.addActions(m_swatchSizeGroup->actions());

    menu.addSeparator();

    menu.addAction(m_toggleQuickPaletteAction);
    
    menu.addSeparator();

    QAction* moveUp = menu.addAction(tr("Move up"));
    connect(moveUp, &QAction::triggered, this, [&paletteCard, this]() {
        m_paletteCardCollection->moveUp(paletteCard);
    });

    QAction* moveDown = menu.addAction(tr("Move down"));
    connect(moveDown, &QAction::triggered, this, [&paletteCard, this]() {
        m_paletteCardCollection->moveDown(paletteCard);
    });

    moveUp->setEnabled(m_paletteCardCollection->canMoveUp(paletteCard));
    moveDown->setEnabled(m_paletteCardCollection->canMoveDown(paletteCard));

    menu.exec(point);
}

void ColorPicker::quickPaletteContextMenuRequested(const QPoint& point)
{
    m_menu->exec(point);
}

QWidget* ColorPicker::makeSeparator(QWidget* parent)
{
    // makes a horizontal line separator; will not play nicely with the show-on-hover scrollbar
    auto separator = new QFrame(parent);
    separator->setFrameStyle(QFrame::StyledPanel);
    separator->setFrameShadow(QFrame::Plain);
    separator->setFrameShape(QFrame::HLine);
    Style::addClass(separator, SEPARATOR_CLASS);
    return separator;
}

QWidget* ColorPicker::makePaddedSeparator(QWidget* parent)
{
    // makes a horizontal line separator that has padding and plays nicely with the show-on-hover scrollbar
    QWidget* container = new QWidget(parent);
    Style::addClass(container, SEPARATOR_CONTAINER_CLASS);
    QLayout* containerLayout = new QVBoxLayout(container);

    QWidget* separator = makeSeparator(container);
    containerLayout->addWidget(separator);
    m_separators.push_back(container);

    return container;
}

void ColorPicker::savePalette(QSharedPointer<PaletteCard> card, bool queryFileName)
{
    if (!m_paletteCardCollection->containsCard(card) || !m_colorLibraries.contains(card))
    {
        return;
    }

    auto& colorLibrary = m_colorLibraries[card];
    if (saveColorLibrary(colorLibrary, queryFileName))
    {
        card->setTitle(QFileInfo(colorLibrary.fileName).baseName());
        card->setModified(false);
    }
}

bool ColorPicker::saveColorLibrary(ColorLibrary& colorLibrary, bool queryFileName)
{
    if (queryFileName || colorLibrary.fileName.isEmpty())
    {
        QString lastDirectory = colorLibrary.fileName.isEmpty() ? m_lastSaveDirectory : colorLibrary.fileName;

        auto fileName = QFileDialog::getSaveFileName(this, tr("Save Palette As"), lastDirectory, tr("Color Palettes (*.pal)"));
        if (fileName.isEmpty())
        {
            return false;
        }

        m_lastSaveDirectory = fileName;

        colorLibrary.fileName = fileName;
    }

    if (!colorLibrary.palette->save(colorLibrary.fileName))
    {
        QMessageBox::critical(this, tr("Color Palette Export Error"), tr("Failed to save \"%1\"").arg(colorLibrary.fileName));
        return false;
    }

    return true;
}

int ColorPicker::colorLibraryIndex(const Palette* palette) const
{
    auto it = std::find_if(m_colorLibraries.begin(), m_colorLibraries.end(),
                           [palette](const ColorLibrary& entry) { return entry.palette.data() == palette; });
    return it == m_colorLibraries.end() ? -1 : aznumeric_cast<int>(std::distance(m_colorLibraries.begin(), it));
}

void ColorPicker::readSettings()
{
    QSettings settings;
    settings.beginGroup(m_context);

    QString sectionName = ConfigurationName(m_configuration);
    if (!settings.childGroups().contains(sectionName))
    {
        return;
    }
    settings.beginGroup(sectionName);

    if (m_configuration != Configuration::HueSaturation)
    {
        bool showRgbValues = settings.value(g_showRgbValuesKey, true).toBool();
        m_showRgbValuesAction->setChecked(showRgbValues);

        bool showHexValues = settings.value(g_showHexValuesKey, true).toBool();
        m_showHexValueAction->setChecked(showHexValues);
    }

    // Restore last used tab for the color sliders
    int lastSliderTabIndex = settings.value(g_lastSliderTabIndexKey, 0).toInt();
    m_slidersTabWidget->setCurrentIndex(lastSliderTabIndex);

    if (settings.contains(g_quickPaletteKey))
    {
        *m_quickPalette = settings.value(g_quickPaletteKey).value<Palette>();
    }

    QVector<std::pair<QString, Internal::ColorLibrarySettings>> colorLibraries;
    if (settings.contains(g_colorLibrariesKey) && !settings.contains(QStringLiteral("%1/size").arg(g_colorLibrariesKey)))
    {
        settings.remove(g_colorLibrariesKey);
    }

    const int colorLibrariesCount = settings.beginReadArray(g_colorLibrariesKey);
    for (int i = 0; i < colorLibrariesCount; ++i)
    {
        settings.setArrayIndex(i);
        colorLibraries.push_back(std::make_pair(settings.value(g_colorLibraryNameKey).toString(),
            Internal::ColorLibrarySettings{settings.value(g_colorLibraryExpandedKey).toBool()}));
    }
    settings.endArray();

    QStringList missingLibraries;
    for (auto it = colorLibraries.constBegin(), end = colorLibraries.constEnd(); it != end; ++it)
    {
        const QString& fileName = it->first;
        const Internal::ColorLibrarySettings& colorLibrarySettings = it->second;
        if (!importPaletteFromFile(fileName, colorLibrarySettings))
        {
            missingLibraries.append(QDir::toNativeSeparators(fileName));
        }
    }

    if (!missingLibraries.empty())
    {
        QMessageBox::warning(this, tr("Failed to load color libraries"), tr("The following color libraries could not be located on disk:\n%1\n"
            "They will be removed from your saved settings. Please re-import them again if you can locate them.").arg(missingLibraries.join('\n')));
    }

    int swatchSize = settings.value(g_swatchSizeKey, 3).toInt();

    auto swatchSizeActions = m_swatchSizeGroup->actions();
    if (swatchSize >= 0 && swatchSize < swatchSizeActions.count())
    {
        swatchSizeActions[swatchSize]->setChecked(true);
    }

    m_lastSaveDirectory = settings.value(g_paletteFileDialogKey).toString();

    setQuickPaletteVisibility(settings.value(g_showQuickPaletteKey, true).toBool());
}

void ColorPicker::writeSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("%1/%2").arg(m_context, ConfigurationName(m_configuration)));

    settings.setValue(g_showRgbValuesKey, m_rgbaEdit->isVisible());
    settings.setValue(g_showHexValuesKey, m_hexEdit->isVisible());
    settings.setValue(g_lastSliderTabIndexKey, m_slidersTabWidget->currentIndex());

    settings.setValue(g_quickPaletteKey, QVariant::fromValue(*m_quickPalette));

    // Iterate over the color libraries based on the order in the palette card collection.
    // That way, when the user moves the palette cards, the order will be saved properly
    settings.beginWriteArray(g_colorLibrariesKey);
    for (int i = 0, count = m_paletteCardCollection->count(); i < count; i++)
    {
        QSharedPointer<PaletteCard> paletteCard = m_paletteCardCollection->paletteCard(i);
        const ColorLibrary& colorLibrary = m_colorLibraries[paletteCard];
        if (!colorLibrary.fileName.isEmpty())
        {
            settings.setArrayIndex(i);
            settings.setValue(g_colorLibraryNameKey, colorLibrary.fileName);
            settings.setValue(g_colorLibraryExpandedKey, paletteCard->isExpanded());
        }
    }
    settings.endArray();

    auto swatchSizeActions = m_swatchSizeGroup->actions();
    auto it = std::find_if(swatchSizeActions.begin(), swatchSizeActions.end(),
                           [](QAction* action) { return action->isChecked(); });
    if (it != swatchSizeActions.end())
    {
        int swatchSize = aznumeric_cast<int>(std::distance(swatchSizeActions.begin(), it));
        settings.setValue(g_swatchSizeKey, swatchSize);
    }

    settings.setValue(g_paletteFileDialogKey, m_lastSaveDirectory);

    settings.setValue(g_showQuickPaletteKey, m_quickPaletteCard->isVisible());

    settings.endGroup();
    settings.sync();
}

bool ColorPicker::eventFilter(QObject* o, QEvent* e)
{
    if (o == m_containerWidget && e->type() == QEvent::Resize)
    {
        const int f = 2 * m_scrollArea->frameWidth();
        QSize sz{f, f};

        sz += static_cast<QResizeEvent*>(e)->size();
        sz.setWidth(sz.width() + m_scrollArea->verticalScrollBar()->sizeHint().width());

        m_scrollArea->setMinimumSize(sz);
    }

    return StyledDialog::eventFilter(o, e);
}

void ColorPicker::hideEvent(QHideEvent* event)
{
    AZ_UNUSED(event);
    m_colorGrid->StopSelection();
}

} // namespace AzQtComponents

#include "Components/Widgets/moc_ColorPicker.cpp"

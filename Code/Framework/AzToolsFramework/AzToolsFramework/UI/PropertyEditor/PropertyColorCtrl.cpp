/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyColorCtrl.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLineEdit>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
// 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
// qpainter.h(465): warning C4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
#include <QPainter> 
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QToolButton>
#include <QtGui/QRegExpValidator>

namespace AzToolsFramework
{
    PropertyColorCtrl::PropertyColorCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        m_alphaChannelEnabled = false;

        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setAlignment(Qt::AlignLeft);

        static const int minColorEditWidth = 100;
        m_colorEdit = new QLineEdit();
        m_colorEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_colorEdit->setMinimumWidth(minColorEditWidth);
        m_colorEdit->setFixedHeight(PropertyQTConstant_DefaultHeight);
        m_colorEdit->setValidator(CreateTextEditValidator());

        m_pDefaultButton = new QToolButton(this);
        m_pDefaultButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        static const int colorIconWidth = 32;
        static const int colorIconHeight = 16;
        static const int colorButtonWidth = colorIconWidth + 2;
        // The icon size needs to be smaller than the fixed size to make sure it visually aligns properly.
        QSize fixedButtonSize = QSize(colorButtonWidth, PropertyQTConstant_DefaultHeight);
        QSize iconSize = QSize(colorIconWidth, colorIconHeight);
        m_pDefaultButton->setIconSize(iconSize);
        m_pDefaultButton->setFixedSize(fixedButtonSize);

        setLayout(pLayout);

        // Set layout spacing to the desired amount but also account for the
        // color icon being smaller than the button which leaves some blank space
        static const int layoutSpacing = 4 - (colorButtonWidth - colorIconWidth);
        pLayout->setContentsMargins(1, 0, 1, 0);
        pLayout->setSpacing(layoutSpacing);
        pLayout->addWidget(m_pDefaultButton);
        pLayout->addWidget(m_colorEdit);

        m_pColorDialog = nullptr;
        this->setFocusProxy(m_pDefaultButton);
        setFocusPolicy(m_pDefaultButton->focusPolicy());

        connect(m_pDefaultButton, SIGNAL(clicked()), this, SLOT(openColorDialog()));

        connect(m_colorEdit, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));
        connect(m_colorEdit, SIGNAL(returnPressed()), this, SLOT(OnEditingFinished()));
        connect(m_colorEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
    }

    PropertyColorCtrl::~PropertyColorCtrl()
    {
    }

    QColor PropertyColorCtrl::value() const
    {
        return m_color;
    }

    void PropertyColorCtrl::setAlphaChannelEnabled(bool enabled)
    {
        if (m_alphaChannelEnabled != enabled)
        { 
            m_alphaChannelEnabled = enabled;
            m_colorEdit->setValidator(CreateTextEditValidator());
        }
    }

    QWidget* PropertyColorCtrl::GetFirstInTabOrder()
    {
        return m_pDefaultButton;
    }
    QWidget* PropertyColorCtrl::GetLastInTabOrder()
    {
        return m_pDefaultButton;
    }

    void PropertyColorCtrl::UpdateTabOrder()
    {
        // there is only one focusable element in this, the color dialog button.
    }

    void PropertyColorCtrl::SetColor(QColor color, bool updateDialogColor)
    {
        if (color.isValid())
        {
            // Don't update the color if this color was just broadcast out or the picker will loose its initial color state.
            updateDialogColor &= color != m_lastSetColor;
            m_lastSetColor = color;

            m_color = color;
            QPixmap pixmap(m_pDefaultButton->iconSize());
            pixmap.fill(TransformColor(color, m_config.m_propertyColorSpaceId, m_config.m_colorSwatchColorSpaceId));
            // Draw a border
            QPainter painter(&pixmap);
            painter.setPen(QColor("#333333"));
            painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));
            QIcon newIcon(pixmap);
            m_pDefaultButton->setIcon(newIcon);
            if (m_pColorDialog && updateDialogColor)
            {
                AZ::Color azColor = AzQtComponents::fromQColor(m_color);
                azColor = TransformColor(azColor, m_config.m_propertyColorSpaceId, m_config.m_colorPickerDialogColorSpaceId);
                m_pColorDialog->setCurrentColor(azColor);
            }

            auto colorStr = AzQtComponents::MakePropertyDisplayStringInts(m_color, m_alphaChannelEnabled);
            m_colorEdit->setText(colorStr);
        }
    }

    void PropertyColorCtrl::setValue(QColor val)
    {
        SetColor(val);
    }

    void PropertyColorCtrl::openColorDialog()
    {
        m_originalColor = m_color;
        m_lastSetColor = m_color;

        CreateColorDialog();

        if (m_pColorDialog != nullptr)
        {
            AZ::Color color = AzQtComponents::fromQColor(m_originalColor);
            color = TransformColor(color, m_config.m_propertyColorSpaceId, m_config.m_colorPickerDialogColorSpaceId);
            m_pColorDialog->setCurrentColor(color);
            m_pColorDialog->setSelectedColor(color);
            m_pColorDialog->show();
        }
    }

    QRegExpValidator* PropertyColorCtrl::CreateTextEditValidator()
    {
        /*Use regex to validate the input
        *\d\d?    Match 0-99
        *1\d\d    Match 100 - 199
        *2[0-4]\d Match 200-249
        *25[0-5]  Match 250 - 255
        *(25[0-5]|2[0-4]\d|1\d\d|\d\d?)\s*,\s*){2} Match the first two (or three with alpha channel) "0-255,"
        *(25[0-5]|2[0-4]\d|1\d\d|\d\d?) Match the last "0-255"
        */

        int numInitialChannelComponents = m_alphaChannelEnabled ? 3 : 2;

        const QString regex = QString(
            R"(^\s*((25[0-5]|2[0-4]\d|1\d\d|\d\d?)\s*,\s*){%1}(25[0-5]|2[0-4]\d|1\d\d|\d\d?)\s*$)"
        ).arg(numInitialChannelComponents);

        return new QRegExpValidator(QRegExp(regex), this);
    }

    void PropertyColorCtrl::CreateColorDialog()
    {
        // Don't need to create a dialog if it already exists.
        if (m_pColorDialog != nullptr)
        {
            return;
        }
        const auto config = m_alphaChannelEnabled ? AzQtComponents::ColorPicker::Configuration::RGBA : AzQtComponents::ColorPicker::Configuration::RGB;
        m_pColorDialog = new AzQtComponents::ColorPicker(config, QString(), this);
        m_pColorDialog->setWindowTitle(tr("Select Color"));
        m_pColorDialog->setWindowModality(Qt::ApplicationModal);
        m_pColorDialog->setAttribute(Qt::WA_DeleteOnClose);

        if (m_config.m_propertyColorSpaceId != m_config.m_colorPickerDialogColorSpaceId)
        {
            m_pColorDialog->setAlternateColorspaceEnabled(true);

            AZStd::string propertyColorSpaceName = m_config.m_colorSpaceNames[m_config.m_propertyColorSpaceId];
            AZStd::string dialogColorSpaceName = m_config.m_colorSpaceNames[m_config.m_colorPickerDialogColorSpaceId];
            if (!propertyColorSpaceName.empty() && !dialogColorSpaceName.empty())
            {
                QString comment = AZStd::string::format("Mixing space: %s | Final space: %s", dialogColorSpaceName.c_str(), propertyColorSpaceName.c_str()).c_str();
                m_pColorDialog->setComment(comment);
            }

            if (!propertyColorSpaceName.empty())
            {
                m_pColorDialog->setAlternateColorspaceName(propertyColorSpaceName.c_str());
            }
            else
            {
                m_pColorDialog->setAlternateColorspaceName("Output");
            }
        }

        connect(m_pColorDialog, &AzQtComponents::ColorPicker::currentColorChanged, this, 
            [=](AZ::Color color)
        {
            color = TransformColor(color, m_config.m_colorPickerDialogColorSpaceId, m_config.m_propertyColorSpaceId);
            m_pColorDialog->setAlternateColorspaceValue(color);
            onSelected(AzQtComponents::toQColor(color));
        });

        connect(m_pColorDialog, &QDialog::rejected, this,
            [this]()
        {
            onSelected(m_originalColor);
        });

        connect(m_pColorDialog, &QDialog::finished, this,
            [this]()
        {
            emit editingFinished();
            m_pColorDialog = nullptr;
        });


        // Position the picker around cursor.
        QLayout* layout = m_pColorDialog->layout();
        if (layout)
        {
            QSize halfSize = (layout->sizeHint() / 2);
            m_pColorDialog->move(QCursor::pos() - QPoint(halfSize.width(), halfSize.height()));
        }
    }

    void PropertyColorCtrl::onSelected(QColor color)
    {
        // Update the color but don't need to update the dialog color since
        // this signal came from the dialog
        if (m_color != color)
        {
            SetColor(color, false);
            emit valueChanged(m_color);
        }
    }

    QColor PropertyColorCtrl::convertFromString(const QString& string)
    {
        QStringList strList = string.split(",", Qt::SkipEmptyParts);
        int R = 0, G = 0, B = 0, A = 255;
        AZ_Assert(strList.size() == 3 || strList.size() == 4, "Invalid input string for RGB field!");
        R = strList[0].trimmed().toInt();
        G = strList[1].trimmed().toInt();
        B = strList[2].trimmed().toInt();
        if (m_alphaChannelEnabled)
        { 
            A = strList[3].trimmed().toInt();
        }
        return QColor(R, G, B, A);
    }

    ////////////////////////////////////////////////////////////////
    QWidget* AZColorPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyColorCtrl* newCtrl = aznew PropertyColorCtrl(pParent);
        connect(newCtrl, &PropertyColorCtrl::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &PropertyEditorGUIMessages::RequestWrite, newCtrl);
        });
        connect(newCtrl, &PropertyColorCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    // called when the user pressed enter, tab or we lost focus
    void PropertyColorCtrl::OnEditingFinished()
    {

        int pos = 0;
        QString text = m_colorEdit->text();
        QValidator::State validateState = m_colorEdit->validator()->validate(text, pos);

        if (validateState == QValidator::State::Acceptable)
        {
            QColor newValue = convertFromString(text);
            if (newValue != m_color)
            {
                setValue(newValue);
                emit valueChanged(newValue);
            }
        }
        else
        {   
            //If the text is not valid, set back to old value
            setValue(m_color);
        }

        emit editingFinished();
    }

    // called each time when a new character has been entered or removed, this doesn't update the value yet
    void PropertyColorCtrl::OnTextEdited(const QString& newText)
    {
        int pos = 0;
        QString text = newText;
        QValidator::State validateState = m_colorEdit->validator()->validate(text, pos);

        if (validateState == QValidator::State::Invalid)
        {
            m_colorEdit->setStyleSheet("color: red;");
        }
        else
        {
            m_colorEdit->setStyleSheet("");
        }
    }

    void AZColorPropertyHandler::ConsumeAttribute(PropertyColorCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ_CRC_CE("AlphaChannel"))
        {
            bool alphaChannel;
            if (attrValue->Read<bool>(alphaChannel))
            { 
                GUI->setAlphaChannelEnabled(alphaChannel);
            }
            else
            {
                (void)debugName;
                AZ_WarningOnce("AZColorPropertyHandler", false, "Failed to read 'AlphaChannel' attribute from property '%s'; expected `bool' type", debugName);
            }
        }
        // Unlike other property editors that are configured through a collection of attributes attributes, PropertyColorCtrl uses a single ColorEditorConfiguration
        // attribute that encompasses many settings. This is because ColorEditorConfiguration's settings are closely related to each other, and if a widget needs
        // to set one of these settings, it likely needs to set all of the settings. So it's less cumbersome to just group them all together.
        else if (attrib == AZ_CRC_CE("ColorEditorConfiguration"))
        {
            ColorEditorConfiguration config;
            if (attrValue->Read<ColorEditorConfiguration>(config))
            {
                GUI->SetColorEditorConfiguration(config);
            }
        }
    }

    
    void PropertyColorCtrl::SetColorEditorConfiguration(const ColorEditorConfiguration& configuration)
    {
        m_config = configuration;
        setAlphaChannelEnabled(m_config.m_colorPickerDialogConfiguration == AzQtComponents::ColorPicker::Configuration::RGBA);
    }

    QColor PropertyColorCtrl::TransformColor(const QColor& color, uint32_t fromColorSpaceId, uint32_t toColorSpaceId) const
    {
        AZ::Color azColor = AzQtComponents::FromQColor(color);
        azColor = TransformColor(azColor, fromColorSpaceId, toColorSpaceId);
        return AzQtComponents::ToQColor(azColor);
    }

    AZ::Color PropertyColorCtrl::TransformColor(const AZ::Color& color, uint32_t fromColorSpaceId, uint32_t toColorSpaceId) const
    {
        if (m_config.m_transformColorCallback)
        {
            return m_config.m_transformColorCallback(color, fromColorSpaceId, toColorSpaceId);
        }

        return color;
    }

    void AZColorPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyColorCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        QColor val = GUI->value();
        AZ::Color asAZColor((float)val.redF(), (float)val.greenF(), (float)val.blueF(), (float)val.alphaF());
        instance = static_cast<property_t>(asAZColor);
    }

    bool AZColorPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZ::Vector4 asVector4 = static_cast<AZ::Vector4>(instance);
        QColor asQColor;
        asQColor.setRedF((qreal)asVector4.GetX());
        asQColor.setGreenF((qreal)asVector4.GetY());
        asQColor.setBlueF((qreal)asVector4.GetZ());
        asQColor.setAlphaF((qreal)asVector4.GetW());
        GUI->setValue(asQColor);
        return false;
    }

    QWidget* Vector3ColorPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyColorCtrl* newCtrl = aznew PropertyColorCtrl(pParent);
        connect(newCtrl, &PropertyColorCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        connect(newCtrl, &PropertyColorCtrl::editingFinished, this, [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
            });
        // note: Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        return newCtrl;
    }
    void Vector3ColorPropertyHandler::ConsumeAttribute(PropertyColorCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }
    void Vector3ColorPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyColorCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        QColor val = GUI->value();
        AZ::Vector3 asVector3((float)val.redF(), (float)val.greenF(), (float)val.blueF());
        instance = static_cast<property_t>(asVector3);
    }

    bool Vector3ColorPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        AZ::Vector3 asVector3 = static_cast<AZ::Vector3>(instance);
        QColor asQColor;
        asQColor.setRedF((qreal)asVector3.GetX());
        asQColor.setGreenF((qreal)asVector3.GetY());
        asQColor.setBlueF((qreal)asVector3.GetZ());
        GUI->setValue(asQColor);
        return false;
    }

    ////////////////////////////////////////////////////////////////
    void RegisterColorPropertyHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew Vector3ColorPropertyHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew AZColorPropertyHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyColorCtrl.cpp"

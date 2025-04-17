/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QWidget>

#include "PropertyAudioCtrlTypes.h"
#endif

class QLabel;
class QLineEdit;
class QPushButton;
class QHBoxLayout;
class QMimeData;


namespace AzToolsFramework
{
    //=============================================================================
    // Audio Control Selector Request Bus
    // For connecting UI proper
    //=============================================================================
    class AudioControlSelectorRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AudioPropertyType;

        virtual AZStd::string SelectResource(AZStd::string_view previousValue)
        {
            return previousValue;
        }
    };

    using AudioControlSelectorRequestBus = AZ::EBus<AudioControlSelectorRequests>;

    //=============================================================================
    // Audio Control Selector Widget
    //=============================================================================
    class AudioControlSelectorWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AudioControlSelectorWidget, AZ::SystemAllocator);
        AudioControlSelectorWidget(QWidget* parent = nullptr);

        void SetControlName(const QString& controlName);
        QString GetControlName() const;
        void SetPropertyType(AudioPropertyType type);
        AudioPropertyType GetPropertyType() const
        {
            return m_propertyType;
        }

        // todo: enable drag-n-drop from Audio Controls Editor
        bool eventFilter(QObject* obj, QEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    signals:
        void ControlNameChanged(const QString& path);

        public slots:
        void OnClearControl();
        void OnOpenAudioControlSelector();

    protected:
        bool IsCorrectMimeData(const QMimeData* pData) const;
        void focusInEvent(QFocusEvent* event) override;

        AzQtComponents::BrowseEdit* m_browseEdit;
        QHBoxLayout* m_mainLayout;

        AudioPropertyType m_propertyType;
        QString m_controlName;

    private:
        void UpdateWidget();
        static AZStd::string GetResourceSelectorNameFromType(AudioPropertyType propertyType);
    };


    //=============================================================================
    // Property Handler
    //=============================================================================
    class AudioControlSelectorWidgetHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<CReflectedVarAudioControl, AudioControlSelectorWidget>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AudioControlSelectorWidgetHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override
        {
            return AZ_CRC_CE("AudioControl");
        }

        bool IsDefaultHandler() const override
        {
            return true;
        }

        QWidget* CreateGUI(QWidget* parent) override;
        void ConsumeAttribute(widget_t* gui, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, widget_t* gui, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, widget_t* gui, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    void RegisterAudioPropertyHandler();

} // namespace AzToolsFramework

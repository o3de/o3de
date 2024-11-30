/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_ENTITYIDCTRL_CTRL
#define PROPERTY_ENTITYIDCTRL_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <QtWidgets/QWidget>
#include <QIcon>
#include "PropertyEditorAPI.h"
#endif

class QCheckBox;
class QLineEdit;
class QMimeData;
class QToolButton;

namespace AzToolsFramework
{
    class EntityIdQLineEdit;
    class EditorEntityIdContainer;

    //just a test to see how it would work to pop a dialog

    class PropertyEntityIdCtrl
        : public QWidget
        , private EditorPickModeRequestBus::Handler
        , private EditorEvents::Bus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyEntityIdCtrl, AZ::SystemAllocator);

        PropertyEntityIdCtrl(QWidget *pParent = NULL);
        virtual ~PropertyEntityIdCtrl();

        AZ::EntityId GetEntityId() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

        bool SetChildWidgetsProperty(const char* name, const QVariant& variant);

        // QT overrides
        virtual void dragEnterEvent(QDragEnterEvent* event) override;
        virtual void dropEvent(QDropEvent* event) override;
        virtual void dragLeaveEvent(QDragLeaveEvent* event) override;

        // EditorPickModeRequestBus
        void StopEntityPickMode() override;
        void PickModeSelectEntity(AZ::EntityId /*id*/) override;

        // AzToolsFramework::EditorEvents::Bus::Handler
        void OnEscape() override;

        void SetRequiredServices(AZStd::span<const AZ::ComponentServiceType> requiredServices);
        void SetIncompatibleServices(AZStd::span<const AZ::ComponentServiceType> incompatibleServices);
        void SetMismatchedServices(bool mismatchedServices);

        void SetAcceptedEntityContext(AzFramework::EntityContextId contextId);

        void SetHasClearButton(bool value) { m_hasClearButton = value; }
        void SetHasPickButton(bool value) { m_hasPickButton = value; }
        void SetAllowsDrop(bool value) { m_allowsDrop = value; }

        bool HasClearButton() { return m_hasClearButton; }
        bool HasPickButton() { return m_hasPickButton; }
        bool AllowsDrop() { return m_allowsDrop; }

    signals:
        void OnEntityIdChanged(AZ::EntityId newEntityId);

        void OnPickStart();
        void OnPickComplete();

    public slots:
        void SetCurrentEntityId(const AZ::EntityId& newEntityId, bool emitChange, const AZStd::string& nameOverride);
        void ClearEntityId();

    protected:
        bool IsCorrectMimeData(const QMimeData* mimeData) const;
        bool EntityIdsFromMimeData(const QMimeData& mimeData, AzToolsFramework::EditorEntityIdContainer* entityIdListContainer = nullptr) const;

        // Move the editor into Pick Mode.
        void StartEntityPickMode();
        AzFramework::EntityContextId GetPickModeEntityContextId();

        QString BuildTooltip();

        EntityIdQLineEdit* m_entityIdLineEdit;
        QToolButton* m_pickButton;
        AZ::ComponentDescriptor::DependencyArrayType m_requiredServices;
        AZ::ComponentDescriptor::DependencyArrayType m_incompatibleServices;
        AzFramework::EntityContextId m_acceptedEntityContextId;
        AZStd::list<AZStd::string> m_componentsSatisfyingServices;

        bool m_hasClearButton{ true };
        bool m_hasPickButton{ true };
        bool m_allowsDrop { true };

        QIcon m_pickerIcon;
    };

    class EntityIdPropertyHandler : QObject, public PropertyHandler<AZ::EntityId, PropertyEntityIdCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EntityIdPropertyHandler, AZ::SystemAllocator);

        virtual AZ::u32 GetHandlerName(void) const override { return AZ::Edit::UIHandlers::EntityId; }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyEntityIdCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyEntityIdCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyEntityIdCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget *pParent) override;
        virtual void ConsumeAttribute(PropertyEntityIdCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyEntityIdCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyEntityIdCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterEntityIdPropertyHandler();

} // namespace AzToolsFramework

#endif

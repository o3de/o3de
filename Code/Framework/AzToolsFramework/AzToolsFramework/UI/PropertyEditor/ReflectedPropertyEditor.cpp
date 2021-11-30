/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ReflectedPropertyEditor.hxx"
#include "PropertyRowWidget.hxx"
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Sfmt.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QApplication>
#include <QPainter>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QTextFormat::d': class 'QSharedDataPointer<QTextFormatPrivate>' needs to have dll-interface to be used by clients of class 'QTextFormat'
#include <QtWidgets/QInputDialog>
AZ_POP_DISABLE_WARNING
#include <QtCore/QTimer>
#include <QtCore/QSet>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzCore/std/sort.h>

namespace AzToolsFramework
{
    const AZ::SerializeContext::ClassData* CreateContainerElementSelectClassCallback(const AZ::Uuid& classId, const AZ::Uuid& typeId, AZ::SerializeContext* context)
    {
        AZStd::vector<const AZ::SerializeContext::ClassData*> derivedClasses;
        context->EnumerateDerived(
            [&derivedClasses]
        (const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*knownType*/) -> bool
        {
            derivedClasses.push_back(classData);
            return true;
        },
            classId,
            typeId
            );

        if (derivedClasses.empty())
        {
            const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeId);
            const char* className = classData ?
                (classData->m_editData ? classData->m_editData->m_name : classData->m_name)
                : "<unknown>";

            QMessageBox mb(QMessageBox::Information,
                QObject::tr("Select Class"),
                QObject::tr("No classes could be found that derive from \"%1\".").arg(className),
                QMessageBox::Ok);
            mb.exec();
            return nullptr;
        }

        QStringList derivedClassNames;
        for (auto& derivedClass : derivedClasses)
        {
            const char* derivedClassName = derivedClass->m_editData ? derivedClass->m_editData->m_name : derivedClass->m_name;
            derivedClassNames.push_back(derivedClassName);
        }

        QString item;
        QInputDialog dialog(nullptr);
        dialog.setWindowTitle(QObject::tr("Class to create"));
        dialog.setLabelText(QObject::tr("Classes"));
        dialog.setComboBoxItems(derivedClassNames);
        dialog.setTextValue(derivedClassNames.value(0));
        dialog.setComboBoxEditable(false);
        dialog.setModal(true);
        // Dialog box won't adjust size until after it is shown.
        dialog.show();
        dialog.adjustSize();
        bool ok = dialog.exec();
        if (ok)
        {
            item = dialog.textValue();
        }
        else
        {
            return nullptr;
        }

        // Reverse lookup the derived class from the class name selected
        for (int derivedClassNameIndex = 0; derivedClassNameIndex < derivedClassNames.size(); ++derivedClassNameIndex)
        {
            if (derivedClassNames[derivedClassNameIndex] == item)
            {
                return derivedClasses[derivedClassNameIndex];
            }
        }

        return nullptr;
    }

    // Internal bus used so that instances of the reflected property editor in different
    // dlls/shared libs still work and communicate when refreshes are required
    class InternalReflectedPropertyEditorEvents
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<InternalReflectedPropertyEditorEvents>;

        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // there's only one address to this bus, its always broadcast
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // each instance connects to it.
        //////////////////////////////////////////////////////////////////////////

        virtual void QueueInvalidationIfSharedData(InternalReflectedPropertyEditorEvents* sender, PropertyModificationRefreshLevel level, const AZStd::set<void*>& sourceInstanceSet) = 0;
    };

    class ReflectedPropertyEditorUpdateSentinel
    {
    public:
        ReflectedPropertyEditorUpdateSentinel(ReflectedPropertyEditor* propertyEditor, int* guardVariable)
            : m_propertyEditor(propertyEditor)
            , m_guardVariable(guardVariable)
        {
            int& guardVariableRef = *m_guardVariable;
            if (guardVariableRef++ == 0)
            {
                m_propertyEditor->setUpdatesEnabled(false);
            }
        }

        ~ReflectedPropertyEditorUpdateSentinel()
        {
            int& guardVariableRef = *m_guardVariable;
            if (--guardVariableRef == 0)
            {
                m_propertyEditor->setUpdatesEnabled(true);
            }
        }

    private:
        QWidget* m_propertyEditor;
        int* m_guardVariable;
    };

    class ReflectedPropertyEditor::Impl
        : private PropertyEditorGUIMessages::Bus::Handler
        , private InternalReflectedPropertyEditorEvents::Bus::Handler
    {
        friend class ReflectedPropertyEditor;

    public:
        Impl(ReflectedPropertyEditor* editor)
            : m_editor(editor)
            , m_sizeHintOffset(5, 5)
            , m_treeIndentation(20) // 16px + 4px to match card header
            , m_leafIndentation(16)
        {
        }

        typedef AZStd::list<InstanceDataHierarchy> InstanceDataHierarchyList;
        typedef AZStd::unordered_map<QWidget*, InstanceDataNode*> UserWidgetToDataMap;
        typedef AZStd::vector<PropertyRowWidget*> RowContainerType;

        ReflectedPropertyEditor*            m_editor;
        AZ::SerializeContext*               m_context;
        IPropertyEditorNotify*              m_ptrNotify;
        ComponentEditor*                    m_editorParent;
        InstanceDataHierarchyList           m_instances; ///< List of instance sets to display, other one can aggregate other instances.
        InstanceDataHierarchy::ValueComparisonFunction m_valueComparisonFunction;
        ReflectedPropertyEditor::WidgetList m_widgets;
        ReflectedPropertyEditor::WidgetList m_specialGroupWidgets;
        InstanceDataNode* groupSourceNode = nullptr;
        RowContainerType m_widgetsInDisplayOrder;
        UserWidgetToDataMap m_userWidgetsToData;
        VisibilityCallback m_visibilityCallback;
        void AddProperty(InstanceDataNode* node, PropertyRowWidget* pParent, int depth, AZStd::string_view labelOverride = "");
        void CreateEditorWidget(PropertyRowWidget* pWidget);
        void ExpandChildren(PropertyRowWidget* pWidget, bool expand, bool checkWhetherChildShouldExpand);
        void UpdateExpansionState();
        QSet<PropertyRowWidget*> getTopLevelWidgets() const;

        ////////////////////////////////////////////////////////////////////////////////////////
        // Support for logical property groups / visual hierarchy.
        PropertyRowWidget* GetOrCreateLogicalGroupWidget(InstanceDataNode* node, PropertyRowWidget* parent, int depth);
        size_t CountRowsInAllDescendents(PropertyRowWidget* pParent);

        using GroupWidgetList = AZStd::unordered_map<AZStd::pair<PropertyRowWidget*, AZStd::string>, PropertyRowWidget*>;
        GroupWidgetList m_groupWidgets;
        ////////////////////////////////////////////////////////////////////////////////////////

        QVBoxLayout* m_rowLayout;
        QScrollArea* m_mainScrollArea;
        QWidget* m_containerWidget;

        QSpacerItem* m_spacer;
        AZStd::vector<PropertyRowWidget*> m_widgetPool;

        PropertyRowWidget* CreateOrPullFromPool();
        void ReturnAllToPool();

        void SaveExpansion();
        bool ShouldRowAutoExpand(PropertyRowWidget* widget) const;

        void AdjustLabelWidth();

        AZ::u32 m_savedStateKey;

        int m_propertyLabelAutoResizeMinimumWidth;
        int m_propertyLabelWidth;
        bool m_autoResizeLabels;

        PropertyRowWidget *m_selectedRow;
        bool m_selectionEnabled;

        AZStd::intrusive_ptr<ReflectedPropertyEditorState> m_savedState;

        AZ::u32 CreatePathKey(PropertyRowWidget* widget) const;
        bool CheckSavedExpandState(AZ::u32 pathKey) const;
        bool HasSavedExpandState(AZ::u32 pathKey) const;

        PropertyModificationRefreshLevel m_queuedRefreshLevel;
        bool m_preventRefresh = false;
        bool m_lastEnabledState = true;

        bool m_hideRootProperties;
        bool m_queuedTabOrderRefresh;
        DynamicEditDataProvider m_dynamicEditDataProvider;

        bool FilterNode(InstanceDataNode* node, const char* filter);
        bool IsFilteredOut(InstanceDataNode* node);

        bool m_hasFilteredOutNodes = false;

        AZStd::unordered_map<InstanceDataNode*, bool> m_nodeFilteredOutState;

        ReadOnlyQueryFunction   m_readOnlyQueryFunction;
        HiddenQueryFunction     m_hiddenQueryFunction;
        IndicatorQueryFunction  m_indicatorQueryFunction;

        // Offset to add to size hint. Used to leave a border around the widget
        QSize m_sizeHintOffset;

        int m_treeIndentation;
        int m_leafIndentation;

    private:
        AZStd::set<void*> CreateInstanceSet();
        bool Intersects(const AZStd::set<void*>& cachedInstanceSet);
        void QueueInvalidationForAllMatchingReflectedPropertyEditors(PropertyModificationRefreshLevel level);

        // InternalReflectedPropertyEditorEvents overrides
        void QueueInvalidationIfSharedData(InternalReflectedPropertyEditorEvents* sender, PropertyModificationRefreshLevel level, const AZStd::set<void*>& sourceInstanceSet) override;

        // PropertyEditorGUIMessages::Bus::Handler
        void RequestWrite(QWidget* editorGUI) override;
        void AddElementsToParentContainer(QWidget* editorGUI, size_t numElements, const InstanceDataNode::FillDataClassCallback& fillDataCallback) override;
        void RequestRefresh(PropertyModificationRefreshLevel) override;
        void RequestPropertyNotify(QWidget* editorGUI) override;
        void OnEditingFinished(QWidget* editorGUI) override;
    };

    class ReflectedPropertyEditorState
        : public AZ::UserSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(ReflectedPropertyEditorState, AZ::SystemAllocator, 0);
        AZ_RTTI(ReflectedPropertyEditorState, "{A229B615-622B-4C0B-A17C-A1F5C3144D6E}", AZ::UserSettings);

        AZStd::unordered_set<AZ::u32> m_expandedElements; // crc of them + their parents.
        AZStd::unordered_set<AZ::u32> m_savedElements; // Elements we are aware of and have a valid ExpandedElements structure for

        ReflectedPropertyEditorState()
        {
        }

        // the Reflected Property Editor remembers which properties are expanded:
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ReflectedPropertyEditorState>()
                    ->Version(3)
                    ->Field("m_expandedElements", &ReflectedPropertyEditorState::m_expandedElements)
                    ->Field("m_savedElements", &ReflectedPropertyEditorState::m_savedElements);
            }
        }

        void SetExpandedState(const AZ::u32 key, const bool state)
        {
            if (key != 0)
            {
                m_savedElements.insert(key);

                if (state)
                {
                    m_expandedElements.insert(key);
                }
                else
                {
                    m_expandedElements.erase(key);
                }
            }
        }

        bool GetExpandedState(const AZ::u32 key) const
        {
            return m_expandedElements.find(key) != m_expandedElements.end();
        }

        bool HasExpandedState(const AZ::u32 key) const
        {
            return m_savedElements.find(key) != m_savedElements.end();
        }
    };

    ReflectedPropertyEditor::ReflectedPropertyEditor(QWidget* pParent)
        : QFrame(pParent)
        , m_impl(new Impl(this))
    {
        m_impl->m_propertyLabelWidth = 200;
        m_impl->m_propertyLabelAutoResizeMinimumWidth = 0;

        m_impl->m_savedStateKey = 0;

        m_impl->m_editorParent = nullptr;
        setLayout(new QVBoxLayout());
        layout()->setSpacing(0);
        layout()->setContentsMargins(0, 0, 0, 0);

        m_impl->m_mainScrollArea = nullptr; // we only create this if needed

        m_impl->m_containerWidget = new QWidget(this);
        m_impl->m_containerWidget->setObjectName("ContainerForRows");
        m_impl->m_rowLayout = new QVBoxLayout(m_impl->m_containerWidget);
        m_impl->m_rowLayout->setContentsMargins(0, 0, 0, 0);
        m_impl->m_rowLayout->setSpacing(0);


        m_impl->PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        m_impl->InternalReflectedPropertyEditorEvents::Bus::Handler::BusConnect();
        m_impl->m_queuedRefreshLevel = Refresh_None;
        m_impl->m_preventRefresh = false;
        m_impl->m_ptrNotify = nullptr;
        m_impl->m_hideRootProperties = false;
        m_impl->m_queuedTabOrderRefresh = false;
        m_impl->m_spacer = nullptr;
        m_impl->m_context = nullptr;
        m_impl->m_selectedRow = nullptr;
        m_impl->m_selectionEnabled = false;
        m_impl->m_autoResizeLabels = false;

        m_impl->m_hasFilteredOutNodes = false;
    }

    ReflectedPropertyEditor::~ReflectedPropertyEditor()
    {
        m_releasePrompt = true;

        m_impl->InternalReflectedPropertyEditorEvents::Bus::Handler::BusDisconnect();
        m_impl->PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
    }

    void ReflectedPropertyEditor::Setup(AZ::SerializeContext* context, IPropertyEditorNotify* pnotify, bool enableScrollbars, int propertyLabelWidth, ComponentEditor* editorParent)
    {
        m_impl->m_ptrNotify = pnotify;
        m_impl->m_context = context;
        m_impl->m_editorParent = editorParent;

        m_impl->m_propertyLabelWidth = propertyLabelWidth;

        if (!enableScrollbars)
        {
            // NO SCROLL BARS LAYOUT:
            // this (VBoxLayout)   - created in constructor
            //      - Container Widget (VBoxLayout) - created in constructor
            //      - Spacer to eat up remaining Space
            qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, m_impl->m_containerWidget);
        }
        else
        {
            // SCROLL BARS layout
            // this (VBoxLayout)
            //      - Scroll Area
            //          - Container Widget (VBoxLayout)
            //      - Spacer to eat up remaining space
            m_impl->m_mainScrollArea = new QScrollArea(this);
            m_impl->m_mainScrollArea->setWidgetResizable(true);
            m_impl->m_mainScrollArea->setFocusPolicy(Qt::ClickFocus);
            m_impl->m_containerWidget->setParent(m_impl->m_mainScrollArea);
            m_impl->m_mainScrollArea->setWidget(m_impl->m_containerWidget);
            qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, m_impl->m_mainScrollArea);
        }
    }

    void ReflectedPropertyEditor::SetHideRootProperties(bool hideRootProperties)
    {
        m_impl->m_hideRootProperties = hideRootProperties;
    }

    bool ReflectedPropertyEditor::AddInstance(void* instance, const AZ::Uuid& classId, void* aggregateInstance, void* compareInstance)
    {
#if defined(_DEBUG)
        for (auto testIt = m_impl->m_instances.begin(); testIt != m_impl->m_instances.end(); ++testIt)
        {
            for (size_t idx = 0; idx < testIt->GetNumInstances(); ++idx)
            {
                AZ_Assert(testIt->GetInstance(idx) != instance, "Attempt to add a duplicate instance to a property editor.");
            }
        }
#endif
        if (aggregateInstance)
        {
            // find aggregate instance
            for (auto it = m_impl->m_instances.begin(); it != m_impl->m_instances.end(); ++it)
            {
                if (it->ContainsRootInstance(aggregateInstance))
                {
                    it->AddRootInstance(instance, classId);

                    if (compareInstance)
                    {
                        it->AddComparisonInstance(compareInstance, classId);
                    }

                    return true;
                }
            }
        }
        else
        {
            m_impl->m_instances.push_back();
            m_impl->m_instances.back().SetValueComparisonFunction(m_impl->m_valueComparisonFunction);
            m_impl->m_instances.back().AddRootInstance(instance, classId);

            if (compareInstance)
            {
                m_impl->m_instances.back().AddComparisonInstance(compareInstance, classId);
            }

            return true;
        }
        return false;
    }

    void ReflectedPropertyEditor::EnumerateInstances(InstanceDataHierarchyCallBack enumerationCallback)
    {
        for (auto& instance : m_impl->m_instances)
        {
            enumerationCallback(instance);
        }
    }

    void ReflectedPropertyEditor::SetValueComparisonFunction(const InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction)
    {
        m_impl->m_valueComparisonFunction = valueComparisonFunction;
    }

    void ReflectedPropertyEditor::SetReadOnlyQueryFunction(const ReadOnlyQueryFunction& readOnlyQueryFunction)
    {
        m_impl->m_readOnlyQueryFunction = readOnlyQueryFunction;
    }

    void ReflectedPropertyEditor::SetHiddenQueryFunction(const HiddenQueryFunction& hiddenQueryFunction)
    {
        m_impl->m_hiddenQueryFunction = hiddenQueryFunction;
    }

    void ReflectedPropertyEditor::SetIndicatorQueryFunction(const IndicatorQueryFunction& indicatorQueryFunction)
    {
        m_impl->m_indicatorQueryFunction = indicatorQueryFunction;
    }

    bool ReflectedPropertyEditor::HasFilteredOutNodes() const
    {
        return m_impl->m_hasFilteredOutNodes;
    }

    bool ReflectedPropertyEditor::HasVisibleNodes() const
    {
        return m_impl->m_widgetsInDisplayOrder.size() > 0;
    }

    void ReflectedPropertyEditor::ClearInstances()
    {
        m_releasePrompt = true;

        m_impl->SaveExpansion();
        m_impl->ReturnAllToPool();
        m_impl->m_instances.clear();
        m_impl->m_selectedRow = nullptr;
    }

    PropertyRowWidget* ReflectedPropertyEditor::Impl::GetOrCreateLogicalGroupWidget(InstanceDataNode* node, PropertyRowWidget* parent, int depth)
    {
        // Locate the logical group closest to this node in the hierarchy, if one exists.
        // This will be the most recently-encountered "Group" ClassElement prior to our DataElement.

        if (node && node->GetParent() && node->GetParent()->GetClassMetadata())
        {
            const AZ::Edit::ElementData* groupElementData = node->GetGroupElementMetadata();
            // if the node is in a group then create the widget for the group
            if (groupElementData)
            {
                bool isToggleGroup = false;
                const char* groupName = groupElementData->m_description;
                PropertyRowWidget*& widgetEntry = m_groupWidgets[{parent, groupName}];

                // Create the group's widget if we haven't already.
                if (!widgetEntry)
                {
                    widgetEntry = CreateOrPullFromPool();
                    widgetEntry->SetFilterString(m_editor->GetFilterString());

                    // Initialized normally if the group does not have a member variable attached to it,
                    // otherwise initialize it as a group that will have a toggle switch.
                    if (groupElementData->IsClassElement())
                    {
                        widgetEntry->Initialize(groupName, parent, depth, m_propertyLabelWidth);
                    }
                    else
                    {
                        widgetEntry->InitializeToggleGroup(groupName, parent, depth, groupSourceNode, m_propertyLabelWidth);
                        QWidget* toggleSwitch = widgetEntry->GetToggle();
                        PropertyHandlerBase* pHandler = widgetEntry->GetHandler();
                        m_userWidgetsToData[toggleSwitch] = groupSourceNode;
                        m_specialGroupWidgets[groupSourceNode] = widgetEntry;
                        pHandler->ConsumeAttributes_Internal(toggleSwitch, groupSourceNode);
                        pHandler->ReadValuesIntoGUI_Internal(toggleSwitch, groupSourceNode);
                        widgetEntry->OnValuesUpdated();
                        isToggleGroup = true;
                    }

                    widgetEntry->SetLeafIndentation(m_leafIndentation);
                    widgetEntry->SetTreeIndentation(m_treeIndentation);
                    widgetEntry->setObjectName(groupName);

                    for (const AZ::Edit::AttributePair& attribute : groupElementData->m_attributes)
                    {
                        InstanceDataNode* readerNode = (isToggleGroup) ? groupSourceNode : node;
                        PropertyAttributeReader reader(readerNode->GetParent()->FirstInstance(), attribute.second);
                        QString descriptionOut;
                        bool foundDescription = false;
                        widgetEntry->ConsumeAttribute(attribute.first, reader, true, &descriptionOut, &foundDescription);
                        if (foundDescription)
                        {
                            widgetEntry->SetDescription(descriptionOut);
                        }
                    }

                    if (parent)
                    {
                        parent->AddedChild(widgetEntry);
                    }

                    m_widgetsInDisplayOrder.push_back(widgetEntry);
                }

                return widgetEntry;
            }
        }

        return nullptr;
    }

    size_t ReflectedPropertyEditor::Impl::CountRowsInAllDescendents(PropertyRowWidget* pParent)
    {
        // Recursively sum the number of property row widgets beneath the given parent
        auto& children = pParent->GetChildrenRows();
        size_t childCount = children.size();

        size_t result = childCount;

        for (size_t i = 0; i < childCount; ++i)
        {
            result += CountRowsInAllDescendents(children[i]);
        }

        return result;
    }
    
    bool IsParentAssociativeContainer(InstanceDataNode* node)
    {
        return node->GetParent() && node->GetParent()->GetClassMetadata()->m_container && node->GetParent()->GetClassMetadata()->m_container->GetAssociativeContainerInterface();
    }

    bool IsPairContainer(InstanceDataNode* node)
    {
        AZ::SerializeContext* context{};
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AZ::Uuid genericPairId("{9F3F5302-3390-407a-A6F7-2E011E3BB686}");
        auto genericClassInfo = context->FindGenericClassInfo(node->GetClassMetadata()->m_typeId);

        return (genericClassInfo && genericClassInfo->GetGenericTypeId() == genericPairId);
    }

    QSet<PropertyRowWidget*> ReflectedPropertyEditor::Impl::getTopLevelWidgets() const
    {
        QSet<PropertyRowWidget*> toplevelWidgets;
        for (const auto& widget : m_widgetsInDisplayOrder)
        {
            auto parent = widget;
            while (parent->GetParentRow())
            {
                parent = parent->GetParentRow();
            }
            toplevelWidgets.insert(parent);
        }
        return toplevelWidgets;
    }

    // sets the expansion state for every PropertyRowWidget starting from the top down
    void ReflectedPropertyEditor::Impl::UpdateExpansionState()
    {
        const auto widgetsToExpand = getTopLevelWidgets();
        for (const auto widget : widgetsToExpand)
        {
            // make sure the editing widget has been created for top-level widgets
            CreateEditorWidget(widget);

            // determine whether each top-level widget should expand and recursively update it's children
            const bool expand = ShouldRowAutoExpand(widget);
            widget->SetExpanded(expand);
            ExpandChildren(widget, expand, true);
        }

    }

    // creates and populates the GUI to edit the property if not already created
    void ReflectedPropertyEditor::Impl::CreateEditorWidget(PropertyRowWidget* pWidget)
    {
        if (!pWidget->HasChildWidgetAlready() && !pWidget->GetToggle())
        {
            PropertyHandlerBase* pHandler = pWidget->GetHandler();
            if (pHandler)
            {
                // create widget gui here.
                QWidget* newChildWidget = pHandler->CreateGUI(pWidget);
                if (newChildWidget)
                {
                    m_userWidgetsToData[newChildWidget] = pWidget->GetNode();
                    pHandler->ConsumeAttributes_Internal(newChildWidget, pWidget->GetNode());
                    pHandler->ReadValuesIntoGUI_Internal(newChildWidget, pWidget->GetNode());
                    pWidget->ConsumeChildWidget(newChildWidget);
                    pWidget->OnValuesUpdated();

                    if (!m_queuedTabOrderRefresh)
                    {
                        QTimer::singleShot(0, m_editor, SLOT(RecreateTabOrder()));
                    }
                    m_queuedTabOrderRefresh = true;
                }
            }
        }
    }

    // recursively goes through all children either collapsing or expanding them.
    // "checkIfChildrenShouldExpand" tells whether to test whether to autoexpand each child or force it to expand
    void ReflectedPropertyEditor::Impl::ExpandChildren(PropertyRowWidget* parentWidget, bool expand, bool checkIfChildrenShouldExpand)
    {
        for (auto childWidget : parentWidget->GetChildrenRows())
        {
            if (expand || parentWidget->ForceAutoExpand())
            {
                childWidget->show();
                CreateEditorWidget(childWidget);

                const bool expandChild = checkIfChildrenShouldExpand ? ShouldRowAutoExpand(childWidget) : !childWidget->IsForbidExpansion();
                childWidget->SetExpanded(expandChild);
                ExpandChildren(childWidget, expandChild, checkIfChildrenShouldExpand);
            }
            else
            {
                // contract all children
                ExpandChildren(childWidget, false, checkIfChildrenShouldExpand);
                childWidget->hide();
            }
        }
    }

    void ReflectedPropertyEditor::Impl::AddProperty(InstanceDataNode* node, PropertyRowWidget* pParent, int depth, AZStd::string_view labelOverride)
    {
        // Removal markers should not be displayed in the property grid.
        if (!node || node->IsRemovedVersusComparison())
        {
            return;
        }

        if (m_hiddenQueryFunction)
        {
            if (m_hiddenQueryFunction(node))
            {
                return;
            }
        }

        if (IsFilteredOut(node))
        {
            return;
        }

        // Evaluate editor reflection and visibility attributes for the node.
        NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*node);
        bool checkChildVisibility = visibility != NodeDisplayVisibility::NotVisible;

        if (m_visibilityCallback)
        {
            m_visibilityCallback(node, visibility, checkChildVisibility);
        }

        if (!checkChildVisibility)
        {
            return;
        }

        
        ReflectedPropertyEditorUpdateSentinel updateSentinel(m_editor, &m_editor->m_updateDepth);

        const bool isParentAssociativeContainer = IsParentAssociativeContainer(node);
        // Map our pair editor into a label (key) -> value entry
        // Only do this if we're a valid part of an associative container and the InstanceDataHierarchy figured out how to rasterize
        // our key entry into a string (which we can check by looking up whether it stored ElementInstances)
        const bool isAssociativeContainerPair = isParentAssociativeContainer &&
            IsPairContainer(node) &&
            node->FindAttribute(AZ::Edit::InternalAttributes::ElementInstances);
        
        PropertyRowWidget* pWidget = nullptr;
        if (visibility == NodeDisplayVisibility::Visible || visibility == NodeDisplayVisibility::HideChildren)
        {
            if (isParentAssociativeContainer && isAssociativeContainerPair && visibility == NodeDisplayVisibility::Visible)
            {
                // For associative containers, hide the pair node itself and only show the children
                visibility = NodeDisplayVisibility::ShowChildrenOnly;
            }
            else
            {
                auto widgetDisplayOrder = m_widgetsInDisplayOrder.end();

                // Handle anchoring to logical groups defined by ClassElement(AZ::Edit::ClassElements::Group,...).
                PropertyRowWidget* groupWidget = GetOrCreateLogicalGroupWidget(node, pParent, depth);
                if (groupWidget)
                {
                    groupWidget->show();
                    pParent = groupWidget;
                    depth = groupWidget->GetDepth() + 1;

                    // Insert this node's widget after all existing properties within the group.
                    for (auto iter = m_widgetsInDisplayOrder.begin(); iter != m_widgetsInDisplayOrder.end(); ++iter)
                    {
                        if (*iter == groupWidget)
                        {
                            widgetDisplayOrder = iter;

                            // We have to allow for containers in the group so we add the total number of descendant rows
                            AZStd::advance(widgetDisplayOrder, CountRowsInAllDescendents(groupWidget) + 1);
                        }
                    }
                }
                if (!node->GetElementEditMetadata() || (node->GetElementEditMetadata()->m_elementId != AZ::Edit::ClassElements::Group))
                {
                    pWidget = CreateOrPullFromPool();
                    pWidget->show();

                    pWidget->SetFilterString(m_editor->GetFilterString());
                    pWidget->Initialize(pParent, node, depth, m_propertyLabelWidth);

                    if (labelOverride != "")
                    {
                        pWidget->SetNameLabel(labelOverride.data());
                    }

                    pWidget->setObjectName(pWidget->label());
                    pWidget->SetSelectionEnabled(m_selectionEnabled);
                    pWidget->SetLeafIndentation(m_leafIndentation);
                    pWidget->SetTreeIndentation(m_treeIndentation);

                    m_widgets[node] = pWidget;
                    m_widgetsInDisplayOrder.insert(widgetDisplayOrder, pWidget);

                    if (pParent)
                    {
                        pParent->AddedChild(pWidget);
                    }

                    if (pParent || !m_hideRootProperties)
                    {
                        depth += 1;
                    }
                    pParent = pWidget;
                }

                // Save the last InstanceDataNode that is a Group ClassElement so that we can use it as the source node for its widget.
                if (node->GetElementEditMetadata() && (node->GetElementEditMetadata()->m_elementId == AZ::Edit::ClassElements::Group))
                {
                    groupSourceNode = node;
                }
            }
        }

        if (visibility != NodeDisplayVisibility::HideChildren)
        {
            auto& children = node->GetChildren();

            if (isParentAssociativeContainer && isAssociativeContainerPair)
            {
                // For pairs, show only the 2nd child and use the pair's display name (which should be the key string) as the label text

                AZ_Assert(children.size() == 2, "Pair must have only two children");
                const char* displayName = "";

                if (node->GetElementEditMetadata())
                {
                    displayName = node->GetElementEditMetadata()->m_name;
                }

                AddProperty(&children.back(), pParent, depth, displayName);
            }
            else
            {
                using WeightedNode = AZStd::pair<int, AzToolsFramework::InstanceDataNode*>;
                // Sort lowest display order first
                auto compareFunc = [](WeightedNode lhs, WeightedNode rhs) -> bool { return lhs.first < rhs.first; };
                AZStd::multiset<WeightedNode, decltype(compareFunc)> sortedChildrenNodes(compareFunc);

                // Build a list of children and their display order values
                for (auto& childNode : children)
                {
                    int childAppearanceOrder = 0;

                    auto* classData = childNode.GetClassMetadata();
                    auto* editorData = classData && classData->m_editData ? classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr;

                    if (editorData)
                    {
                        if (AZ::Edit::Attribute* attribute = editorData->FindAttribute(AZ::Edit::Attributes::DisplayOrder))
                        {
                            PropertyAttributeReader reader(childNode.GetInstance(0), attribute);
                            reader.Read<int>(childAppearanceOrder);
                        }
                    }

                    sortedChildrenNodes.insert({childAppearanceOrder, &childNode});
                }

                for (WeightedNode& pair : sortedChildrenNodes)
                {
                    AddProperty(pair.second, pParent, depth);
                }
            }
        }

        if (pWidget)
        {
            if (m_indicatorQueryFunction)
            {
                pWidget->UpdateIndicator(m_indicatorQueryFunction(node));
            }

            // Set this as a "Root" element so that our Qt stylesheet can set the labels on these rows to bold text
            pWidget->setProperty("Root", !pWidget->GetParentRow() && pWidget->HasChildRows());

            if (!pWidget->GetParentRow() && m_hideRootProperties && pWidget->HasChildRows())
                {
                pWidget->HideContent();
            }
        }
    }

    /// Must call after Add/Remove instance for the change to be applied
    void ReflectedPropertyEditor::InvalidateAll(const char* filter)
    {
        m_releasePrompt = true;

        setUpdatesEnabled(false);
        m_impl->m_selectedRow = nullptr;
        if (m_impl->m_ptrNotify && m_impl->m_selectedRow)
        {
            m_impl->m_ptrNotify->PropertySelectionChanged(GetNodeFromWidget(m_impl->m_selectedRow), false);
        }

        m_impl->ReturnAllToPool();

        m_impl->m_nodeFilteredOutState.clear();
        m_impl->m_hasFilteredOutNodes = false;

        for (auto& instance : m_impl->m_instances)
        {
            instance.Build(m_impl->m_context, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, m_impl->m_dynamicEditDataProvider, m_impl->m_editorParent);
            m_impl->FilterNode(instance.GetRootNode(), filter);
            m_impl->AddProperty(instance.GetRootNode(), nullptr, 0);
        }

        m_impl->UpdateExpansionState();

        for (PropertyRowWidget* widget : m_impl->m_widgetsInDisplayOrder)
        {
            widget->RefreshStyle();
            m_impl->m_containerWidget->layout()->addWidget(widget);
        }

        if (m_impl->m_mainScrollArea)
        {
            // if we're responsible for our own scrolling, then add the spacer.
            if (!m_impl->m_spacer)
            {
                m_impl->m_spacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            }
            else
            {
                m_impl->m_containerWidget->layout()->removeItem(m_impl->m_spacer);
            }

            m_impl->m_containerWidget->layout()->addItem(m_impl->m_spacer);
        }

        layout()->setEnabled(true);
        layout()->update();
        layout()->activate();
        emit OnExpansionContractionDone();

        // Active property editors should all support transient state saving for the current session, at a minimum.
        // A key must still be manually provided for persistent saving across sessions.
        if (0 == m_impl->m_savedStateKey)
        {
            if (!m_impl->m_instances.empty() && m_impl->m_instances.begin()->GetRootNode() && m_impl->m_instances.begin()->GetRootNode()->GetClassMetadata())
            {
                // Based on instance type name; persists when editing any object of this type.
                SetSavedStateKey(AZ::Crc32(m_impl->m_instances.begin()->GetRootNode()->GetClassMetadata()->m_name));
            }
            else
            {
                // Random key; valid for lifetime of control instance.
                SetSavedStateKey(AZ::Sfmt().Rand32());
            }
        }

        m_impl->SaveExpansion();

        m_impl->AdjustLabelWidth();

        setUpdatesEnabled(true);
    }

    void ReflectedPropertyEditor::SetFilterString(AZStd::string str)
    {
        m_currentFilterString = str;
    }

    AZStd::string ReflectedPropertyEditor::GetFilterString()
    {
        return m_currentFilterString;
    }

    bool ReflectedPropertyEditor::Impl::FilterNode(InstanceDataNode* node, const char* filter)
    {
        bool isFilterMatch = true;

        if (node)
        {
            const NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*node, false);

            if (filter)
            {
                isFilterMatch = NodeMatchesFilter(*node, filter) && visibility == NodeDisplayVisibility::Visible;

                if (!isFilterMatch && visibility == NodeDisplayVisibility::Visible)
                {
                    isFilterMatch = NodeGroupMatchesFilter(*node, filter);
                }

                if (!isFilterMatch)
                {
                    for (auto& childNode : node->GetChildren())
                    {
                        isFilterMatch |= FilterNode(&childNode, filter);
                    }

                    if (!isFilterMatch && visibility == NodeDisplayVisibility::Visible)
                    {
                        m_hasFilteredOutNodes = true;
                    }

                }
            }

            m_nodeFilteredOutState[node] = !isFilterMatch;
        }

        return isFilterMatch;
    }

    bool ReflectedPropertyEditor::Impl::IsFilteredOut(InstanceDataNode* node)
    {
        auto hiddenItr = m_nodeFilteredOutState.find(node);
        return hiddenItr != m_nodeFilteredOutState.end() && hiddenItr->second;
    }

    void ReflectedPropertyEditor::Impl::AdjustLabelWidth()
    {
        if (m_autoResizeLabels)
        {
            m_editor->SetLabelWidth(AZ::GetMax(m_propertyLabelAutoResizeMinimumWidth, m_editor->GetMaxLabelWidth() + 10));
        }
    }

    void ReflectedPropertyEditor::InvalidateAttributesAndValues()
    {
        m_releasePrompt = true;

        for (InstanceDataHierarchy& instance : m_impl->m_instances)
        {
            instance.RefreshComparisonData(AZ::SerializeContext::ENUM_ACCESS_FOR_READ, m_impl->m_dynamicEditDataProvider);
        }

        for (auto it = m_impl->m_widgets.begin(); it != m_impl->m_widgets.end(); ++it)
        {
            PropertyRowWidget* pWidget = it->second;

            QWidget* childWidget = pWidget->GetChildWidget();

            if (pWidget->GetHandler() && childWidget)
            {
                pWidget->GetHandler()->ConsumeAttributes_Internal(childWidget, it->first);
                pWidget->GetHandler()->ReadValuesIntoGUI_Internal(childWidget, it->first);
                pWidget->OnValuesUpdated();
            }
            pWidget->RefreshAttributesFromNode(false);

            if (m_impl->m_indicatorQueryFunction)
            {
                pWidget->UpdateIndicator(m_impl->m_indicatorQueryFunction(pWidget->GetNode()));
            }
        }
    }

    void ReflectedPropertyEditor::InvalidateValues()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_releasePrompt = true;

        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "ReflectedPropertyEditor::InvalidateValues:InstancesRefreshDataCompare");
            for (InstanceDataHierarchy& instance : m_impl->m_instances)
            {
                const bool dataIdentical = instance.RefreshComparisonData(
                    AZ::SerializeContext::ENUM_ACCESS_FOR_READ, m_impl->m_dynamicEditDataProvider);

                if (m_impl->m_editorParent)
                {
                    m_impl->m_editorParent->SetComponentOverridden(!dataIdentical);
                }
            }
        }

        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "ReflectedPropertyEditor::InvalidateValues:RowWidgetGuiUpdate");
            for (auto it = m_impl->m_userWidgetsToData.begin(); it != m_impl->m_userWidgetsToData.end(); ++it)
            {
                auto rowWidget = m_impl->m_widgets.find(it->second);

                if (rowWidget != m_impl->m_widgets.end())
                {
                    PropertyRowWidget* pWidget = rowWidget->second;
                    if (pWidget->GetHandler())
                    {
                        pWidget->GetHandler()->ReadValuesIntoGUI_Internal(it->first, rowWidget->first);
                        pWidget->OnValuesUpdated();
                    }
                }
            }
        }
    }

    PropertyRowWidget* ReflectedPropertyEditor::Impl::CreateOrPullFromPool()
    {
        PropertyRowWidget* newWidget = nullptr;
        if (m_widgetPool.empty())
        {
            newWidget = aznew PropertyRowWidget(m_containerWidget);
            QObject::connect(newWidget, &PropertyRowWidget::onRequestedContainerClear, m_editor,
                [=](InstanceDataNode* node)
            {
                m_editor->OnPropertyRowRequestClear(newWidget, node);
            }
            );
            QObject::connect(newWidget, &PropertyRowWidget::onRequestedContainerElementRemove, m_editor,
                [=](InstanceDataNode* node)
            {
                m_editor->OnPropertyRowRequestContainerRemoveItem(newWidget, node);
            }
            );

            QObject::connect(newWidget, &PropertyRowWidget::onRequestedContainerAdd, m_editor,
                [=](InstanceDataNode* node)
            {
                m_editor->OnPropertyRowRequestContainerAddItem(newWidget, node);
            }
            );

            QObject::connect(newWidget, &PropertyRowWidget::onUserExpandedOrContracted, m_editor,
                [=](InstanceDataNode* node, bool expanded)
            {
                m_editor->OnPropertyRowExpandedOrContracted(newWidget, node, expanded, true);
            }
            );

            QObject::connect(newWidget, &PropertyRowWidget::onRequestedContextMenu, m_editor,
                [=](InstanceDataNode* node, const QPoint& point)
            {
                if (m_ptrNotify)
                {
                    PropertyRowWidget* widget = m_editor->GetWidgetFromNode(node);
                    if (widget)
                    {
                        m_ptrNotify->RequestPropertyContextMenu(node, point);
                    }
                }
            }
            );

            QObject::connect(newWidget, &PropertyRowWidget::onRequestedSelection, m_editor, &ReflectedPropertyEditor::SelectInstance);

            newWidget->SetReadOnlyQueryFunction(m_readOnlyQueryFunction);
        }
        else
        {
            newWidget = m_widgetPool.back();
            m_widgetPool.erase(m_widgetPool.begin() + m_widgetPool.size() - 1);
        }
        return newWidget;
    }

    void ReflectedPropertyEditor::Impl::ReturnAllToPool()
    {
        m_editor->layout()->setEnabled(false);
        for (auto& widget : m_widgets)
        {
            widget.second->hide();
            widget.second->Clear();
            m_containerWidget->layout()->removeWidget(widget.second);
            m_widgetPool.push_back(widget.second);
        }

        for (auto& widget : m_groupWidgets)
        {
            widget.second->hide();
            widget.second->Clear();
            m_containerWidget->layout()->removeWidget(widget.second);
            m_widgetPool.push_back(widget.second);
        }

        m_userWidgetsToData.clear();
        m_widgets.clear();
        m_widgetsInDisplayOrder.clear();
        m_groupWidgets.clear();
    }

    void ReflectedPropertyEditor::OnPropertyRowExpandedOrContracted(PropertyRowWidget* widget, InstanceDataNode* /*node*/, bool expanded, bool fromUserInteraction)
    {
        ReflectedPropertyEditorUpdateSentinel updateSentinel(this, &m_updateDepth);
        layout()->setEnabled(false);

        // Create a saved state if a user interaction occurred and there is no existing saved state and we have a saved state key to use
        if (m_impl->m_savedState && fromUserInteraction)
        {
            m_impl->m_savedState->SetExpandedState(m_impl->CreatePathKey(widget), expanded);
        }

        m_impl->ExpandChildren(widget, expanded, true);

        layout()->setEnabled(true);
        layout()->update();
        layout()->activate();
        emit OnExpansionContractionDone();

        m_impl->AdjustLabelWidth();
    }

    void ReflectedPropertyEditor::RecreateTabOrder()
    {
        // re-create the tab order, based on vertical position in the list.

        QWidget* pLastWidget = nullptr;

        for (AZStd::size_t pos = 0; pos < m_impl->m_widgetsInDisplayOrder.size(); ++pos)
        {
            if (pLastWidget)
            {
                QWidget* pFirst = m_impl->m_widgetsInDisplayOrder[pos]->GetFirstTabWidget();
                setTabOrder(pLastWidget, pFirst);
                m_impl->m_widgetsInDisplayOrder[pos]->UpdateWidgetInternalTabbing();
            }

            pLastWidget = m_impl->m_widgetsInDisplayOrder[pos]->GetLastTabWidget();
        }
        m_impl->m_queuedTabOrderRefresh = false;
    }

    void ReflectedPropertyEditor::SetSavedStateKey(AZ::u32 key)
    {
        if (m_impl->m_savedStateKey != key)
        {
            m_impl->m_savedStateKey = key;
            m_impl->m_savedState = key ? AZ::UserSettings::CreateFind<ReflectedPropertyEditorState>(m_impl->m_savedStateKey, AZ::UserSettings::CT_GLOBAL) : nullptr;
        }
    }

    bool ReflectedPropertyEditor::Impl::CheckSavedExpandState(AZ::u32 pathKey) const
    {
        return m_savedState ? m_savedState->GetExpandedState(pathKey) : false;
    }

    bool ReflectedPropertyEditor::Impl::HasSavedExpandState(AZ::u32 pathKey) const
    {
        // We are only considered to have a saved expanded state if we are a saved element
        return m_savedState ? m_savedState->HasExpandedState(pathKey) : false;
    }

    // given a widget, create a key which includes its parent(s)
    AZ::u32 ReflectedPropertyEditor::Impl::CreatePathKey(PropertyRowWidget* widget) const
    {
        if (widget)
        {
            AZ::Crc32 crc = AZ::Crc32(widget->GetIdentifier());

            auto parentWidget = widget->GetParentRow();
            if (parentWidget != nullptr)
            {
                AZ::u32 parentCRC = CreatePathKey(parentWidget);
                crc.Add(&parentCRC, sizeof(AZ::u32), false);
            }
            return (AZ::u32)crc;
        }

        return 0;
    }

    void ReflectedPropertyEditor::Impl::SaveExpansion()
    {
        if (m_savedState)
        {
            for (const auto& widgetPair : m_widgets )
            {
                auto widget = widgetPair.second;
                m_savedState->SetExpandedState(CreatePathKey(widget), widget->IsExpanded());
            }
        }
    }

    bool ReflectedPropertyEditor::Impl::ShouldRowAutoExpand(PropertyRowWidget* widget) const
    {
        auto parent = widget->GetParentRow();
        if (!parent && m_hideRootProperties)
        {
            return true;
        }

        if (widget->IsForbidExpansion())
        {
            return false;
        }
        else if (widget->ForceAutoExpand())
        {
            return true;
        }

        const auto& key = CreatePathKey(widget);
        if (HasSavedExpandState(key))
        {
            return CheckSavedExpandState(key);
        }

        return widget->AutoExpand();
    }

    AZStd::set<void*> ReflectedPropertyEditor::Impl::CreateInstanceSet()
    {
        AZStd::set<void*> instanceSet;
        for (auto& dataInstance : m_instances)
        {
            for (size_t idx = 0; idx < dataInstance.GetNumInstances(); ++idx)
            {
                instanceSet.insert(dataInstance.GetInstance(idx));
            }
        }

        return instanceSet;
    }

    bool ReflectedPropertyEditor::Impl::Intersects(const AZStd::set<void*>& cachedInstanceSet)
    {
        for (auto& dataInstance : m_instances)
        {
            for (size_t idx = 0; idx < dataInstance.GetNumInstances(); ++idx)
            {
                auto it = cachedInstanceSet.find(dataInstance.GetInstance(idx));
                if (it != cachedInstanceSet.end())
                {
                    return true;
                }
            }
        }

        return false;
    }

    void ReflectedPropertyEditor::Impl::QueueInvalidationIfSharedData(InternalReflectedPropertyEditorEvents* sender, PropertyModificationRefreshLevel level, const AZStd::set<void*>& sourceInstanceSet)
    {
        if (sender != this)
        {
            const bool needToQueueRefresh = (level > m_queuedRefreshLevel);
            if (!needToQueueRefresh)
            {
                return;
            }

            bool intersectingInstances = Intersects(sourceInstanceSet);
            if (!intersectingInstances)
            {
                return;
            }
        }

        m_editor->QueueInvalidation(level);
    }

    void ReflectedPropertyEditor::Impl::QueueInvalidationForAllMatchingReflectedPropertyEditors(PropertyModificationRefreshLevel level)
    {
        const bool needToQueueRefreshForThis = (level > m_queuedRefreshLevel);
        if (!needToQueueRefreshForThis)
        {
            return;
        }

        AZStd::set<void*> instanceSet = CreateInstanceSet();

        InternalReflectedPropertyEditorEvents* sender = this;
        InternalReflectedPropertyEditorEvents::Bus::Broadcast(&InternalReflectedPropertyEditorEvents::QueueInvalidationIfSharedData, sender, level, instanceSet);
    }

    // calculate the index/offset of the instance data node in the container
    // (useful for notifying which element in a vector was modified/removed)
    static int CalculateElementIndexInContainer(
        InstanceDataNode* node, void* parentInstanceNode,
        AZ::SerializeContext::IDataContainer* container, AZStd::vector<void*>& nodeInstancesOut)
    {
        // get the node's instance list, giving the ElementInstances attribute for containers priority over the raw instance list
        if (!node->ReadAttribute(AZ::Edit::InternalAttributes::ElementInstances, nodeInstancesOut))
        {
            for (size_t i = 0, instanceCount = node->GetNumInstances(); i < instanceCount; ++i)
            {
                nodeInstancesOut.push_back((node->GetElementMetadata()->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                    ? node->GetInstanceAddress(i)
                    : node->GetInstance(i));
            }
        }

        int elementIndex = 0;
        void* elementPtr = nodeInstancesOut.empty() ? nullptr : nodeInstancesOut.front();

        // find the index of the element we are about to remove
        container->EnumElements(parentInstanceNode, [&elementIndex, elementPtr](
            void* instancePointer, const AZ::Uuid& /*elementClassId*/,
            const AZ::SerializeContext::ClassData* /*elementGenericClassData*/,
            const AZ::SerializeContext::ClassElement* /*genericClassElement*/)
        {
            if (instancePointer == elementPtr)
            {
                return false;
            }

            elementIndex++;
            return true;
        });

        return elementIndex;
    }

    void ReflectedPropertyEditor::Impl::RequestWrite(QWidget* editorGUI)
    {
        auto it = m_userWidgetsToData.find(editorGUI);
        if (it == m_userWidgetsToData.end())
        {
            return;
        }

        // Get the property editor from either the widget map or the special toggle group widgets
        auto rowWidget = m_widgets.find(it->second);
        if (rowWidget == m_widgets.end())
        {
            rowWidget = m_specialGroupWidgets.find(it->second);
        }
        if (rowWidget != m_widgets.end() || rowWidget != m_specialGroupWidgets.end())
        {
            InstanceDataNode* node = rowWidget->first;
            PropertyRowWidget* widget = rowWidget->second;
            PropertyHandlerBase* handler = widget->GetHandler();
            if (handler)
            {
                if (rowWidget->second->ShouldPreValidatePropertyChange())
                {
                    void* tempValue = rowWidget->first->GetClassMetadata()->m_factory->Create("Validate Attribute");

                    handler->WriteGUIValuesIntoTempProperty_Internal(editorGUI, tempValue, rowWidget->first->GetClassMetadata()->m_typeId, rowWidget->first->GetSerializeContext());

                    bool validated = rowWidget->second->ValidatePropertyChange(tempValue, rowWidget->first->GetClassMetadata()->m_typeId);

                    rowWidget->first->GetClassMetadata()->m_factory->Destroy(tempValue);

                    // Validate the change to make sure everything is okay before actually modifying the value on anything
                    if (!validated)
                    {
                        // Force the values to update so that they are correct since something just declined changes and
                        // we want the UI to display the current values and not the invalid ones
                        m_editor->QueueInvalidation(Refresh_Values);
                        return;
                    }
                }

                if (m_ptrNotify)
                {
                    m_ptrNotify->BeforePropertyModified(node);
                }

                handler->WriteGUIValuesIntoProperty_Internal(editorGUI, node);

                // if the element being modified exists in a container, calculate
                // the index to be passed through to PropertyNotify
                const auto calculateElementIndex = [](InstanceDataNode* node) -> int {
                    if (InstanceDataNode* parent = node->GetParent())
                    {
                        if (AZ::SerializeContext::IDataContainer* container = parent->GetClassMetadata()->m_container)
                        {
                            AZStd::vector<void*> nodeInstancesOut;
                            return CalculateElementIndexInContainer(
                                node, parent->GetInstance(0), container, nodeInstancesOut);
                        }
                    }

                    return 0;
                };

                // once we've written our values, we need to potentially callback:
                const PropertyModificationRefreshLevel level = widget->DoPropertyNotify(calculateElementIndex(node));

                if (m_ptrNotify)
                {
                    m_ptrNotify->AfterPropertyModified(node);
                }

                if (level < Refresh_Values)
                {
                    for (InstanceDataHierarchy& instance : m_instances)
                    {
                        bool dataIdentical = instance.RefreshComparisonData(AZ::SerializeContext::ENUM_ACCESS_FOR_READ, m_dynamicEditDataProvider);
                        widget->OnValuesUpdated();

                        if (m_editorParent)
                        {
                            m_editorParent->SetComponentOverridden(!dataIdentical);
                        }
                    }
                }

                QueueInvalidationForAllMatchingReflectedPropertyEditors(level);
            }
        }
    }

    void ReflectedPropertyEditor::Impl::AddElementsToParentContainer(QWidget* editorGUI, size_t numElements, const InstanceDataNode::FillDataClassCallback& fillDataCallback)
    {
        if (numElements == 0 || !editorGUI)
        {
            return;
        }

        auto iterUserWidgetToData = m_userWidgetsToData.find(editorGUI);
        if (iterUserWidgetToData == m_userWidgetsToData.end())
        {
            return;
        }

        // get the property editor
        auto rowWidget = m_widgets.find(iterUserWidgetToData->second);
        if (rowWidget != m_widgets.end())
        {
            InstanceDataNode* myInstanceDataNode = rowWidget->first;
            if (myInstanceDataNode)
            {
                InstanceDataNode* parentInstanceDataNode = myInstanceDataNode->GetParent();
                if (parentInstanceDataNode)
                {
                    if (parentInstanceDataNode->GetClassMetadata() && parentInstanceDataNode->GetClassMetadata()->m_container)
                    {
                        for (size_t index = 0; index < numElements; index++)
                        {
                            parentInstanceDataNode->CreateContainerElement(CreateContainerElementSelectClassCallback, fillDataCallback);
                        }

                        m_editor->QueueInvalidation(Refresh_EntireTree);
                    }
                }
            }
        }
    }


    void ReflectedPropertyEditor::Impl::RequestRefresh(PropertyModificationRefreshLevel level)
    {
        m_editor->QueueInvalidation(level);
    }

    void ReflectedPropertyEditor::Impl::RequestPropertyNotify(QWidget* editorGUI)
    {
        auto it = m_userWidgetsToData.find(editorGUI);
        if (it == m_userWidgetsToData.end())
        {
            return;
        }

        auto rowWidget = m_widgets.find(it->second);
        if (rowWidget != m_widgets.end())
        {
            // once we've written our values, we need to potentially callback:
            PropertyModificationRefreshLevel level = rowWidget->second->DoPropertyNotify();

            if (level < Refresh_Values)
            {
                for (InstanceDataHierarchy& instance : m_instances)
                {
                    instance.RefreshComparisonData(AZ::SerializeContext::ENUM_ACCESS_FOR_READ, m_dynamicEditDataProvider);
                    rowWidget->second->OnValuesUpdated();
                }
            }

            m_editor->QueueInvalidation(level);
        }
    }

    void ReflectedPropertyEditor::Impl::OnEditingFinished(QWidget* editorGUI)
    {
        auto it = m_userWidgetsToData.find(editorGUI);
        if (it == m_userWidgetsToData.end())
        {
            return;
        }

        // get the property editor
        auto rowWidget = m_widgets.find(it->second);
        if (rowWidget != m_widgets.end())
        {
            rowWidget->second->DoEditingCompleteNotify();

            PropertyHandlerBase* handler = rowWidget->second->GetHandler();
            if (handler)
            {
                if (m_ptrNotify)
                {
                    m_ptrNotify->SetPropertyEditingComplete(rowWidget->first);
                }
            }
        }
    }

    void ReflectedPropertyEditor::OnPropertyRowRequestClear(PropertyRowWidget* widget, InstanceDataNode* node)
    {
        AZ::SerializeContext::IDataContainer* container = node->GetClassMetadata()->m_container;
        AZ_Assert(container->IsFixedSize() == false || container->IsSmartPointer(), "Attempted to clear elements in a static container");

        bool isContainerEmpty = false;
        for (size_t i = 0; !isContainerEmpty && i < node->GetNumInstances(); ++i)
        {
            isContainerEmpty = container->Size(node->GetInstance(i)) == 0;
        }

        // Bail out if the user aborts (or the container is empty, which is by definition a no-op)
        if (isContainerEmpty ||
            QMessageBox::question(this,
            QStringLiteral("Clear container?"),
            QStringLiteral("Are you sure you want to remove all elements from this container?")) == QMessageBox::No)
        {
            return;
        }

        // get the property editor
        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->BeforePropertyModified(node);
        }

        // make space for number of elements stored by each instance
        AZStd::vector<size_t> instanceElements(node->GetNumInstances());
        for (size_t instanceIndex = 0; instanceIndex < node->GetNumInstances(); ++instanceIndex)
        {
            // record how many elements were in each instance
            instanceElements[instanceIndex] = container->Size(node->GetInstance(instanceIndex));
            // clear all elements
            container->ClearElements(node->GetInstance(instanceIndex), node->GetSerializeContext());
        }

        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->AfterPropertyModified(node);
            m_impl->m_ptrNotify->SealUndoStack();
        }

        // fire remove notification for each element removed
        for (const AZ::Edit::AttributePair& attribute : node->GetElementEditMetadata()->m_attributes)
        {
            if (attribute.first == AZ::Edit::Attributes::RemoveNotify)
            {
                if (InstanceDataNode* pParent = node->GetParent())
                {
                    if (auto funcVoid = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second))
                    {
                        for (size_t instanceIndex = 0; instanceIndex < pParent->GetNumInstances(); ++instanceIndex)
                        {
                            // remove all elements in reverse order
                            for (AZ::s64 elementIndex = instanceElements[instanceIndex] - 1; elementIndex >= 0 ; --elementIndex)
                            {
                                // remove callback (without element index)
                                funcVoid->Invoke(pParent->GetInstance(instanceIndex));
                            }
                        }
                    }
                    else if (auto funcIndex = azdynamic_cast<AZ::Edit::AttributeFunction<void(size_t)>*>(attribute.second))
                    {
                        for (size_t instanceIndex = 0; instanceIndex < pParent->GetNumInstances(); ++instanceIndex)
                        {
                            // remove all elements in reverse order
                            for (AZ::s64 elementIndex = instanceElements[instanceIndex] - 1; elementIndex >= 0; --elementIndex)
                            {
                                // remove callback (with element index)
                                size_t tempElementIndex = elementIndex;
                                funcIndex->Invoke(pParent->GetInstance(instanceIndex), AZStd::move(tempElementIndex));
                            }
                        }
                    }
                }
            }
        }

        // Fire general change notifications for the container widget.
        if (widget)
        {
            widget->DoPropertyNotify();
        }

        // Need to refresh any pinned inspectors as well to keep the container state in sync
        QueueInvalidation(Refresh_EntireTree);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    InstanceDataNode* ReflectedPropertyEditor::FindContainerNodeForNode(InstanceDataNode* node) const
    {
        // Locate the owning container. There may be a level of indirection due to wrappers, such as DynamicSerializableField.
        InstanceDataNode* pContainerNode = node->GetParent();
        if (!pContainerNode)
        {
            return nullptr;
        }

        while (pContainerNode && !pContainerNode->GetClassMetadata()->m_container)
        {
            pContainerNode = pContainerNode->GetParent();
            node = node->GetParent();
        }

        // Check for pContainerNode again, can happen if a node is deleted during operation.
        if (!pContainerNode)
        {
            return nullptr;
        }

        if (IsParentAssociativeContainer(pContainerNode) && IsPairContainer(pContainerNode))
        {
            // Go up one more level to the associative container, we'll remove the pair from that container
            pContainerNode = pContainerNode->GetParent();
            node = node->GetParent();
        }

        AZ_Assert(
            pContainerNode, "Failed to locate parent container for element \"%s\" of type %s.",
            node->GetElementMetadata() ? node->GetElementMetadata()->m_name : node->GetClassMetadata()->m_name,
            node->GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str());

        return pContainerNode;
    }

    InstanceDataNode* ReflectedPropertyEditor::GetNodeAtIndex(int index)
    {
        if (index >= m_impl->m_widgetsInDisplayOrder.size())
        {
            return nullptr;
        }

        return GetNodeFromWidget(m_impl->m_widgetsInDisplayOrder[index]);
    }

    QSet<PropertyRowWidget*> ReflectedPropertyEditor::GetTopLevelWidgets()
    {
        return m_impl->getTopLevelWidgets();
    }

    void ReflectedPropertyEditor::ChangeNodeIndex(InstanceDataNode* containerNode, InstanceDataNode* node, int fromIndex, int toIndex)
    {
        auto container = containerNode->GetElementMetadata()
            ? containerNode->GetElementMetadata()->m_genericClassInfo->GetClassData()->m_container
            : nullptr;

        if (fromIndex == toIndex)
        {
            return;
        }

        if (!container || container->GetAssociativeContainerInterface())
        {
            return;
        }

        AZ::Uuid typeId = node->GetClassMetadata()->m_typeId;

        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->BeforePropertyModified(containerNode);
        }

        const AZ::SerializeContext::ClassElement* containerClassElement = container->GetElement(container->GetDefaultElementNameCrc());

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // Backup the item we're moving.
        void* srcElement = nullptr;
        void* destElement = nullptr;

        int destIndex = -1;
        int srcIndex = fromIndex;

        srcElement = container->GetElementByIndex(containerNode->GetInstance(0), containerClassElement, srcIndex);

        void* tmpBuffer = serializeContext->CloneObject(srcElement, typeId);

        // Shuffle all intervening items up (or down).
        int indexOffset = (toIndex < fromIndex) ? -1 : 1;

        while (destIndex != toIndex - indexOffset)
        {
            destIndex = srcIndex;
            srcIndex += indexOffset;

            destElement = srcElement;

            srcElement = container->GetElementByIndex(containerNode->GetInstance(0), containerClassElement, srcIndex);

            serializeContext->CloneObjectInplace(destElement, srcElement, typeId);
        }

        // Now replace the final element with the one backed up previously.
        destElement = srcElement;

        serializeContext->CloneObjectInplace(destElement, tmpBuffer, typeId);

        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->AfterPropertyModified(containerNode);
            m_impl->m_ptrNotify->SealUndoStack();
        }

        // Need to refresh any pinned inspectors as well to keep the container state in sync
        QueueInvalidation(Refresh_Values);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }

    void ReflectedPropertyEditor::MoveNodeToIndex(InstanceDataNode* node, int index)
    {
        InstanceDataNode* pContainerNode = FindContainerNodeForNode(node);

        if (!pContainerNode)
        {
            return;
        }

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;

        AZStd::vector<void*> nodeInstancesOut;
        const int elementIndex = CalculateElementIndexInContainer(node, pContainerNode->GetInstance(0), container, nodeInstancesOut);

        ChangeNodeIndex(pContainerNode, node, elementIndex, index);
    }

    void ReflectedPropertyEditor::MoveNodeBefore(InstanceDataNode* nodeToMove, InstanceDataNode* nodeToMoveBefore)
    {
        InstanceDataNode* pContainerNode = FindContainerNodeForNode(nodeToMove);
        InstanceDataNode* pContainerNodeTarget = FindContainerNodeForNode(nodeToMoveBefore);

        if (nodeToMove == nodeToMoveBefore)
        {
            return;
        }

        // Can only move nodes within the same parent.
        if (pContainerNode != pContainerNodeTarget)
        {
            return;
        }

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;

        AZStd::vector<void*> nodeInstancesOut;
        int elementIndex = CalculateElementIndexInContainer(nodeToMove, pContainerNode->GetInstance(0), container, nodeInstancesOut);
        nodeInstancesOut.clear();
        int elementIndexTarget =
            CalculateElementIndexInContainer(nodeToMoveBefore, pContainerNode->GetInstance(0), container, nodeInstancesOut);

        if (elementIndex < elementIndexTarget)
        {
            elementIndexTarget -= 1;
        }

        ChangeNodeIndex(pContainerNode, nodeToMove, elementIndex, elementIndexTarget);
    }

    void ReflectedPropertyEditor::MoveNodeAfter(InstanceDataNode* nodeToMove, InstanceDataNode* nodeToMoveBefore)
    {
        InstanceDataNode* pContainerNode = FindContainerNodeForNode(nodeToMove);
        InstanceDataNode* pContainerNodeTarget = FindContainerNodeForNode(nodeToMoveBefore);

        if (nodeToMove == nodeToMoveBefore)
        {
            return;
        }

        // Can only move nodes within the same parent.
        if (pContainerNode != pContainerNodeTarget)
        {
            return;
        }

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;

        AZStd::vector<void*> nodeInstancesOut;
        int elementIndex = CalculateElementIndexInContainer(nodeToMove, pContainerNode->GetInstance(0), container, nodeInstancesOut);
        nodeInstancesOut.clear();
        int elementIndexTarget =
            CalculateElementIndexInContainer(nodeToMoveBefore, pContainerNode->GetInstance(0), container, nodeInstancesOut);

        if (elementIndex > elementIndexTarget)
        {
            elementIndexTarget += 1;
        }

        ChangeNodeIndex(pContainerNode, nodeToMove, elementIndex, elementIndexTarget);
    }

    int ReflectedPropertyEditor::GetNodeIndexInContainer(InstanceDataNode* node)
    {
        InstanceDataNode* pContainerNode = FindContainerNodeForNode(node);

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;

        AZStd::vector<void*> nodeInstancesOut;
        int elementIndex = CalculateElementIndexInContainer(node, pContainerNode->GetInstance(0), container, nodeInstancesOut);

        return elementIndex;
    }

    void ReflectedPropertyEditor::OnPropertyRowRequestContainerRemoveItem(PropertyRowWidget* widget, InstanceDataNode* node)
    {
        // Locate the owning container. There may be a level of indirection due to wrappers, such as DynamicSerializableField.
        InstanceDataNode* pContainerNode = node->GetParent();
        while (pContainerNode && !pContainerNode->GetClassMetadata()->m_container)
        {
            pContainerNode = pContainerNode->GetParent();
            node = node->GetParent();
        }

        // Check if the container is actually a PairContainer that belongs to an associative container
        if (IsParentAssociativeContainer(pContainerNode) && IsPairContainer(pContainerNode))
        {
            // Go up one more level to the associative container, we'll remove the pair from that container
            pContainerNode = pContainerNode->GetParent();
            node = node->GetParent();
        }

        AZ_Assert(pContainerNode, "Failed to locate parent container for element \"%s\" of type %s.",
            node->GetElementMetadata() ? node->GetElementMetadata()->m_name : node->GetClassMetadata()->m_name,
            node->GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str());

        // remove it from all containers:
        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->BeforePropertyModified(pContainerNode);
        }

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;
        AZ_Assert(container->IsFixedSize() == false ||
            container->IsSmartPointer(), "We can't remove elements from a fixed size container!");

        // the index of the element being removed
        AZStd::vector<void*> nodeInstancesOut;
        const int elementIndex = CalculateElementIndexInContainer(
            node, pContainerNode->GetInstance(0), container, nodeInstancesOut);

        // pass the context as the last parameter to actually delete the related data.
        for (AZStd::size_t instanceIndex = 0; instanceIndex < pContainerNode->GetNumInstances(); ++instanceIndex)
        {
            void* elementPtr = nodeInstancesOut[instanceIndex];
            [[maybe_unused]] bool removed =
                container->RemoveElement(pContainerNode->GetInstance(instanceIndex), elementPtr, pContainerNode->GetSerializeContext());
            AZ_Assert(removed, "Failed to remove element \"%s\" of type %s from instance %d.",
                node->GetElementMetadata() ? node->GetElementMetadata()->m_name : node->GetClassMetadata()->m_name,
                node->GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str(),
                instanceIndex);
        }


        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->AfterPropertyModified(pContainerNode);
            m_impl->m_ptrNotify->SealUndoStack();
        }

        for (const AZ::Edit::AttributePair& attribute : pContainerNode->GetElementEditMetadata()->m_attributes)
        {
            if (attribute.first == AZ::Edit::Attributes::RemoveNotify)
            {
                if (InstanceDataNode* pParent = pContainerNode->GetParent())
                {
                    if (auto funcVoid = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second))
                    {
                        for (size_t instanceIndex = 0; instanceIndex < pParent->GetNumInstances(); ++instanceIndex)
                        {
                            funcVoid->Invoke(pParent->GetInstance(instanceIndex));
                        }
                    }
                    else if (auto funcIndex = azdynamic_cast<AZ::Edit::AttributeFunction<void(size_t)>*>(attribute.second))
                    {
                        for (size_t instanceIndex = 0; instanceIndex < pParent->GetNumInstances(); ++instanceIndex)
                        {
                            size_t tempElementIndex = elementIndex;
                            funcIndex->Invoke(pParent->GetInstance(instanceIndex), AZStd::move(tempElementIndex));
                        }
                    }
                }
            }
        }

        // Fire general change notifications for the container widget.
        if (widget)
        {
            widget->DoPropertyNotify();
        }

        // Need to refresh any pinned inspectors as well to keep the container state in sync
        QueueInvalidation(Refresh_EntireTree);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
    }

    // Helper functions to populate a dataPtr with a default constructed type, if it's in a list of supported, default-constructable types.
    namespace ReflectedPropertyEditorHelper
    {
        template <class TDataType>
        bool HandleDefaultValue(void* dataPtr, const AZ::Uuid& typeId)
        {
            if (typeId == azrtti_typeid<TDataType>() || typeId == azrtti_typeid<AZ::Internal::RValueToLValueWrapper<TDataType>>())
            {
                *reinterpret_cast<TDataType*>(dataPtr) = {};
                return true;
            }
            return false;
        }

        template <class TDataType, class TOtherDataType, class... TRemainingDataTypes>
        bool HandleDefaultValue(void* dataPtr, const AZ::Uuid& typeId)
        {
            return HandleDefaultValue<TDataType>(dataPtr, typeId) || HandleDefaultValue<TOtherDataType, TRemainingDataTypes...>(dataPtr, typeId);
        }

        // In the case of primitive numbers, initialize to 0.
        bool HandleDefaultNumericValues(void* dataPtr, const AZ::Uuid& typeId)
        {
            return HandleDefaultValue<
                double,
                float,
                AZ::u8,
                char,
                AZ::u16,
                AZ::s16,
                AZ::u32,
                AZ::s32,
                AZ::u64,
                AZ::s64,
                bool
            >(dataPtr, typeId);
        }
    }

    void ReflectedPropertyEditor::OnPropertyRowRequestContainerAddItem(PropertyRowWidget* widget, InstanceDataNode* pContainerNode)
    {
        // Do expansion before modifying container as container modifications will invalidate and disallow the expansion until a later queued refresh
        OnPropertyRowExpandedOrContracted(widget, pContainerNode, true, true);

        // Locate the owning container. There may be a level of indirection due to wrappers, such as DynamicSerializableField.
        while (pContainerNode && !pContainerNode->GetClassMetadata()->m_container)
        {
            pContainerNode = pContainerNode->GetParent();
        }

        AZ_Assert(pContainerNode && pContainerNode->GetClassMetadata()->m_container, "Failed to locate container node for element \"%s\" of type %s.",
            pContainerNode->GetElementMetadata() ? pContainerNode->GetElementMetadata()->m_name : pContainerNode->GetClassMetadata()->m_name,
            pContainerNode->GetClassMetadata()->m_typeId.ToString<AZStd::string>().c_str());

        AZ::SerializeContext::IDataContainer* container = pContainerNode->GetClassMetadata()->m_container;

        //If the container is at capacity, we do not want to add another item.
        if (container->IsFixedCapacity() && !container->IsSmartPointer())
        {
            bool fullCapacity = pContainerNode->GetNumInstances() != 0;
            for (size_t i = 0; i < pContainerNode->GetNumInstances(); ++i)
            {
                if(container->Size(pContainerNode->GetInstance(i)) != container->Capacity(pContainerNode->GetInstance(i)))
                {
                    fullCapacity = false;
                    break;
                }
            }
            // Every instance to be modified is at full capacity, therefore an element cannot be added
            if (fullCapacity)
            {
                return;
            }
        }

        if (m_impl->m_ptrNotify)
        {
            m_impl->m_ptrNotify->BeforePropertyModified(pContainerNode);
        }
        // add element
        //data.m_dataNode->WriteStart();

        AZ_Assert(container->IsFixedSize() == false || container->IsSmartPointer(), "We can't add elements to static containers");

        auto promptForValue = [this](void* value, AZ::Uuid typeId, const char* message) -> bool
        {
            QDialog dialog;
            QVBoxLayout* layout = new QVBoxLayout(&dialog);

            ReflectedPropertyEditor* promptEditor = new ReflectedPropertyEditor(&dialog);

            if (message)
            {
                // Set the edit data for the key prompt
                AZ::Edit::ElementData syntheticData;
                syntheticData.m_elementId = 0;
                syntheticData.m_name = message;
                syntheticData.m_description = "";

                bool hasProvidedFirstDynamicEditData = false;
                promptEditor->SetDynamicEditDataProvider([&](const void* objectPtr, const AZ::SerializeContext::ClassData* classData) -> const AZ::Edit::ElementData*
                    {
                        Q_UNUSED(classData)

                        // Ensure we only override the first instance of dynamic element data
                        // This prevents overriding the name field for compound editors like AZ::EntityId where the address
                        // of the first field may be equivalent to the object address
                        if (objectPtr == value && !hasProvidedFirstDynamicEditData)
                        {
                            hasProvidedFirstDynamicEditData = true;
                            return &syntheticData;
                        }
                        return nullptr;
                    });
            }

            // Prompt using a new ReflectedPropertyEditor to query the value
            promptEditor->Setup(m_impl->m_context, nullptr, false);
            promptEditor->SetVisibilityCallback([](InstanceDataNode* node, NodeDisplayVisibility& visibility, bool& checkChildVisibility)
                {
                    // An associative container key will be (at the time of writing) either:
                    // - An RValueToLValueWrapper, a reflected class with one entry to expose the key type (most common)
                    // - A container class, like AZStd::pair (uncommon but possible with the highly permissive reflection system)
                    // In both of these cases, we're not interested in the top-level node (either the wrapper class or a container)
                    // and so we should hide the top level node and show its direct descendants (i.e. nodes with no grandparent)
                    if (!node->GetParent())
                    {
                        visibility = NodeDisplayVisibility::NotVisible;
                        checkChildVisibility = true;
                    }
                    else if (!node->GetParent()->GetParent())
                    {
                        visibility = NodeDisplayVisibility::Visible;
                    }
                });
            promptEditor->AddInstance(value, typeId);
            promptEditor->InvalidateAll();
            promptEditor->ExpandAll();
            promptEditor->setFixedHeight(promptEditor->GetContentHeight() + 2);
            layout->addWidget(promptEditor);

            QDialogButtonBox* buttonBox = new QDialogButtonBox;
            buttonBox->addButton(QDialogButtonBox::Ok);
            buttonBox->addButton(QDialogButtonBox::Cancel);
            // We cannot use exec here as it is modal and we need to be able to click in the viewport or drag entities onto the dislog
            int dialogFlag = -1;
            connect(buttonBox, &QDialogButtonBox::accepted, &dialog, [&dialogFlag]() {dialogFlag = 1; });
            connect(buttonBox, &QDialogButtonBox::rejected, &dialog, [&dialogFlag]() {dialogFlag = 0; });
            layout->addWidget(buttonBox);

            // Make sure the dialog stays on top ready for dropping onto
            dialog.setWindowFlags(Qt::WindowStaysOnTopHint);
            dialog.show();
            dialog.adjustSize();

            m_releasePrompt = false;

            while (dialogFlag < 0)
            {
                if (m_releasePrompt)
                {
                    dialogFlag = 0;
                    dialog.reject();
                    break;
                }

                qApp->processEvents();
            }

            return dialogFlag ? true : false;
        };

        AZStd::shared_ptr<void> keyToAdd(nullptr);

        bool createdElement = pContainerNode->CreateContainerElement(CreateContainerElementSelectClassCallback,
            [pContainerNode, promptForValue, &keyToAdd](void* dataPtr, const AZ::SerializeContext::ClassElement* classElement, bool noDefaultData, AZ::SerializeContext*) -> bool
        {
            bool handled = false;

            if (noDefaultData)
            {
                // If we're a keyed container, go ahead and prompt for a key to insert
                auto container = pContainerNode->GetElementMetadata() ? pContainerNode->GetElementMetadata()->m_genericClassInfo->GetClassData()->m_container : nullptr;
                auto associativeInterface = container ? container->GetAssociativeContainerInterface() : nullptr;
                if (associativeInterface)
                {
                    if (keyToAdd)
                    {
                        handled = true;
                    }
                    else
                    {
                        auto attribute = classElement->FindAttribute(AZ_CRC("KeyType", 0x15bc5303));
                        auto attributeData = azrtti_cast<AZ::AttributeData<AZ::TypeId>*>(attribute);
                        AZ_Assert(attributeData, "KeyType must be defined for keyed containers");
                        auto keyId = attributeData->Get(dataPtr);

                        keyToAdd = associativeInterface->CreateKey();
                        ReflectedPropertyEditorHelper::HandleDefaultNumericValues(keyToAdd.get(), keyId);

                        handled = promptForValue(keyToAdd.get(), keyId, "New Key");
                    }

                    if (handled)
                    {
                        associativeInterface->SetElementKey(dataPtr, keyToAdd.get());
                    }
                }
                else
                {
                    handled = promptForValue(dataPtr, classElement->m_typeId, nullptr);
                }
            }
            else
            {
                handled = ReflectedPropertyEditorHelper::HandleDefaultNumericValues(dataPtr, classElement->m_typeId);
            }

            return handled;
        }
        );

        if (createdElement)
        {
            // Fire any add notifications for the container widget.
            for (const AZ::Edit::AttributePair& attribute : pContainerNode->GetElementEditMetadata()->m_attributes)
            {
                if (attribute.first == AZ::Edit::Attributes::AddNotify)
                {
                    AZ::Edit::AttributeFunction<void()>* func = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(attribute.second);
                    if (func)
                    {
                        InstanceDataNode* pParent = pContainerNode->GetParent();
                        if (pParent)
                        {
                            for (size_t idx = 0; idx < pParent->GetNumInstances(); ++idx)
                            {
                                func->Invoke(pParent->GetInstance(idx));
                            }
                        }
                    }
                }
            }

            // Fire general change notifications for the container widget.
            if (widget)
            {
                widget->DoPropertyNotify();
            }

            // Only seal the undo stack once all modifications have been completed
            if (m_impl->m_ptrNotify)
            {
                m_impl->m_ptrNotify->AfterPropertyModified(pContainerNode);
                m_impl->m_ptrNotify->SealUndoStack();
            }

            // Need to refresh any pinned inspectors as well to keep the container state in sync
            QueueInvalidation(Refresh_EntireTree);
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_EntireTree);
        }
    }

    void ReflectedPropertyEditor::SetAutoResizeLabels(bool autoResizeLabels)
    {
        m_impl->m_autoResizeLabels = autoResizeLabels;
    }

    void ReflectedPropertyEditor::CancelQueuedRefresh()
    {
        m_impl->m_queuedRefreshLevel = Refresh_None;
    }

    void ReflectedPropertyEditor::PreventDataAccess(bool shouldPrevent)
    {
        // This is a no-op if we're already not blocking refresh actions
        if (!m_impl->m_preventRefresh && !shouldPrevent)
        {
            return;
        }

        // If we've set this to true twice in a row, then when one of them sets it to false, refreshes will be allowed
        // potentially before the second caller is ready for them.  This case should get examined to see why nested calls
        // are happening.  Either m_preventRefresh might need to turn into a refcount to allow nesting, or the assert might
        // be invalid, or the nesting shouldn't occur at all.
        AZ_Assert(!(m_impl->m_preventRefresh && shouldPrevent), 
                  "PreventRefresh set to 'true' twice.  If multiple different callers are setting this, it might need to become a refcount.");

        // Prevent property refreshes while we're disabled This avoids us accidentally refreshing during a destructive change.
        // This can occur in the event of a refresh occurring in a higher-level construct (e.g. the component entity inspector)
        m_impl->m_preventRefresh = shouldPrevent;

        // If we're preventing refreshes, we should also block user input to avoid any dangerous data access or mutation
        if (shouldPrevent)
        {
            m_impl->m_lastEnabledState = isEnabled();
            setEnabled(false);
        }
        // Otherwise, we should restore our enabled state to whatever it was before we were set to prevent access
        else
        {
            setEnabled(m_impl->m_lastEnabledState);
        }
    }

    void ReflectedPropertyEditor::QueueInvalidation(PropertyModificationRefreshLevel level)
    {
        if ((!m_impl->m_preventRefresh) && ((int)level > m_impl->m_queuedRefreshLevel))
        {
            // the callback told us that we need to do something more drastic than we're already scheduled to do (which might be nothing)
            bool rerequest = (m_impl->m_queuedRefreshLevel == Refresh_None); // if we haven't scheduled a refresh, then we will schedule one.
            m_impl->m_queuedRefreshLevel = level;
            if (rerequest)
            {
                QTimer::singleShot(0, this, SLOT(DoRefresh()));
            }
        }
    }

    void ReflectedPropertyEditor::ForceQueuedInvalidation()
    {
        DoRefresh();
    }

    void ReflectedPropertyEditor::DoRefresh()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_impl->m_preventRefresh || (m_impl->m_queuedRefreshLevel == Refresh_None))
        {
            return;
        }

        setUpdatesEnabled(false);

        // we've been asked to refresh the gui.
        switch (m_impl->m_queuedRefreshLevel)
        {
        case Refresh_Values:
            InvalidateValues();
            break;
        case Refresh_AttributesAndValues:
            InvalidateAttributesAndValues();
            break;
        case Refresh_EntireTree:
        case Refresh_EntireTree_NewContent:
            InvalidateAll();
            break;
        }

        m_impl->m_queuedRefreshLevel = Refresh_None;

        setUpdatesEnabled(true);
    }

    void ReflectedPropertyEditor::paintEvent(QPaintEvent* event)
    {
        QWidget::paintEvent(event);
    }

    void ReflectPropertyEditor(AZ::ReflectContext* context)
    {
        ReflectedPropertyEditorState::Reflect(context);
    }

    InstanceDataNode* ReflectedPropertyEditor::GetNodeFromWidget(QWidget* pTarget) const
    {
        PropertyRowWidget* pRow = nullptr;
        while ((pTarget != nullptr) && (pTarget != this))
        {
            if ((pRow = qobject_cast<AzToolsFramework::PropertyRowWidget*>(pTarget)) != nullptr)
            {
                break;
            }
            pTarget = pTarget->parentWidget();
        }

        if (pRow)
        {
            return pRow->GetNode();
        }

        return nullptr;
    }

    PropertyRowWidget* ReflectedPropertyEditor::GetWidgetFromNode(InstanceDataNode* node) const
    {
        auto widgetIter = m_impl->m_widgets.find(node);
        if (widgetIter != m_impl->m_widgets.end())
        {
            return widgetIter->second;
        }

        return nullptr;
    }

    void ReflectedPropertyEditor::ExpandAll(bool saveExpansionState)
    {
        const auto widgetsToExpand = m_impl->getTopLevelWidgets();
        for (auto widget : widgetsToExpand)
        {
            m_impl->CreateEditorWidget(widget);
            widget->SetExpanded(true);
            m_impl->ExpandChildren(widget, true, false);
        }

        m_impl->AdjustLabelWidth();

        if (saveExpansionState)
        {
            m_impl->SaveExpansion();
        }
    }

    void ReflectedPropertyEditor::CollapseAll(bool saveExpansionState)
    {
        const auto widgetsToCollapse = m_impl->getTopLevelWidgets();
        for (auto widget : widgetsToCollapse)
        {
            const auto key = AZStd::make_pair(widget->GetParentRow(), widget->label().toUtf8().data());
            bool isGroup = m_impl->m_groupWidgets.find(key) != m_impl->m_groupWidgets.end();
            if (!isGroup || widget->GetParentRow() == nullptr)
            {
                widget->SetExpanded(false);
                m_impl->ExpandChildren(widget, false, false);
            }
        }

        if (saveExpansionState)
        {
            m_impl->SaveExpansion();
        }
    }

    const ReflectedPropertyEditor::WidgetList& ReflectedPropertyEditor::GetWidgets() const
    {
        return m_impl->m_widgets;
    }

    int ReflectedPropertyEditor::GetContentHeight() const
    {
        return m_impl->m_containerWidget->layout()->sizeHint().height();
    }

    int ReflectedPropertyEditor::GetMaxLabelWidth() const
    {
        int width = 0;
        for (auto widget : m_impl->m_widgetsInDisplayOrder)
        {
            width = std::max(width, widget->LabelSizeHint().width());
        }
        return width;
    }

    void ReflectedPropertyEditor::SetLabelAutoResizeMinimumWidth(int labelMinimumWidth)
    {
        m_impl->m_propertyLabelAutoResizeMinimumWidth = labelMinimumWidth;
    }

    void ReflectedPropertyEditor::SetLabelWidth(int labelWidth)
    {
        m_impl->m_propertyLabelWidth = labelWidth;
        for (auto widget : m_impl->m_widgetsInDisplayOrder)
        {
            widget->SetLabelWidth(labelWidth);
        }
    }


    void ReflectedPropertyEditor::SetSelectionEnabled(bool selectionEnabled)
    {
        m_impl->m_selectionEnabled = selectionEnabled;
        for (auto widget : m_impl->m_widgetsInDisplayOrder)
        {
            widget->SetSelectionEnabled(selectionEnabled);
        }
        if (!m_impl->m_selectionEnabled)
        {
            m_impl->m_selectedRow = nullptr;
        }
    }

    void ReflectedPropertyEditor::SelectInstance(InstanceDataNode* node)
    {
        PropertyRowWidget* widget = GetWidgetFromNode(node);
        PropertyRowWidget *deselected = m_impl->m_selectedRow;
        if (deselected)
        {
            deselected->SetSelected(false);
            m_impl->m_selectedRow = nullptr;
            if (m_impl->m_ptrNotify)
            {
                InstanceDataNode *prevNode = GetNodeFromWidget(deselected);
                m_impl->m_ptrNotify->PropertySelectionChanged(prevNode, false);
            }
        }
        //if we selected a new row
        if (widget && widget != deselected)
        {
            m_impl->m_selectedRow = widget;
            m_impl->m_selectedRow->SetSelected(true);
            if (m_impl->m_ptrNotify)
            {
                m_impl->m_ptrNotify->PropertySelectionChanged(node, true);
            }
        }
    }

    AzToolsFramework::InstanceDataNode* ReflectedPropertyEditor::GetSelectedInstance() const
    {
        return GetNodeFromWidget(m_impl->m_selectedRow);
    }

    QSize ReflectedPropertyEditor::sizeHint() const
    {
        return m_impl->m_containerWidget->sizeHint() + m_impl->m_sizeHintOffset;
    }

    void ReflectedPropertyEditor::SetDynamicEditDataProvider(DynamicEditDataProvider provider)
    {
        m_impl->m_dynamicEditDataProvider = provider;
    }

    void ReflectedPropertyEditor::SetVisibilityCallback(VisibilityCallback callback)
    {
        m_impl->m_visibilityCallback = callback;
    }

    QWidget* ReflectedPropertyEditor::GetContainerWidget()
    {
        return m_impl->m_containerWidget;
    }

    void ReflectedPropertyEditor::SetSizeHintOffset(const QSize& offset)
    {
        m_impl->m_sizeHintOffset = offset;
    }

    QSize ReflectedPropertyEditor::GetSizeHintOffset() const
    {
        return m_impl->m_sizeHintOffset;
    }

    void ReflectedPropertyEditor::SetTreeIndentation(int indentation)
    {
        AZ_Assert(m_impl->m_instances.empty(), "This method should not be called after instances were added. Call this method before AddInstance.");
        m_impl->m_treeIndentation = indentation;
    }


    void ReflectedPropertyEditor::SetLeafIndentation(int indentation)
    {
        AZ_Assert(m_impl->m_instances.empty(), "This method should not be called after instances were added. Call this method before AddInstance.");
        m_impl->m_leafIndentation = indentation;
    }

}

#include "UI/PropertyEditor/moc_ReflectedPropertyEditor.cpp"

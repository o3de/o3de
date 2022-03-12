/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ReflectedPropertyCtrl.h"

// Qt
#include <QScopedValueRollback>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QScrollArea>

// AzToolsFramework
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>

//AzCore
#include <AzCore/Component/ComponentApplicationBus.h>

// Editor
#include "Clipboard.h"


ReflectedPropertyControl::ReflectedPropertyControl(QWidget *parent /*= nullptr*/, Qt::WindowFlags windowFlags /*= Qt::WindowFlags()*/)
    : QWidget(parent, windowFlags)
    , AzToolsFramework::IPropertyEditorNotify()
    , m_filterLineEdit(nullptr)
    , m_filterWidget(nullptr)
    , m_titleLabel(nullptr)
    , m_bEnableCallback(true)
    , m_updateVarFunc(nullptr)
    , m_updateObjectFunc(nullptr)
    , m_selChangeFunc(nullptr)
    , m_undoFunc(nullptr)
    , m_bStoreUndoByItems(true)
    , m_bForceModified(false)
    , m_groupProperties(false)
    , m_sortProperties(false)
    , m_bSendCallbackOnNonModified(true)
    , m_initialized(false)
    , m_isTwoColumnSection(false)
{
    EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(m_serializeContext, "Serialization context not available");
    qRegisterMetaType<IVariable*>("IVariablePtr");

    m_editor = new AzToolsFramework::ReflectedPropertyEditor(nullptr);
    m_editor->SetAutoResizeLabels(true);

    m_titleLabel = new QLabel;
    m_titleLabel->hide();

    m_filterWidget = new QWidget;
    QLabel *label = new QLabel(tr("Search"));
    m_filterLineEdit = new QLineEdit;
    QHBoxLayout *filterlayout = new QHBoxLayout(m_filterWidget);
    filterlayout->addWidget(label);
    filterlayout->addWidget(m_filterLineEdit);
    connect(m_filterLineEdit, &QLineEdit::textChanged, this, &ReflectedPropertyControl::RestrictToItemsContaining);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_filterWidget);
    mainLayout->addWidget(m_editor, 1);

    SetShowFilterWidget(false);
    setMinimumSize(330, 0);
}

void ReflectedPropertyControl::Setup(bool showScrollbars /*= true*/, int labelWidth /*= 150*/)
{
    if (!m_initialized)
    {
        m_editor->Setup(m_serializeContext, this, showScrollbars, labelWidth);
        m_initialized = true;
    }
}

QSize ReflectedPropertyControl::sizeHint() const
{
    return m_editor->sizeHint();
}

ReflectedPropertyItem* ReflectedPropertyControl::AddVarBlock(CVarBlock *varBlock, const char *szCategory /*= nullptr*/)
{
    AZ_Assert(m_initialized, "ReflectedPropertyControl not initialized. Setup must be called first.");

    if (!varBlock)
        return nullptr;

    m_pVarBlock = varBlock;

    if (!m_root)
    {
        m_root = new ReflectedPropertyItem(this, nullptr);
        m_rootContainer.reset(new CPropertyContainer(szCategory ? AZStd::string(szCategory) : AZStd::string()));
        m_rootContainer->SetAutoExpand(true);
        m_editor->AddInstance(m_rootContainer.get());
    }


    AZStd::vector<IVariable*> variables(varBlock->GetNumVariables());

    //Copy variables into vector
    int n = 0;
    std::generate(variables.begin(), variables.end(), [&n, varBlock]{return varBlock->GetVariable(n++); });

    //filter list based on search string
    if (!m_filterString.isEmpty())
    {
        AZStd::vector<IVariable*> newVariables;
        for (IVariable* var : variables)
        {
            if (QString(var->GetName()).toLower().contains(m_filterString))
            {
                newVariables.emplace_back(var);
            }
        }
        variables.swap(newVariables);
    }

    //sorting if needed.  sort first when grouping to make grouping easier
    if (m_sortProperties || m_groupProperties)
    {
        std::sort(variables.begin(), variables.end(), [](IVariable *var1, IVariable *var2) {return QString::compare(var1->GetName(), var2->GetName(), Qt::CaseInsensitive) <=0; } );
    }

    CPropertyContainer *parentContainer = m_rootContainer.get();
    ReflectedPropertyItem *parentItem = m_root;
    QChar currentGroupInitial;
    for (auto var : variables)
    {
        if (m_groupProperties)
        {
            //check to see if this item starts with same letter as last. If not, create a new group for it.
            const QChar groupInitial = var->GetName().toUpper().at(0);
            if (groupInitial != currentGroupInitial)
            {
                currentGroupInitial = groupInitial;
                //make new group be the parent for this item
                parentItem = new ReflectedPropertyItem(this, parentItem);

                QString groupName(groupInitial);
                parentContainer = new CPropertyContainer(AZStd::string(groupName.toUtf8().data()));
                parentContainer->SetAutoExpand(false);
                m_rootContainer->AddProperty(parentContainer);
            }
        }
        ReflectedPropertyItemPtr childItem = new ReflectedPropertyItem(this, parentItem);
        childItem->SetVariable(var);
        CReflectedVar *reflectedVar = childItem->GetReflectedVar();
        parentContainer->AddProperty(reflectedVar);
    }
    m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);

    return parentItem;
}

//////////////////////////////////////////////////////////////////////////
static void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, const char* humanVarName, const char* description, IVariable::OnSetCallback* func, void* pUserData, char dataType = IVariable::DT_SIMPLE)
{
    if (varName)
    {
        var.SetName(varName);
    }
    if (humanVarName)
    {
        var.SetHumanName(humanVarName);
    }
    if (description)
    {
        var.SetDescription(description);
    }
    var.SetDataType(dataType);
    var.SetUserData(QVariant::fromValue<void *>(pUserData));
    if (func)
    {
        var.AddOnSetCallback(func);
    }
    varArray.AddVariable(&var);
}

void ReflectedPropertyControl::CreateItems(XmlNodeRef node)
{
    CVarBlockPtr out;
    CreateItems(node, out, nullptr);
}

void ReflectedPropertyControl::CreateItems(XmlNodeRef node, CVarBlockPtr& outBlockPtr, IVariable::OnSetCallback* func, bool splitCamelCaseIntoWords)
{
    SelectItem(nullptr);

    outBlockPtr = new CVarBlock;
    for (size_t i = 0, iGroupCount(node->getChildCount()); i < iGroupCount; ++i)
    {
        XmlNodeRef groupNode = node->getChild(static_cast<int>(i));

        if (groupNode->haveAttr("hidden"))
        {
            bool isGroupHidden = false;
            groupNode->getAttr("hidden", isGroupHidden);
            if (isGroupHidden)
            {
                // do not create visual editors for this group
                continue;
            }
        }

        CSmartVariableArray group;
        group->SetName(groupNode->getTag());
        group->SetHumanName(groupNode->getTag());
        group->SetDescription("");
        group->SetDataType(IVariable::DT_SIMPLE);
        outBlockPtr->AddVariable(&*group);

        for (int k = 0, iChildCount(groupNode->getChildCount()); k < iChildCount; ++k)
        {
            XmlNodeRef child = groupNode->getChild(k);

            const char* type;
            if (!child->getAttr("type", &type))
            {
                continue;
            }

            // read parameter description from the tip tag and from associated console variable
            QString strDescription;
            child->getAttr("tip", strDescription);
            QString strTipCVar;
            child->getAttr("TipCVar", strTipCVar);
            if (!strTipCVar.isEmpty())
            {
                strTipCVar.replace("*", child->getTag());
                if (ICVar* pCVar = gEnv->pConsole->GetCVar(strTipCVar.toUtf8().data()))
                {
                    if (!strDescription.isEmpty())
                    {
                        strDescription += QString("\r\n");
                    }
                    strDescription = pCVar->GetHelp();

#ifdef FEATURE_SVO_GI
                    // Hide or unlock experimental items
                    if ((pCVar->GetFlags() & VF_EXPERIMENTAL) && strstr(groupNode->getTag(), "Total_Illumination"))
                    {
                        AzToolsFramework::EditorSettingsAPIRequests::SettingOutcome outcome;
                        AzToolsFramework::EditorSettingsAPIBus::BroadcastResult(outcome, &AzToolsFramework::EditorSettingsAPIBus::Handler::GetValue, "Settings\\ExperimentalFeatures|TotalIlluminationEnabled");
                        AZStd::any outcomeValue = outcome.GetValue<AZStd::any>();
                        if (outcomeValue.is<bool>())
                        {
                            bool featureEnabled = AZStd::any_cast<bool>(outcomeValue);
                            if (!featureEnabled)
                            {
                                continue;
                            }
                        }
                    }
#endif
                }
            }
            QString humanReadableName;
            child->getAttr("human", humanReadableName);
            if (humanReadableName.isEmpty())
            {
                humanReadableName = child->getTag();

                if (splitCamelCaseIntoWords)
                {
                    for (int index = 1; index < humanReadableName.length() - 1; index++)
                    {
                        // insert spaces between words
                        if ((humanReadableName[index - 1].isLower() && humanReadableName[index].isUpper()) || (humanReadableName[index + 1].isLower() && humanReadableName[index - 1].isUpper() && humanReadableName[index].isUpper()))
                        {
                            humanReadableName.insert(index++, ' ');
                        }

                        // convert single upper cases letters to lower case
                        if (humanReadableName[index].isUpper() && humanReadableName[index + 1].isLower())
                        {
                            humanReadableName[index] = humanReadableName[index].toLower();
                        }
                    }
                }
            }

            void* pUserData = reinterpret_cast<void*>((i << 16) | k);

            if (!azstricmp(type, "int"))
            {
                CSmartVariable<int> intVar;
                AddVariable(group, intVar, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData);
                int nValue(0);
                if (child->getAttr("value", nValue))
                {
                    intVar->Set(nValue);
                }

                int nMin(0), nMax(0);
                if (child->getAttr("min", nMin) && child->getAttr("max", nMax))
                {
                    intVar->SetLimits(static_cast<float>(nMin), static_cast<float>(nMax));
                }
            }
            else if (!azstricmp(type, "float"))
            {
                CSmartVariable<float> floatVar;
                AddVariable(group, floatVar, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData);
                float fValue(0.0f);
                if (child->getAttr("value", fValue))
                {
                    floatVar->Set(fValue);
                }

                float fMin(0), fMax(0);
                if (child->getAttr("min", fMin) && child->getAttr("max", fMax))
                {
                    floatVar->SetLimits(fMin, fMax);
                }
            }
            else if (!azstricmp(type, "vector"))
            {
                CSmartVariable<Vec3> vec3Var;
                AddVariable(group, vec3Var, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData);
                Vec3 vValue(0, 0, 0);
                if (child->getAttr("value", vValue))
                {
                    vec3Var->Set(vValue);
                }
            }
            else if (!azstricmp(type, "bool"))
            {
                CSmartVariable<bool> bVar;
                AddVariable(group, bVar, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData);
                bool bValue(false);
                if (child->getAttr("value", bValue))
                {
                    bVar->Set(bValue);
                }
            }
            else if (!azstricmp(type, "texture"))
            {
                CSmartVariable<QString> textureVar;
                AddVariable(group, textureVar, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData, IVariable::DT_TEXTURE);
                const char* textureName;
                if (child->getAttr("value", &textureName))
                {
                    textureVar->Set(textureName);
                }
            }
            else if (!azstricmp(type, "color"))
            {
                CSmartVariable<Vec3> colorVar;
                AddVariable(group, colorVar, child->getTag(), humanReadableName.toUtf8().data(), strDescription.toUtf8().data(), func, pUserData, IVariable::DT_COLOR);
                ColorB color;
                if (child->getAttr("value", color))
                {
                    ColorF colorLinear = ColorGammaToLinear(QColor(color.r, color.g, color.b));
                    Vec3 colorVec3(colorLinear.r, colorLinear.g, colorLinear.b);
                    colorVar->Set(colorVec3);
                }
            }
        }
    }

    AddVarBlock(outBlockPtr);

    InvalidateCtrl();
}

void ReflectedPropertyControl::ReplaceVarBlock(IVariable *categoryItem, CVarBlock *varBlock)
{
    assert(m_root);
    ReflectedPropertyItem *pCategoryItem = m_root->findItem(categoryItem);
    if (pCategoryItem)
    {
        pCategoryItem->ReplaceVarBlock(varBlock);
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);
    }
}

void ReflectedPropertyControl::ReplaceRootVarBlock(CVarBlock* newVarBlock)
{
    const AZStd::string category = m_rootContainer->m_varName;
    RemoveAllItems();
    AddVarBlock(newVarBlock, category.c_str());
}

void ReflectedPropertyControl::UpdateVarBlock(CVarBlock *pVarBlock)
{
    UpdateVarBlock(m_root, pVarBlock, m_pVarBlock);
    m_editor->QueueInvalidation(AzToolsFramework::Refresh_AttributesAndValues);
}

void ReflectedPropertyControl::UpdateVarBlock(ReflectedPropertyItem* pPropertyItem, IVariableContainer *pSourceContainer, IVariableContainer *pTargetContainer)
{
    for (int i = 0; i < pPropertyItem->GetChildCount(); ++i)
    {
        ReflectedPropertyItem *pChild = pPropertyItem->GetChild(i);

        if (pChild->GetType() != ePropertyInvalid)
        {
            const QString pPropertyVariableName = pChild->GetVariable()->GetName();

            IVariable* pTargetVariable = pTargetContainer->FindVariable(pPropertyVariableName.toUtf8().data());
            IVariable* pSourceVariable = pSourceContainer->FindVariable(pPropertyVariableName.toUtf8().data());

            if (pSourceVariable && pTargetVariable)
            {
                pTargetVariable->SetFlags(pSourceVariable->GetFlags());
                pTargetVariable->SetDisplayValue(pSourceVariable->GetDisplayValue());
                pTargetVariable->SetUserData(pSourceVariable->GetUserData());

                UpdateVarBlock(pChild, pSourceVariable, pTargetVariable);
            }
        }
    }
}

ReflectedPropertyItem* ReflectedPropertyControl::FindItemByVar(IVariable* pVar)
{
    return m_root->findItem(pVar);
}

ReflectedPropertyItem* ReflectedPropertyControl::GetRootItem()
{
    return m_root;
}

int ReflectedPropertyControl::GetContentHeight() const
{
    return m_editor->GetContentHeight();
}


void ReflectedPropertyControl::AddCustomPopupMenuPopup(const QString& text, const AZStd::function<void(int)>& handler, const QStringList& items)
{
    m_customPopupMenuPopups.push_back(SCustomPopupMenu(text, handler, items));
}


void ReflectedPropertyControl::AddCustomPopupMenuItem(const QString& text, const SCustomPopupItem::Callback handler)
{
    m_customPopupMenuItems.push_back(SCustomPopupItem(text, handler));
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void ReflectedPropertyControl::RemoveCustomPopup(const QString& text, T& customPopup)
{
    for (auto itr = customPopup.begin(); itr != customPopup.end(); ++itr)
    {
        if (text == itr->m_text)
        {
            customPopup.erase(itr);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertyControl::RemoveCustomPopupMenuItem(const QString& text)
{
    RemoveCustomPopup(text, m_customPopupMenuItems);
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertyControl::RemoveCustomPopupMenuPopup(const QString& text)
{
    RemoveCustomPopup(text, m_customPopupMenuPopups);
}

void ReflectedPropertyControl::RestrictToItemsContaining(const QString &searchName)
{
   m_filterString = searchName.toLower();
   RecreateAllItems();
}

void ReflectedPropertyControl::SetUpdateCallback(const ReflectedPropertyControl::UpdateVarCallback& callback)
{
    m_updateVarFunc = callback;
}

void ReflectedPropertyControl::SetSavedStateKey(AZ::u32 key)
{
    m_editor->SetSavedStateKey(key);
}

void ReflectedPropertyControl::RemoveAllItems()
{
    m_editor->ClearInstances();
    m_rootContainer.reset();
    m_root.reset();
}

void ReflectedPropertyControl::ClearVarBlock()
{
    RemoveAllItems();
    m_pVarBlock = nullptr;
}

void ReflectedPropertyControl::RecreateAllItems()
{
    RemoveAllItems();
    AddVarBlock(m_pVarBlock);
}


void ReflectedPropertyControl::SetGroupProperties(bool group)
{
    m_groupProperties = group;
    RecreateAllItems();
}

void ReflectedPropertyControl::SetSortProperties(bool sort)
{
    m_sortProperties = sort;
    RecreateAllItems();
}


void ReflectedPropertyControl::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
{
    if (!pNode)
        return;

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *item = m_root->findItem(pReflectedVar);
    AZ_Assert(item, "No item found in property modification callback");
    item->OnReflectedVarChanged();

    OnItemChange(item);
}


void ReflectedPropertyControl::SetIsTwoColumnCtrlSection(bool isSection)
{
    m_isTwoColumnSection = isSection;
}

void ReflectedPropertyControl::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* pNode, const QPoint& pos)
{
    if (!pNode)
        return;

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *pItem = m_root->findItem(pReflectedVar);
    AZ_Assert(pItem, "No item found in Context Menu callback");

    CClipboard clipboard(nullptr);

    // Popup Menu with Event selection.
    QMenu menu;
    unsigned int i = 0;

    const int ePPA_CustomItemBase = 10; // reserved from 10 to 99
    const int ePPA_CustomPopupBase = 100; // reserved from 100 to x*100+100 where x is size of m_customPopupMenuPopups

    menu.addAction(tr("Copy"), [&]() { OnCopy({ pItem }, false); });
    menu.addAction(tr("Copy Recursively"), [&]() { OnCopy({ pItem }, true); });
    if (m_isTwoColumnSection)
    {
        // For a two-column control, OnCopyAll will only copy all for this section
            // Emit a signal to the two-column control if we want to copy all sections
        menu.addAction(tr("Copy Section"), [&]() { OnCopyAll(); });
        menu.addAction(tr("Copy All"), [&]() { emit CopyAllSections(); });
        menu.addSeparator();
        menu.addAction(tr("Paste"), [&]() { emit PasteAllSections(); })->setEnabled(!clipboard.IsEmpty());
    }
    else
    {
        menu.addAction(tr("Copy All"), [&]() { OnCopyAll(); });
        menu.addSeparator();
        menu.addAction(tr("Paste"), [&]() { OnPaste(); })->setEnabled(!clipboard.IsEmpty());
    }


    if (!m_customPopupMenuItems.empty() || !m_customPopupMenuPopups.empty())
    {
        menu.addSeparator();
    }

    for (auto itr = m_customPopupMenuItems.cbegin(); itr != m_customPopupMenuItems.cend(); ++itr, ++i)
    {
        QAction *action = menu.addAction(itr->m_text);
        action->setData(ePPA_CustomItemBase + i);
    }

    for (unsigned int j = 0; j < m_customPopupMenuPopups.size(); ++j)
    {
        SCustomPopupMenu* pMenuInfo = &m_customPopupMenuPopups[j];
        QMenu* pSubMenu = menu.addMenu(pMenuInfo->m_text);

        for (UINT k = 0; k < static_cast<UINT>(pMenuInfo->m_subMenuText.size()); ++k)
        {
            const UINT uID = ePPA_CustomPopupBase + ePPA_CustomPopupBase * j + k;
            QAction *action = pSubMenu->addAction(pMenuInfo->m_subMenuText[k]);
            action->setData(uID);
        }
    }

    QAction *result = menu.exec(pos);
    if (!result)
    {
        return;
    }
    const int res = result->data().toInt();
    if (res >= ePPA_CustomItemBase && res < m_customPopupMenuItems.size() + ePPA_CustomItemBase)
    {
        m_customPopupMenuItems[res - ePPA_CustomItemBase].m_callback();
    }
    else if (res >= ePPA_CustomPopupBase && res < ePPA_CustomPopupBase + ePPA_CustomPopupBase * m_customPopupMenuPopups.size())
    {
        const int menuid = res / ePPA_CustomPopupBase - 1;
        const int option = res % ePPA_CustomPopupBase;
        m_customPopupMenuPopups[menuid].m_callback(option);
    }
}
void ReflectedPropertyControl::SetSelChangeCallback(SelChangeCallback callback)
{
    m_selChangeFunc = callback;
    m_editor->SetSelectionEnabled(true);
}

void ReflectedPropertyControl::PropertySelectionChanged(AzToolsFramework::InstanceDataNode *pNode, bool selected)
{
    if (!pNode)
    {
        return;
    }

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *pItem = m_root->findItem(pReflectedVar);
    AZ_Assert(pItem, "No item found in selectoin change callback");

    if (m_selChangeFunc)
    {
        //keep same logic as MFC where pass null if it's deselection
        m_selChangeFunc(selected ? pItem->GetVariable() : nullptr);
    }
}

CReflectedVar * ReflectedPropertyControl::GetReflectedVarFromCallbackInstance(AzToolsFramework::InstanceDataNode *pNode)
{
    if (!pNode)
        return nullptr;
    const AZ::SerializeContext::ClassData *classData = pNode->GetClassMetadata();
    if (classData->m_azRtti && classData->m_azRtti->IsTypeOf(CReflectedVar::TYPEINFO_Uuid()))
        return reinterpret_cast<CReflectedVar *>(pNode->GetInstance(0));
    else
        return GetReflectedVarFromCallbackInstance(pNode->GetParent());
}


AzToolsFramework::PropertyRowWidget* ReflectedPropertyControl::FindPropertyRowWidget(ReflectedPropertyItem* item)
{
    if (!item)
    {
        return nullptr;
    }
    const AzToolsFramework::ReflectedPropertyEditor::WidgetList& widgets = m_editor->GetWidgets();
    for (const auto& instance : widgets)
    {
        if (instance.second->label() == item->GetPropertyName())
        {
            return instance.second;
        }
    }
    return nullptr;
}


void ReflectedPropertyControl::OnItemChange(ReflectedPropertyItem *item, bool deferCallbacks)
{
    if (!item->IsModified() || !m_bSendCallbackOnNonModified)
        return;

    // variable updates/changes can trigger widgets being shown/hidden; allow a delay triggering the update
    // callback until after the current event queue is processed, so that we aren't changing other widgets
    // as a ton of them are still being created.
    Qt::ConnectionType connectionType = deferCallbacks ? Qt::QueuedConnection : Qt::DirectConnection;
    if (m_updateVarFunc && m_bEnableCallback)
    {
        QMetaObject::invokeMethod(this, "DoUpdateCallback", connectionType, Q_ARG(IVariable*, item->GetVariable()));
    }
    if (m_updateObjectFunc && m_bEnableCallback)
    {
        // KDAB: This callback has same signature as DoUpdateCallback. I think the only reason there are 2 is because some
        // EntityObject registers callback and some derived objects want to register their own callback. the normal UpdateCallback
        // can only be registered for item at a time so the original authors added a 2nd callback function, so we ported it this way.
        // This can probably get cleaned up to only on callback function with multiple receivers.
        QMetaObject::invokeMethod(this, "DoUpdateObjectCallback", connectionType, Q_ARG(IVariable*, item->GetVariable()));
    }
}


void ReflectedPropertyControl::DoUpdateCallback(IVariable *var)
{
    // guard against element containing the IVariable being removed during a deferred callback
    const bool variableStillExists = FindVariable(var);
    AZ_Assert(variableStillExists, "This variable and the item containing it were destroyed during a deferred callback. Change to non-deferred callback.");

    if (!m_updateVarFunc || !variableStillExists)
    {
        return;
    }

    QScopedValueRollback<bool> rb(m_bEnableCallback, false);
    m_updateVarFunc(var);
}

void ReflectedPropertyControl::DoUpdateObjectCallback(IVariable *var)
{
    // guard against element containing the IVariable being removed during a deferred callback
    const bool variableStillExists = FindVariable(var);
    AZ_Assert(variableStillExists, "This variable and the item containing it were destroyed during a deferred callback. Change to non-deferred callback.");

    if ( !m_updateVarFunc || !variableStillExists)
    {
        return;
    }

    QScopedValueRollback<bool> rb(m_bEnableCallback, false);
    m_updateObjectFunc(var);
}

void ReflectedPropertyControl::InvalidateCtrl(bool queued)
{
    if (queued)
    {
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_AttributesAndValues);
    }
    else
    {
        m_editor->InvalidateAttributesAndValues();
    }
}

void ReflectedPropertyControl::RebuildCtrl(bool queued)
{
    if (queued)
    {
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);
    }
    else
    {
        m_editor->InvalidateAll();
    }
}

bool ReflectedPropertyControl::CallUndoFunc(ReflectedPropertyItem *item)
{
    if (!m_undoFunc)
        return false;

    m_undoFunc(item->GetVariable());
    return true;

}

void ReflectedPropertyControl::ClearSelection()
{
    m_editor->SelectInstance(nullptr);
}

void ReflectedPropertyControl::SelectItem(ReflectedPropertyItem* item)
{
    AzToolsFramework::PropertyRowWidget *widget = FindPropertyRowWidget(item);
    if (widget)
    {
        m_editor->SelectInstance(m_editor->GetNodeFromWidget(widget));
    }
}

ReflectedPropertyItem* ReflectedPropertyControl::GetSelectedItem()
{
    AzToolsFramework::PropertyRowWidget *widget = m_editor->GetWidgetFromNode(m_editor->GetSelectedInstance());
    if (widget)
    {
       return m_root->findItem(widget->label());
    }
    return nullptr;
}


QVector<ReflectedPropertyItem*> ReflectedPropertyControl::GetSelectedItems()
{
    auto item = GetSelectedItem();
    return item == nullptr ? QVector<ReflectedPropertyItem*>() : QVector<ReflectedPropertyItem*>{item};
}

void ReflectedPropertyControl::OnCopy(QVector<ReflectedPropertyItem*> itemsToCopy, bool bRecursively)
{
    if (!itemsToCopy.isEmpty())
    {
        CClipboard clipboard(nullptr);
        auto rootNode = XmlHelpers::CreateXmlNode("PropertyCtrl");
        for (auto item : itemsToCopy)
        {
            CopyItem(rootNode, item, bRecursively);
        }
        clipboard.Put(rootNode);
    }
}

void ReflectedPropertyControl::OnCopyAll()
{
    if (m_root)
    {
        CClipboard clipboard(nullptr);
        auto rootNode = XmlHelpers::CreateXmlNode("PropertyCtrl");
        OnCopyAll(rootNode);
        clipboard.Put(rootNode);
    }
}

void ReflectedPropertyControl::OnCopyAll(XmlNodeRef rootNode)
{
    if (m_root)
    {
        for (int i = 0; i < m_root->GetChildCount(); i++)
        {
            CopyItem(rootNode, m_root->GetChild(i), true);
        }
    }
}

void ReflectedPropertyControl::OnPaste()
{
    CClipboard clipboard(nullptr);

    CUndo undo("Paste Properties");

    XmlNodeRef rootNode = clipboard.Get();
    SetValuesFromNode(rootNode);
}

void ReflectedPropertyControl::SetValuesFromNode(XmlNodeRef rootNode)
{
    if (!rootNode || !rootNode->isTag("PropertyCtrl"))
    {
        return;
    }

    for (int i = 0; i < rootNode->getChildCount(); i++)
    {
        XmlNodeRef node = rootNode->getChild(i);
        QString value;
        QString name;
        node->getAttr("Name", name);
        node->getAttr("Value", value);
        auto pItem = m_root->FindItemByFullName(name);
        if (pItem)
        {
            pItem->SetValue(value);
            // process callbacks immediately. In some cases, like the MaterialEditor the changing
            // the value of one item will change the properties available in this control
            // which needs to happen before we paste other values.
            OnItemChange(pItem, false);
        }
    }
}

void ReflectedPropertyControl::CopyItem(XmlNodeRef rootNode, ReflectedPropertyItem* pItem, bool bRecursively)
{
    XmlNodeRef node = rootNode->newChild("PropertyItem");
    node->setAttr("Name", pItem->GetFullName().toLatin1().data());
    node->setAttr("Value", pItem->GetVariable()->GetDisplayValue().toLatin1().data());
    if (bRecursively)
    {
        for (int i = 0; i < pItem->GetChildCount(); i++)
        {
            CopyItem(rootNode, pItem->GetChild(i), bRecursively);
        }
    }
}

void ReflectedPropertyControl::ReloadValues()
{
    if (m_root)
        m_root->ReloadValues();

    InvalidateCtrl();
}

void ReflectedPropertyControl::SetShowFilterWidget(bool showFilter)
{
    m_filterWidget->setVisible(showFilter);
}

void ReflectedPropertyControl::SetUndoCallback(UndoCallback &callback)
{
    m_undoFunc = callback;
}

void ReflectedPropertyControl::ClearUndoCallback()
{
    m_undoFunc = nullptr;
}

bool ReflectedPropertyControl::FindVariable(IVariable *categoryItem) const
{
    assert(m_root);
    if (!m_root)
    {
        return false;
    }

    ReflectedPropertyItem *pItem = m_root->findItem(categoryItem);
    return pItem != nullptr;
}

void ReflectedPropertyControl::EnableUpdateCallback(bool bEnable)
{
    // Handle case where update callbacks were disabled and we're enabling them now
    // Need to force immediate invalidation of any queued invalidations made while callbacks were disabled
    // to make them fire now, while m_bEnableCallback is still false.
    if (bEnable && !m_bEnableCallback)
    {
        m_editor->ForceQueuedInvalidation();
    }
    m_bEnableCallback = bEnable;
}

void ReflectedPropertyControl::SetGrayed([[maybe_unused]] bool grayed)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
    //control should be grayed out but not disabled?
}

void ReflectedPropertyControl::SetReadOnly(bool readonly)
{
    setEnabled(!readonly);
}


void ReflectedPropertyControl::SetMultiSelect([[maybe_unused]] bool multiselect)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
}

void ReflectedPropertyControl::EnableNotifyWithoutValueChange(bool bFlag)
{
    m_bForceModified = bFlag;
}

void ReflectedPropertyControl::SetTitle(const QString &title)
{
    m_titleLabel->setText(title);
    m_titleLabel->setHidden(title.isEmpty());
}

void ReflectedPropertyControl::ExpandAll()
{
    m_editor->ExpandAll();
}

void ReflectedPropertyControl::CollapseAll()
{
    m_editor->CollapseAll();
}

void ReflectedPropertyControl::Expand(ReflectedPropertyItem* item, bool expand)
{
    item->Expand(expand);
}

void ReflectedPropertyControl::ExpandAllChildren(ReflectedPropertyItem* item, bool recursive)
{
    item->ExpandAllChildren(recursive);
}

TwoColumnPropertyControl::TwoColumnPropertyControl(QWidget *parent /*= nullptr*/)
    : QWidget(parent)
    , m_twoColumns(true)
{
    QHBoxLayout *mainlayout = new QHBoxLayout(this);

    m_leftContainer = new QWidget;
    auto leftLayout = new QVBoxLayout(m_leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_leftScrollArea = new QScrollArea;
    m_leftScrollArea->setMinimumWidth(minimumColumnWidth);

    m_leftScrollArea->setWidget(m_leftContainer);
    m_leftScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftScrollArea->setWidgetResizable(true);
    mainlayout->addWidget(m_leftScrollArea);

    m_rightContainer = new QWidget;

    auto rightLayout = new QVBoxLayout(m_rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_rightScrollArea = new QScrollArea;
    m_rightScrollArea->setMinimumWidth(minimumColumnWidth);

    m_rightScrollArea->setWidget(m_rightContainer);
    m_rightScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rightScrollArea->setWidgetResizable(true);
    mainlayout->addWidget(m_rightScrollArea);
}

void TwoColumnPropertyControl::Setup([[maybe_unused]] bool showScrollbars /*= true*/, [[maybe_unused]] int labelWidth /*= 150*/)
{
}
void TwoColumnPropertyControl::AddVarBlock(CVarBlock *varBlock, [[maybe_unused]] const char *szCategory /*= nullptr*/)
{
    m_pVarBlock = varBlock;

    auto leftLayout = static_cast<QBoxLayout *>(m_leftContainer->layout());
    auto rightLayout = static_cast<QBoxLayout *>(m_rightContainer->layout());

    for (int i = 0; i < m_pVarBlock->GetNumVariables(); ++i)
    {
        CVarBlock *vb = new CVarBlock;
        PropertyCard *ctrl = new PropertyCard;
        m_varBlockList.append(vb);
        m_controlList.append(ctrl);
        IVariable *var = m_pVarBlock->GetVariable(i);
        vb->AddVariable(var);
        ctrl->AddVarBlock(vb);

        if (var->GetFlags() & IVariable::UI_ROLLUP2)
        {
            rightLayout->addWidget(ctrl);
        }
        else
        {
            leftLayout->addWidget(ctrl);
        }

        ctrl->GetControl()->SetIsTwoColumnCtrlSection(true);
        connect(ctrl->GetControl(), &ReflectedPropertyControl::CopyAllSections, this, &TwoColumnPropertyControl::OnCopyAll);
        connect(ctrl->GetControl(), &ReflectedPropertyControl::PasteAllSections, this, &TwoColumnPropertyControl::OnPaste);
    }

    leftLayout->addStretch(1);
    rightLayout->addStretch(1);
}

void TwoColumnPropertyControl::resizeEvent(QResizeEvent *event)
{
    const bool twoColumns = event->size().width() >= minimumTwoColumnWidth;
    if (m_twoColumns != twoColumns)
    {
        ToggleTwoColumnLayout();
    }
}

void TwoColumnPropertyControl::ToggleTwoColumnLayout()
{
    auto leftLayout = static_cast<QBoxLayout *>(m_leftContainer->layout());

    if (m_twoColumns)
    {
        // change layout to one column
        leftLayout->insertWidget(0, m_rightScrollArea->takeWidget());
        m_rightScrollArea->hide();
    }
    else
    {
        // change layout to two columns
        auto item = leftLayout->takeAt(0);
        m_rightScrollArea->setWidget(item->widget());
        delete item;
        m_rightScrollArea->show();
    }

    m_twoColumns = !m_twoColumns;
}

void TwoColumnPropertyControl::ReplaceVarBlock(IVariable *categoryItem, CVarBlock *varBlock)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ReplaceVarBlock(categoryItem, varBlock);
    }
}

void TwoColumnPropertyControl::RemoveAllItems()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->RemoveAllItems();
    }
}

bool TwoColumnPropertyControl::FindVariable(IVariable *categoryItem) const
{
    for (auto ctrl : m_controlList)
    {
        if (ctrl->GetControl()->FindVariable(categoryItem))
        {
            return true;
        }
    }

    return false;
}

void TwoColumnPropertyControl::InvalidateCtrl()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->InvalidateCtrl();
    }
}

void TwoColumnPropertyControl::RebuildCtrl()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->RebuildCtrl();
    }
}

void TwoColumnPropertyControl::SetStoreUndoByItems(bool bStoreUndoByItems)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetStoreUndoByItems(bStoreUndoByItems);
    }
}

void TwoColumnPropertyControl::SetUndoCallback(ReflectedPropertyControl::UndoCallback callback)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetUndoCallback(callback);
    }
}

void TwoColumnPropertyControl::ClearUndoCallback()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ClearUndoCallback();
    }
}

void TwoColumnPropertyControl::EnableUpdateCallback(bool bEnable)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->EnableUpdateCallback(bEnable);
    }
}

void TwoColumnPropertyControl::SetUpdateCallback(ReflectedPropertyControl::UpdateVarCallback callback)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetUpdateCallback(callback);
    }
}

void TwoColumnPropertyControl::SetGrayed(bool grayed)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetGrayed(grayed);
    }
}


void TwoColumnPropertyControl::SetSavedStateKey(const QString &key)
{
    for (int i = 0; i < m_controlList.count(); ++i)
    {
        m_controlList[i]->GetControl()->SetSavedStateKey(AZ::Crc32((key + QString::number(i)).toUtf8().data()));
    }
}

void TwoColumnPropertyControl::ExpandAllChildren(ReflectedPropertyItem* item, bool recursive)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ExpandAllChildren(item, recursive);
    }
}

void TwoColumnPropertyControl::ExpandAllChildren(bool recursive)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ExpandAllChildren(ctrl->GetControl()->GetRootItem(), recursive);
    }
}

void TwoColumnPropertyControl::ReloadItems()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ReloadValues();
    }
}

void TwoColumnPropertyControl::OnCopyAll()
{
    CClipboard clipboard(nullptr);
    auto rootNode = XmlHelpers::CreateXmlNode("PropertyCtrl");
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->OnCopyAll(rootNode);
    }
    clipboard.Put(rootNode);
}

void TwoColumnPropertyControl::OnPaste()
{
    CClipboard clipboard(nullptr);

    CUndo undo("Paste Properties");

    XmlNodeRef rootNode = clipboard.Get();
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetValuesFromNode(rootNode);
    }
}


PropertyCard::PropertyCard([[maybe_unused]] QWidget* parent /*= nullptr*/)
{
    // create header bar
    m_header = new AzToolsFramework::ComponentEditorHeader(this);
    m_header->SetExpandable(true);

    // create property editor
    m_propertyEditor = new ReflectedPropertyControl(this);
    m_propertyEditor->Setup(false);
    m_propertyEditor->GetEditor()->SetHideRootProperties(true);
    m_propertyEditor->setProperty("ComponentDisabl", true); // used by stylesheet

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_header);
    mainLayout->addWidget(m_propertyEditor);
    setLayout(mainLayout);

    connect(m_header, &AzToolsFramework::ComponentEditorHeader::OnExpanderChanged, this, &PropertyCard::OnExpanderChanged);
    connect(m_propertyEditor->GetEditor(), &AzToolsFramework::ReflectedPropertyEditor::OnExpansionContractionDone, this, &PropertyCard::OnExpansionContractionDone);

    SetExpanded(true);
}

void PropertyCard::AddVarBlock(CVarBlock *varBlock)
{
    if (varBlock->GetNumVariables() > 0)
    {
        m_header->SetTitle(varBlock->GetVariable(0)->GetName());
        m_propertyEditor->AddVarBlock(varBlock);
    }
}

ReflectedPropertyControl* PropertyCard::GetControl()
{
    return m_propertyEditor;
}

void PropertyCard::SetExpanded(bool expanded)
{
    m_header->SetExpanded(expanded);
    m_propertyEditor->setVisible(expanded);
}

bool PropertyCard::IsExpanded() const
{
    return m_header->IsExpanded();
}

void PropertyCard::OnExpanderChanged(bool expanded)
{
    SetExpanded(expanded);
    emit OnExpansionContractionDone();
}

#include <Controls/ReflectedPropertyControl/moc_ReflectedPropertyCtrl.cpp>

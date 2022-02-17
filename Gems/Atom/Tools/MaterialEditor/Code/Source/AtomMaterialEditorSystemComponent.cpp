
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>

#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>
#include <AtomToolsFramework/Util/Util.h>

#include <AtomMaterialEditorSystemComponent.h>

#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialEditorWindowSettings.h>
#include <Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Document/MaterialDocument.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

namespace AtomMaterialEditor
{
    static const char* GetBuildTargetName()
    {
        return "MaterialEditor";
    }

    void AtomMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomToolsFramework::AtomToolsDocumentSystem::Reflect(context);
        MaterialEditor::MaterialEditorWindowSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomMaterialEditorSystemComponent>("AtomMaterialEditorSystemComponent", "System component that manages launching and maintaining connections the material editor.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialEditor::MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetPropertyValue", &MaterialEditor::MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("GetPropertyValue", &MaterialEditor::MaterialDocumentRequestBus::Events::GetPropertyValue);
        }
    }

    AtomMaterialEditorSystemComponent::AtomMaterialEditorSystemComponent()
        : m_targetName(GetBuildTargetName())
        , m_toolId(GetBuildTargetName())
        , m_notifyRegisterViewsEventHandler([this]() { RegisterAtomWindow(); })
    {
    }

    AtomMaterialEditorSystemComponent::~AtomMaterialEditorSystemComponent() = default;

    void AtomMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomMaterialEditorSystemComponent::Activate()
    {
        if (auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get())
        {
            o3deMaterialEditor->ConnectNotifyRegisterViewsEventHandler(m_notifyRegisterViewsEventHandler);
        }
    }

    void AtomMaterialEditorSystemComponent::Deactivate()
    {
        m_notifyRegisterViewsEventHandler.Disconnect();
        m_assetBrowserInteractions.reset();
        m_documentSystem.reset();
    }

    void AtomMaterialEditorSystemComponent::RegisterAtomWindow()
    {
        m_documentSystem.reset(aznew AtomToolsFramework::AtomToolsDocumentSystem(m_toolId));
        m_documentSystem->RegisterDocumentType(
            [](const AZ::Crc32& toolId) { return aznew MaterialEditor::MaterialDocument(toolId); });

        SetupAssetBrowserInteractions();

        auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get();
        if (o3deMaterialEditor)
        {
            O3DEMaterialEditor::WidgetCreationFunc windowCreationFunc =
                [toolId = m_toolId](QWidget* parent = nullptr)
            {
                return new MaterialEditor::MaterialEditorWindow(toolId, parent);
            };

            o3deMaterialEditor->RegisterViewPane("Render Materials", AZStd::move(windowCreationFunc));
        }
    }

    void AtomMaterialEditorSystemComponent::SetupAssetBrowserInteractions()
    {
        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            [this]([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                const bool isMaterial = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialSourceData::Extension);
                const bool isMaterialType = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialTypeSourceData::Extension);
                if (isMaterial || isMaterialType)
                {
                    menu->addAction(QObject::tr("Open"), [entries, this]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });

                    const QString createActionName =
                        isMaterialType ? QObject::tr("Create Material...") : QObject::tr("Create Child Material...");

                    menu->addAction(createActionName, [entries, this]()
                        {
                            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                                QString(AZ::Utils::GetProjectPath().c_str()) +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets" +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                                AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                entries.front()->GetFullPath(),
                                AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
                        });
                }
                else
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                        });
                }
            });

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder;
            },
            [this](QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                menu->addAction(QObject::tr("Create Material..."), [caller, entries, this]()
                    {
                        MaterialEditor::CreateMaterialDialog createDialog(entries.front()->GetFullPath().c_str(), caller);
                        createDialog.adjustSize();

                        if (createDialog.exec() == QDialog::Accepted &&
                            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
                            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData(),
                                createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData());
                        }
                    });
            });
    }

} // namespace MaterialEditor

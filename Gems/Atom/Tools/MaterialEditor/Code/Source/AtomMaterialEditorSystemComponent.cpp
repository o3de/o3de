
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>
#include <AtomToolsFramework/Util/Util.h>

#include <AtomMaterialEditorSystemComponent.h>

#include <Window/MaterialEditorWindow.h>
#include <Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Document/MaterialDocument.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

namespace AtomMaterialEditor
{
    void AtomMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    AtomMaterialEditorSystemComponent::AtomMaterialEditorSystemComponent()
        : m_notifyRegisterViewsEventHandler([this]() { RegisterAtomWindow(); })
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
        required.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
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
    }

    void AtomMaterialEditorSystemComponent::RegisterAtomWindow()
    {
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew MaterialEditor::MaterialDocument(); });

        SetupAssetBrowserInteractions();

        O3DEMaterialEditor::RegisterViewPane<MaterialEditor::MaterialEditorWindow>("Render Materials");
    }

    void AtomMaterialEditorSystemComponent::SetupAssetBrowserInteractions()
    {
        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);
        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            []([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                const bool isMaterial = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialSourceData::Extension);
                const bool isMaterialType = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::RPI::MaterialTypeSourceData::Extension);
                if (isMaterial || isMaterialType)
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });

                    const QString createActionName =
                        isMaterialType ? QObject::tr("Create Material...") : QObject::tr("Create Child Material...");

                    menu->addAction(createActionName, [entries]()
                        {
                            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                                QString(AZ::Utils::GetProjectPath().c_str()) +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets" +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                                AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
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
            [](QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                menu->addAction(QObject::tr("Create Material..."), [caller, entries]()
                    {
                        MaterialEditor::CreateMaterialDialog createDialog(entries.front()->GetFullPath().c_str(), caller);
                        createDialog.adjustSize();

                        if (createDialog.exec() == QDialog::Accepted &&
                            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
                            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData(),
                                createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData());
                        }
                    });
            });
    }

} // namespace MaterialEditor

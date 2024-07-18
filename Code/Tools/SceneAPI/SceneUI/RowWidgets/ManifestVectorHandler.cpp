/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL((IManifestVectorHandler, AZ_CLASS), SystemAllocator);

            template<typename ManifestType> SerializeContext* IManifestVectorHandler<ManifestType>::s_serializeContext = nullptr;
            template<typename ManifestType> IManifestVectorHandler<ManifestType>* IManifestVectorHandler<ManifestType>::s_instance = nullptr;

            template<typename ManifestType>
            QWidget* IManifestVectorHandler<ManifestType>::CreateGUI(QWidget* parent)
            {
                if(IManifestVectorHandler::s_serializeContext)
                {
                    ManifestVectorWidget* instance = aznew ManifestVectorWidget(IManifestVectorHandler::s_serializeContext, parent);
                    connect(instance, &ManifestVectorWidget::valueChanged, this,
                        [instance]()
                        {
                            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, instance);
                        });
                    return instance;
                }
                else
                {
                    return nullptr;
                }
            }

            template<typename ManifestType>
            u32 IManifestVectorHandler<ManifestType>::GetHandlerName() const
            {
                return AZ_CRC_CE("ManifestVector");
            }

            template<typename ManifestType>
            bool IManifestVectorHandler<ManifestType>::AutoDelete() const
            {
                return false;
            }

            template<typename ManifestType>
            void IManifestVectorHandler<ManifestType>::ConsumeAttribute(ManifestVectorWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                AZ_TraceContext("Attribute name", debugName);

                if (attrib == AZ_CRC_CE("ObjectTypeName"))
                {
                    AZStd::string name;
                    if (attrValue->Read<AZStd::string>(name))
                    {
                        widget->SetCollectionTypeName(name);
                    }
                }
                else if (attrib == AZ_CRC_CE("CollectionName"))
                {
                    AZStd::string name;
                    if (attrValue->Read<AZStd::string>(name))
                    {
                        widget->SetCollectionName(name);
                    }
                }
                // Sets the number of entries the user can add through this widget. It doesn't limit
                //      the amount of entries that can be stored.
                else if (attrib == AZ_CRC_CE("Cap"))
                {
                    size_t cap;
                    if (attrValue->Read<size_t>(cap))
                    {
                        widget->SetCapSize(cap);
                    }
                }
            }

            template<typename ManifestType>
            void IManifestVectorHandler<ManifestType>::WriteGUIValuesIntoProperty(size_t /*index*/, ManifestVectorWidget* GUI,
                typename IManifestVectorHandler::property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                instance.clear();
                AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject> > manifestVector = GUI->GetManifestVector();
                for (auto& manifestObject : manifestVector)
                {
                    instance.push_back(AZStd::static_pointer_cast<ManifestType>(manifestObject));

                }
            }

            template<typename ManifestType>
            bool IManifestVectorHandler<ManifestType>::ReadValuesIntoGUI(size_t /*index*/, ManifestVectorWidget* GUI, const typename IManifestVectorHandler::property_t& instance,
                AzToolsFramework::InstanceDataNode* node)
            {
                AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();

                if (parentNode && parentNode->GetClassMetadata() && parentNode->GetClassMetadata()->m_azRtti)
                {
                    AZ_TraceContext("Parent UUID", parentNode->GetClassMetadata()->m_azRtti->GetTypeId());
                    if (parentNode->GetClassMetadata()->m_azRtti->IsTypeOf(DataTypes::IManifestObject::RTTI_Type()))
                    {
                        DataTypes::IManifestObject* owner = static_cast<DataTypes::IManifestObject*>(parentNode->FirstInstance());
                        GUI->SetManifestVector(instance.begin(), instance.end(), owner);
                    }
                    else if (parentNode->GetClassMetadata()->m_azRtti->IsTypeOf(Containers::RuleContainer::RTTI_Type()))
                    {
                        AzToolsFramework::InstanceDataNode* manifestObject = parentNode->GetParent();
                        if (manifestObject && manifestObject->GetClassMetadata()->m_azRtti->IsTypeOf(DataTypes::IManifestObject::RTTI_Type()))
                        {
                            DataTypes::IManifestObject* owner = static_cast<DataTypes::IManifestObject*>(manifestObject->FirstInstance());
                            GUI->SetManifestVector(instance.begin(), instance.end(), owner);
                        }
                        else
                        {
                            AZ_TracePrintf(Utilities::WarningWindow, "RuleContainer requires a ManifestObject parent.");
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "ManifestVectorWidget requires a ManifestObject parent.");
                    }
                }
                else
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "ManifestVectorWidget requires valid parent with RTTI data specified");
                }

                return false;
            }

            template<typename ManifestType>
            void IManifestVectorHandler<ManifestType>::Register()
            {
                if (!IManifestVectorHandler::s_instance)
                {
                    IManifestVectorHandler::s_instance = aznew IManifestVectorHandler();
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, IManifestVectorHandler::s_instance);
                    EBUS_EVENT_RESULT(s_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
                    AZ_Assert(s_serializeContext, "Serialization context not available");
                }
            }

            template<typename ManifestType>
            void IManifestVectorHandler<ManifestType>::Unregister()
            {
                if (IManifestVectorHandler::s_instance)
                {
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, UnregisterPropertyType, IManifestVectorHandler::s_instance);
                    delete IManifestVectorHandler::s_instance;
                    IManifestVectorHandler::s_instance = nullptr;
                }
            }

            void ManifestVectorHandler::Register()
            {
                IManifestVectorHandler<DataTypes::IManifestObject>::Register();
                IManifestVectorHandler<DataTypes::IRule>::Register();
            }

            void ManifestVectorHandler::Unregister()
            {
                IManifestVectorHandler<DataTypes::IManifestObject>::Unregister();
                IManifestVectorHandler<DataTypes::IRule>::Unregister();
            }
        } // UI
    } // SceneAPI
} // AZ

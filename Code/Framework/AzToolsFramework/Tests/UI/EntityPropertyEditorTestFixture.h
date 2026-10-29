/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <Prefab/PrefabTestFixture.h>

#include <QApplication>
#include <QElapsedTimer>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The test components: a short card and a tall one
    // ----------------------------------------------------------------------------------

    //! A short card: one number
    class InspectorShortTestComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            InspectorShortTestComponent, "{92DC5E55-5441-43A7-9942-418364F38310}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<InspectorShortTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("value", &InspectorShortTestComponent::m_value);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<InspectorShortTestComponent>("Inspector Short Test", "A short card")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorShortTestComponent::m_value, "Value", "");
                }
            }
        }

        float m_value = 0.0f;
    };

    //! A tall card: eight numbers
    class InspectorTallTestComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            InspectorTallTestComponent, "{7B1E7A2C-0C5D-4D5E-9E3B-5B8E6E2F4A11}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<InspectorTallTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("value0", &InspectorTallTestComponent::m_value0)
                    ->Field("value1", &InspectorTallTestComponent::m_value1)
                    ->Field("value2", &InspectorTallTestComponent::m_value2)
                    ->Field("value3", &InspectorTallTestComponent::m_value3)
                    ->Field("value4", &InspectorTallTestComponent::m_value4)
                    ->Field("value5", &InspectorTallTestComponent::m_value5)
                    ->Field("value6", &InspectorTallTestComponent::m_value6)
                    ->Field("value7", &InspectorTallTestComponent::m_value7);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<InspectorTallTestComponent>("Inspector Tall Test", "A tall card")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value0, "Value 0", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value1, "Value 1", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value2, "Value 2", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value3, "Value 3", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value4, "Value 4", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value5, "Value 5", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value6, "Value 6", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &InspectorTallTestComponent::m_value7, "Value 7", "");
                }
            }
        }

        float m_value0 = 0.0f;
        float m_value1 = 1.0f;
        float m_value2 = 2.0f;
        float m_value3 = 3.0f;
        float m_value4 = 4.0f;
        float m_value5 = 5.0f;
        float m_value6 = 6.0f;
        float m_value7 = 7.0f;
    };

    // ----------------------------------------------------------------------------------
    // The fixture: the inspector a test shows, its selections, the event loop, and the console variables the test sets
    // ----------------------------------------------------------------------------------

    class EntityPropertyEditorTestFixture : public PrefabTestFixture
    {
    protected:
        void TearDownEditorFixtureImpl() override
        {
            m_inspector.reset();
            for (const auto& [name, value] : m_cvarValues)
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(AZStd::string::format("%s %s", name.c_str(), value.c_str()).c_str());
            }
            m_cvarValues.clear();
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        //! Sets a console variable for the test: its end gives the variable back the value it had
        void SetCvar(const char* name, bool value)
        {
            bool original = false;
            AZ::Interface<AZ::IConsole>::Get()->GetCvarValue(name, original);
            SetCvarText(name, value ? "true" : "false", original ? "true" : "false");
        }

        void SetCvar(const char* name, int value)
        {
            int original = 0;
            AZ::Interface<AZ::IConsole>::Get()->GetCvarValue(name, original);
            SetCvarText(name, AZStd::string::format("%d", value).c_str(), AZStd::string::format("%d", original).c_str());
        }

        //! An entity under the root with the transform and count components of the given type
        template<class Component>
        AZ::EntityId CreateEntityOf(const char* name, int count)
        {
            const AZ::EntityId entityId = CreateEditorEntityUnderRoot(name);
            AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
            entity->Deactivate();
            for (int index = 0; index < count; ++index)
            {
                entity->CreateComponent<Component>()->SetSerializedIdentifier(AZStd::string::format("%s_%d", name, index));
            }
            entity->Activate();
            return entityId;
        }

        //! A new inspector of the given height, shown (never on screen)
        void NewInspector(int windowHeight = 320)
        {
            m_inspector.reset();
            m_inspector = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
            m_inspector->setAttribute(Qt::WA_DontShowOnScreen);
            m_inspector->resize(420, windowHeight);
            m_inspector->show();
            QApplication::processEvents();
        }

        //! A selection change as the outliner or the viewport makes it: the inspector queues its rebuild
        static void SelectQueued(const AzToolsFramework::EntityIdList& entityIds)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        }

        //! The rebuild a new selection runs (it returns here, before the event loop runs again)
        void Select(const AzToolsFramework::EntityIdList& entityIds)
        {
            SelectQueued(entityIds);
            QMetaObject::invokeMethod(m_inspector.get(), "UpdateContents", Qt::DirectConnection);
        }

        //! The event loop's next turns: what it lays out, shows or rebuilds after the inspector painted
        static void RunTheEventLoop()
        {
            QElapsedTimer timer;
            timer.start();
            while (timer.elapsed() < 100)
            {
                QApplication::processEvents();
            }
        }

        //! The event loop's turns until no card holds its rows (the idle steps), within a bound
        void RunTheIdleSteps()
        {
            for (int turn = 0; turn < 1000 && m_inspector->GetLazyCardsPending() > 0; ++turn)
            {
                QApplication::processEvents();
            }
        }

        //! The inspector's shown cards, top to bottom
        AZStd::vector<AzToolsFramework::ComponentEditor*> Cards() const
        {
            AZStd::vector<AzToolsFramework::ComponentEditor*> cards;
            for (AzToolsFramework::ComponentEditor* card : m_inspector->findChildren<AzToolsFramework::ComponentEditor*>())
            {
                if (card->isVisible())
                {
                    cards.push_back(card);
                }
            }
            AZStd::sort(cards.begin(), cards.end(),
                [](const QWidget* a, const QWidget* b)
                {
                    return a->geometry().y() < b->geometry().y();
                });
            return cards;
        }

        AZStd::unique_ptr<AzToolsFramework::EntityPropertyEditor> m_inspector;

    private:
        void SetCvarText(const char* name, const char* value, const char* original)
        {
            m_cvarValues.emplace(name, original);   // the value before the test's first change: emplace keeps it
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(AZStd::string::format("%s %s", name, value).c_str());
        }

        AZStd::unordered_map<AZStd::string, AZStd::string> m_cvarValues;   //!< the cvars the test set, at their values before it
    };
} // namespace UnitTest

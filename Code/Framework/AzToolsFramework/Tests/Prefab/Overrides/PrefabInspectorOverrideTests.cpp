/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/Widgets/IconElidingLabel.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <Prefab/Overrides/PrefabInspectorOverrideTestFixture.h>

#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>

namespace UnitTest
{
    using PrefabInspectorOverrideTest = PrefabInspectorOverrideTestFixture;

    namespace PrefabInspectorOverrideTestInternal
    {
        //! What the DPE's Tab stops on (DocumentPropertyEditor.cpp IsInputWidget). A widget whose focus goes to another (the line
        //! edit of a spin box) is not a stop of its own.
        bool IsTabStop(const QWidget* widget)
        {
            const bool input = qobject_cast<const QAbstractSpinBox*>(widget) || qobject_cast<const QComboBox*>(widget) ||
                qobject_cast<const QCheckBox*>(widget) || qobject_cast<const QLineEdit*>(widget) || qobject_cast<const QPushButton*>(widget);
            return input && !widget->focusProxy();
        }

        //! The property editor of the inspector's one card
        const AzToolsFramework::DocumentPropertyEditor* FindPropertyEditor()
        {
            AZStd::vector<const AzToolsFramework::ComponentEditor*> componentEditors;
            AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                &AzToolsFramework::EntityPropertyEditorRequestBus::Events::VisitComponentEditors,
                [&componentEditors](const AzToolsFramework::ComponentEditor* componentEditor)
                {
                    componentEditors.push_back(componentEditor);
                    return true;
                });
            return componentEditors.size() == 1 ? componentEditors.front()->findChild<AzToolsFramework::DocumentPropertyEditor*>() : nullptr;
        }

        //! Tab from the card's first stop until the walk leaves the card: each stop as the index of its widget among the card's
        //! stops (in the order the card made them), and -1 for the widget outside the card where the walk ends
        AZStd::vector<int> WalkTheTabChain(QWidget& window, const AzToolsFramework::DocumentPropertyEditor& propertyEditor)
        {
            QList<QWidget*> stops;
            for (QWidget* widget : propertyEditor.findChildren<QWidget*>())
            {
                if (IsTabStop(widget) && widget->isVisibleTo(&window) && widget->isEnabled())
                {
                    stops.push_back(widget);
                }
            }

            AZStd::vector<int> walk;
            if (stops.isEmpty())
            {
                return walk;
            }
            stops.front()->setFocus(Qt::TabFocusReason);
            for (int step = 0; step <= stops.size(); ++step)
            {
                const int index = aznumeric_cast<int>(stops.indexOf(window.focusWidget()));
                walk.push_back(index);
                if (index < 0)
                {
                    break;
                }
                QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
                QApplication::sendEvent(window.focusWidget(), &tab);
            }
            return walk;
        }
    } // namespace PrefabInspectorOverrideTestInternal

    // The Tab chain of a real card, with the composite override label and with the compact one (ed_dpeCompactOverrideLabel): the
    // DPE's Tab (DocumentPropertyEditor::focusNextPrevChild) stops on the input widgets only (the X, Y and Z of a vector, a field's
    // browse button), and the compact label leaves that walk as the composite's tool button and label left it
    TEST_F(PrefabInspectorOverrideTest, TheTabChainOfACardIsTheSameWithTheCompactOverrideLabel)
    {
        using namespace PrefabInspectorOverrideTestInternal;

        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);
        CreateAndValidateEditEntityOverride(newEntityId, grandparentContainerId);  // the Translate row's label has the override dot

        auto* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(console, nullptr);

        AZStd::vector<int> walks[2];
        for (const bool compact : { false, true })
        {
            console->PerformCommand(compact ? "ed_dpeCompactOverrideLabel true" : "ed_dpeCompactOverrideLabel false");

            // a new inspector, shown (a hidden window has no Tab chain): its rows are made with the cvar as it is now
            m_testEntityPropertyEditor = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
            m_testEntityPropertyEditor->setAttribute(Qt::WA_DontShowOnScreen);
            m_testEntityPropertyEditor->resize(400, 900);
            m_testEntityPropertyEditor->show();
            GenerateComponentAdapterDoms(newEntityId);
            QApplication::processEvents();
            // the rebuild the selection queued runs in that turn, and builds its cards' rows in idle steps (interruptible load):
            // the walk needs them all
            m_testEntityPropertyEditor->CompleteLazyCards();

            const auto* propertyEditor = FindPropertyEditor();
            ASSERT_NE(propertyEditor, nullptr);
            EXPECT_EQ(propertyEditor->findChildren<AzQtComponents::IconElidingLabel*>().isEmpty(), !compact) << "compact " << compact;
            walks[compact] = WalkTheTabChain(*m_testEntityPropertyEditor, *propertyEditor);
        }
        console->PerformCommand("ed_dpeCompactOverrideLabel true");   // the default

        // Every Tab moves on to the next stop of the card (three at least: the X, Y and Z of Translate) until it leaves the card,
        // and the walk is the same both ways
        const AZStd::vector<int>& walk = walks[0];
        ASSERT_GE(walk.size(), 4);
        for (size_t step = 0; step + 1 < walk.size(); ++step)
        {
            EXPECT_GE(walk[step], 0) << "step " << step;
            EXPECT_TRUE(AZStd::find(walk.begin() + step + 1, walk.end(), walk[step]) == walk.end())
                << "step " << step << " stops again on " << walk[step];
        }
        EXPECT_EQ(walks[1], walks[0]);
    }

    // The fixture's inspector is a real one (DPE inspector, override management): it also proves what a rebuild leaves behind
    TEST_F(PrefabInspectorOverrideTest, RebuildsDoNotGrowTheLayoutOfAReusedPropertyEditor)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        // three rebuilds: the inspector reuses its component editor, and the property editor in it, every time
        for (int rebuild = 0; rebuild < 3; ++rebuild)
        {
            GenerateComponentAdapterDoms(newEntityId);
        }

        const auto* propertyEditor = PrefabInspectorOverrideTestInternal::FindPropertyEditor();
        ASSERT_NE(propertyEditor, nullptr);     // the inspector's one card has one

        // the rows of the last rebuild and the one stretch below them
        const QLayout* rowLayout = propertyEditor->widget()->layout();
        int rows = 0;
        int stretches = 0;
        for (int itemIndex = 0; itemIndex < rowLayout->count(); ++itemIndex)
        {
            QLayoutItem* item = rowLayout->itemAt(itemIndex);
            rows += item->widget() ? 1 : 0;
            stretches += item->spacerItem() ? 1 : 0;
        }
        EXPECT_GT(rows, 0);
        EXPECT_EQ(stretches, 1);
        EXPECT_TRUE(rowLayout->itemAt(rowLayout->count() - 1)->spacerItem() != nullptr);
    }

    // These index paths depend on multiple factors like the data in the component, how its reflected to serialize and edit contexts,
    // how different DPE adapters likes ReflectionAdapter and PrefabAdapter construct the DPE DOM etc. Therefore, these may
    // change in the future if the data stored in the DPE DOM itself changes and need to be modified accordingly to prevent test failures.
    constexpr AZStd::string_view pathToTranslateRow = "/1/2";

    TEST_F(PrefabInspectorOverrideTest, ValidatePresenceOfOverrideProperty)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        CreateAndValidateEditEntityOverride(newEntityId, grandparentContainerId);

        GenerateComponentAdapterDoms(newEntityId);

        AzToolsFramework::ComponentEditor::VisitComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;
            using AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideLabel;

            EXPECT_FALSE(adapterContents.IsArrayEmpty());

            // Validate the translate row has overridden data.
            AZ::Dom::Path domPathToTranslateRow(pathToTranslateRow);
            AZ::Dom::Value translateRow = adapterContents[domPathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Value.GetName()].GetString(), "Translate");
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RelativePath.GetName()].GetString().empty());
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RevertOverride.GetName()].IsNull());

            EXPECT_TRUE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);
        };

        ValidateComponentEditorDomContents(callback);
    }

    TEST_F(PrefabInspectorOverrideTest, ValidateAbsenceOfOverrideProperty)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        EditEntityAndValidateNoOverride(newEntityId);
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(grandparentContainerId);

        GenerateComponentAdapterDoms(newEntityId);

        AzToolsFramework::ComponentEditor::VisitComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;
            using AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideLabel;

            EXPECT_FALSE(adapterContents.IsArrayEmpty());

            // Validate the translate row has no overridden data.
            AZ::Dom::Path domPathToTranslateRow(pathToTranslateRow);
            AZ::Dom::Value translateRow = adapterContents[domPathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Value.GetName()].GetString(), "Translate");
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RelativePath.GetName()].GetString().empty());
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RevertOverride.GetName()].IsNull());

            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);
        };

        ValidateComponentEditorDomContents(callback);
    }

} // namespace UnitTest

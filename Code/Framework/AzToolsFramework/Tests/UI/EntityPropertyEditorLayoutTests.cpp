/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <UI/EntityPropertyEditorTestFixture.h>

#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The inspector's component list is laid out when an expanded or collapsed row returns, and (with ed_inspectorSettleLayout) when a
    // rebuild returns, before the inspector paints: no row shows before it is placed, and no card shows where the previous
    // selection's card was (the next turns of the event loop move nothing).
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorLayoutTestsInternal
    {
        //! Six numbers under a row of their own, which expands and collapses
        struct LayoutNestedTestData
        {
            AZ_TYPE_INFO(LayoutNestedTestData, "{A5F813C1-6433-456F-95BB-B77DECAD3845}");

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<LayoutNestedTestData>()
                        ->Version(1)
                        ->Field("a", &LayoutNestedTestData::m_a)
                        ->Field("b", &LayoutNestedTestData::m_b)
                        ->Field("c", &LayoutNestedTestData::m_c)
                        ->Field("d", &LayoutNestedTestData::m_d)
                        ->Field("e", &LayoutNestedTestData::m_e)
                        ->Field("f", &LayoutNestedTestData::m_f);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<LayoutNestedTestData>("Layout Nested Test Data", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_a, "A", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_b, "B", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_c, "C", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_d, "D", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_e, "E", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutNestedTestData::m_f, "F", "");
                    }
                }
            }

            float m_a = 0.0f;
            float m_b = 1.0f;
            float m_c = 2.0f;
            float m_d = 3.0f;
            float m_e = 4.0f;
            float m_f = 5.0f;
        };

        //! A tall card: two numbers and the nested ones
        class LayoutTallTestComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(
                LayoutTallTestComponent, "{8C215202-8E3B-4D0C-89F6-EF494DEC2477}", AzToolsFramework::Components::EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context)
            {
                LayoutNestedTestData::Reflect(context);
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<LayoutTallTestComponent, AzToolsFramework::Components::EditorComponentBase>()
                        ->Version(1)
                        ->Field("value0", &LayoutTallTestComponent::m_value0)
                        ->Field("value1", &LayoutTallTestComponent::m_value1)
                        ->Field("nested", &LayoutTallTestComponent::m_nested);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<LayoutTallTestComponent>("Layout Tall Test", "A tall card")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutTallTestComponent::m_value0, "Value 0", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutTallTestComponent::m_value1, "Value 1", "")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LayoutTallTestComponent::m_nested, "Nested", "");
                    }
                }
            }

            float m_value0 = 0.0f;
            float m_value1 = 1.0f;
            LayoutNestedTestData m_nested;
        };

        //! Where a card and its rows are
        struct CardPlace
        {
            QRect m_geometry;           //!< in the component list
            int m_rowsScroll = 0;       //!< the card's rows scrolled inside the card
            QList<QRect> m_rows;        //!< the shown rows, in the card's rows
        };
    } // namespace EntityPropertyEditorLayoutTestsInternal

    class EntityPropertyEditorLayoutTests : public EntityPropertyEditorTestFixture
    {
    protected:
        using LayoutTallTestComponent = EntityPropertyEditorLayoutTestsInternal::LayoutTallTestComponent;
        using CardPlace = EntityPropertyEditorLayoutTestsInternal::CardPlace;

        static constexpr int TallComponents = 4;

        void SetUpEditorFixtureImpl() override
        {
            EntityPropertyEditorTestFixture::SetUpEditorFixtureImpl();
            GetApplication()->RegisterComponentDescriptor(InspectorShortTestComponent::CreateDescriptor());
            GetApplication()->RegisterComponentDescriptor(LayoutTallTestComponent::CreateDescriptor());

            // the transform and a short card, the transform and four tall cards: the cards that show one entity after the other are
            // reused for different components
            m_shortEntityId = CreateEntityOf<InspectorShortTestComponent>("LayoutShortEntity", 1);
            m_tallEntityId = CreateEntityOf<LayoutTallTestComponent>("LayoutTallEntity", TallComponents);
        }

        //! A new inspector of the given height, shown, with lazy cards on or off, showing the entity
        void Show(bool lazy, int windowHeight, AZ::EntityId entityId)
        {
            SetCvar("ed_inspectorLazyCards", lazy);
            NewInspector(windowHeight);
            Select({ entityId });
            RunTheEventLoop();
        }

        //! Where the cards and their rows are now
        AZStd::vector<CardPlace> Places() const
        {
            AZStd::vector<CardPlace> places;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                CardPlace place;
                place.m_geometry = card->geometry();
                if (const auto* rows = card->findChild<AzToolsFramework::DocumentPropertyEditor*>())
                {
                    place.m_rowsScroll = rows->verticalScrollBar()->value();
                    for (const QWidget* row : rows->widget()->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
                    {
                        if (row->isVisible())
                        {
                            place.m_rows.push_back(row->geometry());
                        }
                    }
                }
                places.push_back(AZStd::move(place));
            }
            return places;
        }

        //! The first n places stay where they are when the event loop runs
        static void ExpectTheSamePlaces(const AZStd::vector<CardPlace>& expected, const AZStd::vector<CardPlace>& actual, size_t count)
        {
            ASSERT_GE(expected.size(), count);
            ASSERT_GE(actual.size(), count);
            for (size_t index = 0; index < count; ++index)
            {
                EXPECT_EQ(actual[index].m_geometry, expected[index].m_geometry) << "card " << index;
                EXPECT_EQ(actual[index].m_rowsScroll, expected[index].m_rowsScroll) << "card " << index;
                EXPECT_EQ(actual[index].m_rows, expected[index].m_rows) << "card " << index;
            }
        }

        //! No card over the one above it
        static void ExpectNoOverlap(const AZStd::vector<CardPlace>& places)
        {
            for (size_t index = 1; index < places.size(); ++index)
            {
                EXPECT_GT(places[index].m_geometry.top(), places[index - 1].m_geometry.bottom()) << "card " << index;
            }
        }

        //! The cards at the top that build their rows in the rebuild (with lazy cards the ones below build theirs later and grow)
        size_t CardsBuilt() const
        {
            size_t built = 0;
            for (const AzToolsFramework::ComponentEditor* card : Cards())
            {
                if (card->IsContentBuildHeld())
                {
                    break;
                }
                ++built;
            }
            return built;
        }

        //! The expander of the "Nested" row in the second card (the first tall card); its rows are not its children but the rows'
        //! (the property editor lays all rows out in one list)
        QCheckBox* NestedExpander() const
        {
            const AZStd::vector<AzToolsFramework::ComponentEditor*> cards = Cards();
            if (cards.size() < 2)
            {
                return nullptr;
            }
            for (AzToolsFramework::DPERowWidget* row : cards[1]->findChildren<AzToolsFramework::DPERowWidget*>())
            {
                for (const AzQtComponents::ElidingLabel* label : row->findChildren<AzQtComponents::ElidingLabel*>())
                {
                    if (row->HasChildRows() && label->text() == "Nested")
                    {
                        return row->findChild<QCheckBox*>(QString(), Qt::FindDirectChildrenOnly);
                    }
                }
            }
            return nullptr;
        }

        AZ::EntityId m_shortEntityId;
        AZ::EntityId m_tallEntityId;
    };

    TEST_F(EntityPropertyEditorLayoutTests, TheCardsAreWhereTheyStayWhenARebuildReturnsWithSettleLayout)
    {
        SetCvar("ed_inspectorSettleLayout", true);
        for (const bool lazy : { false, true })
        {
            SCOPED_TRACE(lazy ? "lazy cards" : "full build");

            // the short entity's cards are reused for the tall entity's components, then the tall one's for the short one's
            Show(lazy, 320, m_shortEntityId);
            for (const AZ::EntityId next : { m_tallEntityId, m_shortEntityId })
            {
                Select({ next });
                const AZStd::vector<CardPlace> atReturn = Places();
                const size_t built = CardsBuilt();
                ASSERT_GE(built, 2u);
                ExpectNoOverlap(atReturn);

                RunTheEventLoop();
                ExpectTheSamePlaces(atReturn, Places(), built);
            }
        }
    }

    TEST_F(EntityPropertyEditorLayoutTests, AnExpandedOrCollapsedRowIsPlacedWhenTheClickReturns)
    {
        // a window tall enough for every card
        Show(false, 4000, m_tallEntityId);
        QCheckBox* expander = NestedExpander();
        ASSERT_NE(expander, nullptr);
        ASSERT_TRUE(expander->isChecked());     // a nested row is expanded by default

        const qsizetype rowsExpanded = Places()[1].m_rows.size();
        for (const bool expanded : { false, true })
        {
            SCOPED_TRACE(expanded ? "expanded" : "collapsed");

            expander->click();
            const AZStd::vector<CardPlace> atReturn = Places();
            ExpectNoOverlap(atReturn);

            RunTheEventLoop();
            const AZStd::vector<CardPlace> settled = Places();
            ExpectTheSamePlaces(atReturn, settled, settled.size());
            EXPECT_EQ(expander->isChecked(), expanded);

            // the six nested rows go and come back
            EXPECT_EQ(settled[1].m_rows.size(), expanded ? rowsExpanded : rowsExpanded - 6);
        }
    }
} // namespace UnitTest

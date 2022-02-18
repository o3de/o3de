
#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QStylePainter>
#include <QStyleOptionTab>

#include <O3DEMaterialEditorWidget.h>
#include <O3DEMaterialEditorSystemComponent.h>

namespace O3DEMaterialEditor
{
    namespace
    {
        class RotatedTabBar : public AzQtComponents::TabBar
        {
        public:
            explicit RotatedTabBar(QWidget* parent = nullptr)
                : AzQtComponents::TabBar(parent)
            {
            }

            QSize tabSizeHint([[maybe_unused]] int index) const override
            {
                return QSize(45, 55);
            }

        protected:
            void paintEvent([[maybe_unused]] QPaintEvent* event) override
            {
                QStylePainter painter(this);
                QStyleOptionTab optionTab;

                for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
                {
                    initStyleOption(&optionTab, tabIndex);
                    painter.drawControl(QStyle::CE_TabBarTabShape, optionTab);
                    painter.save();

                    QSize size = optionTab.rect.size();
                    size.transpose();
                    QRect rect(QPoint(), size);
                    rect.moveCenter(optionTab.rect.center());
                    optionTab.rect = rect;

                    QPoint center = tabRect(tabIndex).center();
                    painter.translate(center);
                    painter.rotate(90);
                    painter.translate(-center);
                    painter.drawControl(QStyle::CE_TabBarTabLabel, optionTab);
                    painter.restore();
                }
            }
        };
    }

    O3DEMaterialEditorWidget::O3DEMaterialEditorWidget(QWidget* parent)
        : AzQtComponents::TabWidget(parent)
    {
        auto tabBar = new RotatedTabBar(this);
        setCustomTabBar(tabBar);

        AzQtComponents::TabWidget::applyVerticalStyle(this, false/*bordered*/);

        setTabPosition(QTabWidget::West);
        setIconSize(QSize(33, 33));
        setMovable(false); // RotatedTabBar is not working well while reordering tabs at the moment.

        if (auto* o3deMaterialEditorSystem = GetO3DEMaterialEditorSystem())
        {
            const auto& registeredTabs = o3deMaterialEditorSystem->GetRegisteredTabs();
            for (const auto& registeredTab : registeredTabs)
            {
                // Have render material tab first
                if (registeredTab.m_name == "Render Materials")
                {
                    insertTab(0, registeredTab.m_widgetCreationFunc(nullptr), QIcon(registeredTab.m_icon.c_str()), "");
                    setCurrentIndex(0);
                    setTabToolTip(0, registeredTab.m_name.c_str());
                }
                else
                {
                    addTab(registeredTab.m_widgetCreationFunc(nullptr), QIcon(registeredTab.m_icon.c_str()), "");
                    setTabToolTip(count()-1, registeredTab.m_name.c_str());
                }
            }
        }

        // if no systems registered views, add a tab to let the user know.
        if (count() == 0)
        {
            QWidget* noWindowsTab = new QWidget();
            {
                QVBoxLayout* mainLayout = new QVBoxLayout(noWindowsTab);

                QLabel* introLabel = new QLabel("No material editors registered. Enable gems that provide material editors.", noWindowsTab);
                mainLayout->addWidget(introLabel, 0/*stretch*/, Qt::AlignCenter);

                noWindowsTab->setLayout(mainLayout);
            }
            addTab(noWindowsTab, "");
        }
    }
}

#include <moc_O3DEMaterialEditorWidget.cpp>

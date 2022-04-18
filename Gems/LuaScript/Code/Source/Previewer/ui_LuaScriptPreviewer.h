/********************************************************************************
** Form generated from reading UI file 'LuaScriptPreviewer.ui'
**
** Created by: Qt User Interface Compiler version 6.1.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LUASCRIPTPREVIEWER_H
#define UI_LUASCRIPTPREVIEWER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LuaScriptPreviewerClass
{
public:
    QVBoxLayout *verticalLayout_2;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout;
    QLabel *m_previewText;

    void setupUi(QWidget *LuaScriptPreviewerClass)
    {
        if (LuaScriptPreviewerClass->objectName().isEmpty())
            LuaScriptPreviewerClass->setObjectName(QString::fromUtf8("LuaScriptPreviewerClass"));
        LuaScriptPreviewerClass->resize(207, 275);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(LuaScriptPreviewerClass->sizePolicy().hasHeightForWidth());
        LuaScriptPreviewerClass->setSizePolicy(sizePolicy);
        LuaScriptPreviewerClass->setMinimumSize(QSize(0, 0));
        LuaScriptPreviewerClass->setMaximumSize(QSize(16777215, 16777215));
        verticalLayout_2 = new QVBoxLayout(LuaScriptPreviewerClass);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        scrollArea = new QScrollArea(LuaScriptPreviewerClass);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setEnabled(true);
        sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
        scrollArea->setSizePolicy(sizePolicy);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 187, 255));
        verticalLayout = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        m_previewText = new QLabel(scrollAreaWidgetContents);
        m_previewText->setObjectName(QString::fromUtf8("m_previewText"));
        m_previewText->setLayoutDirection(Qt::LeftToRight);
        m_previewText->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout->addWidget(m_previewText);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_2->addWidget(scrollArea);


        retranslateUi(LuaScriptPreviewerClass);

        QMetaObject::connectSlotsByName(LuaScriptPreviewerClass);
    } // setupUi

    void retranslateUi(QWidget *LuaScriptPreviewerClass)
    {
        LuaScriptPreviewerClass->setWindowTitle(QCoreApplication::translate("LuaScriptPreviewerClass", "Form", nullptr));
        m_previewText->setText(QCoreApplication::translate("LuaScriptPreviewerClass", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LuaScriptPreviewerClass: public Ui_LuaScriptPreviewerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LUASCRIPTPREVIEWER_H

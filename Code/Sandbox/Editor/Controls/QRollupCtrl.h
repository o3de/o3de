#ifndef CRYINCLUDE_EDITOR_CONTROLS_QROLLUPCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_QROLLUPCTRL_H

/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */


#if !defined(Q_MOC_RUN)
#include <QFrame>
#include <QScrollArea>
#include <QIcon>
#endif

class QVBoxLayout;
class QRollupCtrlButton;

class QRollupCtrl
    : public QScrollArea
{
    Q_OBJECT
    Q_PROPERTY(int count READ count)

public:
    explicit QRollupCtrl(QWidget* parent = 0);
    ~QRollupCtrl();

    int addItem(QWidget* widget, const QString& text);
    int addItem(QWidget* widget, const QIcon& icon, const QString& text);
    int insertItem(int index, QWidget* widget, const QString& text);
    int insertItem(int index, QWidget* widget, const QIcon& icon, const QString& text);

    void clear();
    void removeItem(QWidget* widget);
    void removeItem(int index);

    void setItemEnabled(int index, bool enabled);
    bool isItemEnabled(int index) const;

    void setItemText(int index, const QString& text);
    QString itemText(int index) const;

    void setItemIcon(int index, const QIcon& icon);
    QIcon itemIcon(int index) const;

    void setItemToolTip(int index, const QString& toolTip);
    QString itemToolTip(int index) const;

    QWidget* widget(int index) const;
    int indexOf(QWidget* widget) const;
    int count() const;

    void readSettings (const QString& qSettingsGroup);
    void writeSettings(const QString& qSettingsGroup);

public slots:
    void setIndexVisible(int index, bool visible);
    void setWidgetVisible(QWidget* widget, bool visible);
    void expandAllPages(bool v);

protected:
    virtual void itemInserted(int index);
    virtual void itemRemoved(int index);
    void changeEvent(QEvent*) override;
    void showEvent(QShowEvent*) override;

private:
    Q_DISABLE_COPY(QRollupCtrl)

    struct Page
    {
        QRollupCtrlButton* button;
        QFrame* sv;
        QWidget* widget;

        void setText(const QString& text);
        void setIcon(const QIcon& is);
        void setToolTip(const QString& tip);
        QString text() const;
        QIcon icon() const;
        QString toolTip() const;

        inline bool operator==(const Page& other) const
        {
            return widget == other.widget;
        }
    };
    typedef QList<Page> PageList;

    Page* page(QWidget* widget) const;
    const Page* page(int index) const;
    Page* page(int index);

    void updateTabs();
    void relayout();
    bool isPageHidden(int index, QString& qObjectName) const;

    QWidget*        m_body;
    PageList        m_pageList;
    QVBoxLayout*   m_layout;

private slots:
    void _q_buttonClicked();
    void _q_widgetDestroyed(QObject*);
    void _q_custumButtonMenu(const QPoint&);
};


//////////////////////////////////////////////////////////////////////////

inline int QRollupCtrl::addItem(QWidget* item, const QString& text)
{ return insertItem(-1, item, QIcon(), text); }
inline int QRollupCtrl::addItem(QWidget* item, const QIcon& iconSet, const QString& text)
{ return insertItem(-1, item, iconSet, text); }
inline int QRollupCtrl::insertItem(int index, QWidget* item, const QString& text)
{ return insertItem(index, item, QIcon(), text); }

#endif

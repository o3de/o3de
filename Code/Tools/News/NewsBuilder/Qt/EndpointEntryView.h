/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class EndpointEntryViewWidget;
}

namespace News
{
    class Endpoint;

    //! Qt widget representing a single endpoint entry
    class EndpointEntryView
        : public QWidget
    {
        Q_OBJECT
    public:
        EndpointEntryView(QWidget* parent, Endpoint* pEndpoint);
        ~EndpointEntryView();

        void SetSelected(bool selected);

        Endpoint* GetEndpoint() const;

    Q_SIGNALS:
        void selectSignal(EndpointEntryView* pEndpointView);
        void deleteSignal(EndpointEntryView* pEndpointView);

    private:
        static const char* SELECTED_CSS;
        static const char* UNSELECTED_CSS;

        QScopedPointer<Ui::EndpointEntryViewWidget> m_ui;
        Endpoint* m_pEndpoint = nullptr;

    private Q_SLOTS:
        void selectSlot();
        void deleteSlot();
    };
} // namespace News

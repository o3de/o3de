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

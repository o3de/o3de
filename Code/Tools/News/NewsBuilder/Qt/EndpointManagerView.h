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
#include <QDialog>
#endif

namespace Ui
{
    class EndpointManagerViewWidget;
}

namespace News
{
    class BuilderResourceManifest;
    class Endpoint;
    class EndpointEntryView;
    class EndpointManager;

    //! Qt dialog for managing endpoints
    class EndpointManagerView
        : public QDialog
    {
        Q_OBJECT
    public:
        EndpointManagerView(QWidget* parent,
            BuilderResourceManifest& manifest);
        ~EndpointManagerView();

    private:
        QScopedPointer<Ui::EndpointManagerViewWidget> m_ui;
        EndpointManager* m_pManager = nullptr;
        EndpointEntryView* m_pSelectedEndpoint = nullptr;
        QList<EndpointEntryView*> m_endpoints;
        BuilderResourceManifest& m_manifest;

        EndpointEntryView* AddEndpointEntry(Endpoint* pEndpoint);
        void Update() const;

    private Q_SLOTS:
        void accept() override;
        void addEndpointSlot();
        void selectEndpointSlot(EndpointEntryView* pEndpoint);
        void deleteEndpointSlot(EndpointEntryView* pEndpoint);
    };
} // namespace News

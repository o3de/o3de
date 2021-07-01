/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QLayout>

#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFrame.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFactory.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>
#include <AzToolsFramework/AssetBrowser/Previewer/EmptyPreviewer.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        PreviewerFrame::PreviewerFrame(QWidget* parent)
            : QFrame(parent)
        {
            setLayout(new QVBoxLayout);
            Clear();
        }

        void PreviewerFrame::Display(const AssetBrowserEntry* entry)
        {
            const auto factory = FindPreviewerFactory(entry);
            if (factory)
            {
                if (!m_previewer || m_previewer->GetName() != factory->GetName())
                {
                    InstallPreviewer(factory->CreatePreviewer(this));
                }
                m_previewer->Display(entry);
            }
            else
            {
                Clear();
            }
        }

        void PreviewerFrame::Clear()
        {
            if (m_previewer && m_previewer->GetName() == EmptyPreviewer::Name)
            {
                return;
            }
            InstallPreviewer(new EmptyPreviewer());
        }

        const PreviewerFactory* PreviewerFrame::FindPreviewerFactory(const AssetBrowserEntry* entry) const
        {
            AZ::EBusAggregateResults<const PreviewerFactory*> results;
            PreviewerRequestBus::BroadcastResult(results, &PreviewerRequests::GetPreviewerFactory, entry);
            for (const auto factory : results.values)
            {
                if (factory)
                {
                    return factory;
                }
            }
            return nullptr;
        }

        void PreviewerFrame::InstallPreviewer(Previewer* previewer)
        {
            if (m_previewer)
            {
                delete m_previewer;
                m_previewer = nullptr;
            }
            m_previewer = previewer;
            layout()->addWidget(previewer);
        }
    } // AssetBrowser
} // AzToolsFramework

#include <AssetBrowser/Previewer/moc_PreviewerFrame.cpp>

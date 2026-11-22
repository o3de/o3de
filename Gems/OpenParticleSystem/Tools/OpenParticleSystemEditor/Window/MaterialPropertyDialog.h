/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Serializer/ParticleSourceData.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QDialog>
AZ_POP_DISABLE_WARNING

class QLabel;

namespace OpenParticleSystemEditor
{
    class MaterialPropertyWidget;

    //! Floating window that hosts the per-emitter material property inspector.
    //!
    //! The emitter panel this is launched from is only ~249px wide and already lives inside a scroll area,
    //! which is far too cramped for a full material property layout. The mesh Material Component solves the
    //! same problem the same way, by opening a separate resizable inspector instead of inlining one.
    //!
    //! Modeless, so the viewport stays interactive while properties are tweaked. Parented to the widget that
    //! owns the emitter's DetailInfo so it is torn down automatically when that emitter goes away.
    class MaterialPropertyDialog : public QDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(MaterialPropertyDialog, AZ::SystemAllocator, 0);

        explicit MaterialPropertyDialog(QWidget* parent = nullptr);
        ~MaterialPropertyDialog() override = default;

        //! Points the dialog at an emitter and updates the title. Passing nullptr closes it, which is what
        //! happens when the emitter loses its material or switches to a renderer that has none.
        void SetDetail(OpenParticle::ParticleSourceData::DetailInfo* detail);

    Q_SIGNALS:
        //! Forwarded from the inspector. editingFinished is false for the intermediate values produced
        //! while a control is still being dragged.
        void OnMaterialPropertyChanged(bool editingFinished);

    private:
        void UpdateHeading(OpenParticle::ParticleSourceData::DetailInfo* detail);

        MaterialPropertyWidget* m_propertyWidget = nullptr;
        QLabel* m_heading = nullptr;
    };
} // namespace OpenParticleSystemEditor

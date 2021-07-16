/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_DRILLER_H
#define AZCORE_DRILLER_H

#include <AzCore/Driller/Stream.h>
namespace AZStd
{
    class mutex;
}
namespace AZ
{
    namespace Debug
    {
        class DrillerOutputStream;

        /**
         * Driller base class. Every driller should inherit from this class.
         * When a driller is need to start outputting data
         * the DrillerManager will call Driller::Start() so the driller
         * can output the initial state for all reported entities.
         * The same applies for the Stop.
         * Depending on the type of your driller you might choose to collect state
         * even before the driller has started. This of course should be a fast as
         * possible, as we don't want to burden engine systems and it's highly recommended
         * that you use configuration parameters to change that behavior as not all drillers
         * are used on a daily basis.
         * All drillers should use DebugAllocators (AZ_CLASS_ALLOCATOR(Driller,OSAllocator,0))
         * and they should use 'aznew' to create one, as by default if you don't unregister a
         * a driller, the manager will use "delete" to delete them.
         *
         * IMPORTANT: Driller systems works OUTSIDE engine systems, you should NOT use SystemAllocator or any other engine systems
         * as they might be drilled or not available at the moment.
         */
        class Driller
        {
            friend class DrillerManagerImpl;

        public:
            struct Param
            {
                enum Type
                {
                    PT_BOOL,
                    PT_INT,
                    PT_FLOAT
                };
                const char* desc;
                u32 name;
                int type;
                int value;
            };

            Driller()
                : m_output(NULL) {}
            virtual ~Driller() {}

            /// Returns the driller ID Crc32 of the name (Crc32(GetName())
            AZ::u32                 GetId() const;
            /// Driller group name, used only for organizational purpose
            virtual const char*     GroupName() const = 0;
            /// Unique name of the Driller, driller ID is the Crc of the name
            virtual const char*     GetName() const = 0;
            virtual const char*     GetDescription() const = 0;
            // @{ Managing the list of supported driller parameters.
            virtual int             GetNumParams() const        { return 0; }
            virtual const Param*    GetParam(int index) const   { (void)index; return NULL; }

        protected:
            Driller& operator=(const Driller&);

            /// Called by DrillerManager
            virtual void Start(const Param* params = NULL, int numParams = 0)   { (void)params; (void)numParams; }
            /// Called by DrillerManager
            virtual void Stop()     {}
            /// Called every frame by DrillerManger (while the driller is started)
            virtual void Update()   {}

            DrillerOutputStream*    m_output;       ///< Session output stream.
        };

        /**
         * Stores the information while an active
         * driller(s) session is running.
         */
        struct DrillerSession
        {
            int numFrames;
            int curFrame;
            typedef vector<Driller*>::type DrillerArrayType;
            DrillerArrayType         drillers;
            DrillerOutputStream*     output;
        };

        /**
         * Driller manager will manage all active driller sessions and driller factories. Generally you will never
         * need more than one driller manger.
         * IMPORTANT: Driller systems works OUTSIDE engine systems, you should NOT use SystemAllocator or any other engine systems
         * as they might be drilled or not available at the moment.
         */
        class DrillerManager
        {
            friend class DrillerRemoteServer;
        public:
            struct DrillerInfo
            {
                AZ::u32     id;
                vector<Driller::Param>::type params;
            };
            typedef forward_list<DrillerInfo>::type DrillerListType;

            virtual ~DrillerManager() {}

            static DrillerManager*  Create(/*const Descriptor& desc*/);
            static void             Destroy(DrillerManager* manager);

            virtual void            Register(Driller* driller) = 0;
            virtual void            Unregister(Driller* driller) = 0;

            virtual void            FrameUpdate() = 0;

            virtual DrillerSession* Start(DrillerOutputStream& output, const DrillerListType& drillerList, int numFrames = -1) = 0;
            virtual void            Stop(DrillerSession* session) = 0;

            virtual int             GetNumDrillers() const  = 0;
            virtual Driller*        GetDriller(int index) = 0;

        private:
            // If the manager created the allocator, it should destroy it when it gets destroyed
            bool m_ownsOSAllocator = false;
        };
    }
}

#endif // AZCORE_DRILLER_H
#pragma once

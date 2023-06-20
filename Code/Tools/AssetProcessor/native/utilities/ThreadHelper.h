/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef THREADHELPER_H
#define THREADHELPER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QMutex>
#include <QMetaObject>
#include <QWaitCondition>
#include <functional>
#include <QThread>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/functional.h>
#endif

// the Thread Helper exists to make it very easy to create a Qt object
// inside a thread, in such a way that the entire construction of the object
// occurs inside the thread.
// we do this by allowing you to specify a factory function that creates your object
// and we arrange to call it on the newly created thread, so that from the very moment
// your object exists, its already on its thread.  This is important because Qt objects
// set their thread ownership on create, and if your objects have sub-objects or child objects
// that are members, its important that they too are on the same thread.
// to use this system, use a thread Controller object and call initialize.
// initialize on the Thread Controller automatically blocks until your object is created on the
// target thread and returns your new object, allowing you to then connect signals and slots.
// to clean up, just call destroy().


namespace AssetProcessor
{
    AZ::s64 GetThreadLocalJobId();
    void SetThreadLocalJobId(AZ::s64 jobId);

    class ThreadWorker
        : public QObject
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ThreadWorker, AZ::SystemAllocator)
        explicit ThreadWorker(QObject* parent = 0)
            : QObject(parent)
        {
            m_runningThread = new QThread();
        }
        virtual ~ThreadWorker(){}
        //! Destroy function not only stops the running thread
        //! but also deletes the ThreadWorker object
        void Destroy()
        {
            // We are calling deletelater() before quiting the thread
            // because deletelater will only schedule the object for deletion
            // if the thread is running

            QThread* threadptr = m_runningThread;
            deleteLater();
            threadptr->quit();
            threadptr->wait();
            delete threadptr;
        }

Q_SIGNALS:

    public Q_SLOTS:
        void RunInThread()
        {
            create();
        }


    protected:
        virtual void create() = 0;

        QMutex m_waitConditionMutex;
        QWaitCondition m_waitCondition;
        QThread* m_runningThread;
    };

    //! This class helps in creating an instance of the templated
    //! QObject class in a new thread.
    //! Intialize is a blocking call and than will only return with a pointer to the
    //! new object only after the object is created in the new thread.
    //! Please note that each instance of this class has to be dynamically allocated.
    template<typename T>
    class ThreadController
        : public ThreadWorker
    {
    public:
        AZ_CLASS_ALLOCATOR(ThreadController<T>, AZ::SystemAllocator)
        typedef AZStd::function<T* ()> FactoryFunctionType;

        ThreadController()
            : ThreadWorker()
        {
            m_runningThread->setObjectName(T::staticMetaObject.className());
            m_runningThread->start();
            this->moveToThread(m_runningThread);
        }

        virtual ~ThreadController()
        {
        }

        T* initialize(FactoryFunctionType callback = nullptr)
        {
            m_function = callback;
            m_waitConditionMutex.lock();
            QMetaObject::invokeMethod(this, "RunInThread", Qt::QueuedConnection);

            m_waitCondition.wait(&m_waitConditionMutex);
            m_waitConditionMutex.unlock();
            return m_instance;
        }

        virtual void create() override
        {
            if (m_function)
            {
                m_instance = m_function();
            }

            m_waitCondition.wakeOne();
        }

    private:
        T* m_instance;
        FactoryFunctionType m_function;
    };
}

#endif // THREADHELPER_H


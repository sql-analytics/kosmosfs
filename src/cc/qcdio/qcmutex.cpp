//---------------------------------------------------------- -*- Mode: C++ -*-
// $Id: //depot/main/platform/kosmosfs/src/cc/qcdio/qcmutex.cpp#1 $
//
// Created 2008/10/30
// Author: Mike Ovsiannikov
//
// Copyright 2008,2009 Quantcast Corp.
//
// This file is part of Kosmos File System (KFS).
//
// Licensed under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
//
// 
//----------------------------------------------------------------------------

#include "qcmutex.h"
#include "qcutils.h"

    static int
GetAbsTimeout(
    QCMutex::Time    inTimeoutNanoSec,
    struct timespec& outAbsTimeout)
{
    const int theErr = clock_gettime(CLOCK_REALTIME, &outAbsTimeout);
    if (theErr != 0) {
        return theErr;
    }
    const QCMutex::Time k2NanoSec  = QCMutex::Time(1000) * 1000000;
    const QCMutex::Time theNanoSec = outAbsTimeout.tv_nsec + inTimeoutNanoSec;
    outAbsTimeout.tv_nsec = long(theNanoSec % k2NanoSec);
    outAbsTimeout.tv_sec += time_t(theNanoSec / k2NanoSec);
    return theErr;
}

QCMutex::QCMutex()
    : mLockCnt(0),
      mOwner(),
      mMutex()
{
    int theErr;
    pthread_mutexattr_t theAttr;
    if ((theErr = pthread_mutexattr_init(&theAttr)) != 0) {
        RaiseError("QCMutex: pthread_mutex_attr_init", theErr);
    }
    if ((theErr = pthread_mutexattr_settype(
            &theAttr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        RaiseError("QCMutex: pthread_mutexattr_settype", theErr);
    }
    if ((theErr = pthread_mutexattr_settype(
            &theAttr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        RaiseError("QCMutex: pthread_mutexattr_settype", theErr);
    }
    if ((theErr = pthread_mutex_init(&mMutex, &theAttr)) != 0) {
        RaiseError("QCMutex: pthread_mutex_init", theErr);
    }
    if ((theErr = pthread_mutexattr_destroy(&theAttr)) != 0) {
        RaiseError("QCMutex: pthread_mutexattr_destroy", theErr);
    }
}

QCMutex::~QCMutex()
{
    const int theErr = pthread_mutex_destroy(&mMutex);
    if (theErr != 0) {
        RaiseError("QCMutex::~QCMutex: pthread_mutex_destroy", theErr);
    }
}

    bool
QCMutex::Lock(
    QCMutex::Time inTimeoutNanoSec)
{
    struct timespec theAbsTimeout;
    int theErr = GetAbsTimeout(inTimeoutNanoSec, theAbsTimeout);
    if (theErr != 0) {
        RaiseError("QCMutex::Lock: clock_gettime", theErr);
    }
    theErr = pthread_mutex_timedlock(&mMutex, &theAbsTimeout);
    if (theErr == ETIMEDOUT) {
        return false;
    }
    if (theErr != 0) {
        RaiseError("QCMutex::Lock: pthread_mutex_timedlock", theErr);
    }
    return true;
}

    void
QCMutex::RaiseError(
    const char* inMsgPtr,
    int         inSysError)
{
    QCUtils::FatalError(inMsgPtr, inSysError);
}

QCCondVar::QCCondVar()
    : mCond()
{
    const int theErr = pthread_cond_init(&mCond, 0);
    if (theErr) {
        RaiseError("QCCondVar::QCCondVar: pthread_cond_init", theErr);
    }
}

QCCondVar::~QCCondVar()
{
    const int theErr = pthread_cond_destroy(&mCond);
    if (theErr) {
        RaiseError("QCCondVar::~QCCondVar: pthread_cond_destroy", theErr);
    }
}

    bool
QCCondVar::Wait(
    QCMutex&      inMutex,
    QCMutex::Time inTimeoutNanoSec)
{
    struct timespec theAbsTimeout;
    int theErr = GetAbsTimeout(inTimeoutNanoSec, theAbsTimeout);
    if (theErr != 0) {
        RaiseError("QCCondVar::Wait: clock_gettime", theErr);
    }
    inMutex.Unlocked();
    theErr = pthread_cond_timedwait(&mCond, &inMutex.mMutex, &theAbsTimeout);
    if (theErr == ETIMEDOUT) {
        return false;
    }
    if (theErr != 0) {
        RaiseError("QCCondVar::Wait: pthread_cond_timedwait", theErr);
    }
    return inMutex.Locked(theErr);
}

    void
QCCondVar::RaiseError(
    const char* inMsgPtr,
    int         inSysError)
{
    QCUtils::FatalError(inMsgPtr, inSysError);
}

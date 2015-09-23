/*
*                    Copyright (c), NXP Semiconductors
*
*                       (C)NXP Semiconductors B.V.2014
*         All rights are reserved. Reproduction in whole or in part is
*        prohibited without the written consent of the copyright owner.
*    NXP reserves the right to make changes without notice at any time.
*   NXP makes no warranty, expressed, implied or statutory, including but
*   not limited to any implied warranty of merchantability or fitness for any
*  particular purpose, or that the use will not infringe any third party patent,
*   copyright or trademark. NXP must not be liable for any loss or damage
*                            arising from its use.
*
*/
#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "phDTAOSAL.h"

#define LPVOID void*

/*
****************************** Macro Definitions ******************************
*/
/** \ingroup grp_osal_nfc
    Value indicates Failure to suspend/resume thread */
#define PH_OSALNFC_THREADPROC_FAILURE           (0xFFFFFFFF)

/* Thread procedure function which is invoked on thread creation */
/*===========================================================================*
* Static  functions:
*===========================================================================*/
/**
 * \ingroup grp_osal_nfc
 * Thread procedure function which is invoked on thread creation
 * This function shall invoke the user requested callback function which
 * was passed during creation of thread.
 *
 * \param[in] lpParameter   Pointer to the thread structure.
 *
 * \retval PH_OSALNFC_RESET_VALUE    Return from the Thread callback function.
 *
 */
static void * phOsalNfc_ThreadProcedure(void *lpParameter);

/*
*************************** Function Definitions ******************************
*/

OSALSTATUS phOsal_ThreadCreate   (void                         **hThread,
                                 pphOsal_ThreadFunction_t   pThreadFunction,
                                 void                          *pParam)
{
    uint32_t status = 0;
    ALOGD("Osal>%s:Enter",__FUNCTION__);
    if( (NULL == hThread) || (NULL == pThreadFunction) )
    {
        return OSALSTATUS_INVALID_PARAMS;
    }

    /* Check for successfull creation of thread */
    status = pthread_create((pthread_t *)hThread,
                            NULL,
                            pThreadFunction,
                            pParam);
    if(0 != status)
    {
     ALOGE("Osal>Unable to create Thread");
     return OSALSTATUS_FAILED;
    }
    ALOGD("Osal>%s:Exit:Id=0x%x",__FUNCTION__,*(uint32_t*)hThread);
    return OSALSTATUS_SUCCESS;
}

uint32_t phOsal_ThreadGetTaskId(void)
{
    uint32_t dwThreadId = 0;
    ALOGD("Osal>%s:Enter",__FUNCTION__);
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return dwThreadId;
}

OSALSTATUS phOsal_ThreadDelete(void *hThread)
{
    void *pRetVal;
    uint32_t status=0;
    ALOGD("Osal>%s:Enter",__FUNCTION__);
    if(NULL == hThread)
    {
        return OSALSTATUS_INVALID_PARAMS;
    }
    status = pthread_join((pthread_t)hThread,
                                    &pRetVal);
    if(0 != status)
    {
        ALOGE("Osal>Unable to delete Thread Id=0x%x",(size_t)hThread);
        return OSALSTATUS_FAILED;
    }
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_ThreadSuspend(void *hThread)
{
    OSALSTATUS wSuspendStatus = OSALSTATUS_SUCCESS;
    return wSuspendStatus;
}

OSALSTATUS phOsal_ThreadWakeUp(void *hThread)
{
    OSALSTATUS wResumeStatus = OSALSTATUS_SUCCESS;
    return wResumeStatus;
}

OSALSTATUS phOsal_ThreadSetPriority(void *hThread,int32_t sdwPriority)
{
    uint32_t dwStatus=0;
    struct sched_param param;
    int32_t policy;
    ALOGD("Osal>%s:Enter",__FUNCTION__);
    if(NULL == hThread)
    {
        return OSALSTATUS_INVALID_PARAMS;
    }
    dwStatus = pthread_getschedparam((pthread_t)hThread, &policy, &param);
   if(dwStatus != 0)
    {
        ALOGE("Osal>Unable to get thread params=0x%x: Error=0x%x",(size_t)hThread,dwStatus);
        return OSALSTATUS_FAILED;
    }
    param.sched_priority = sdwPriority;
    dwStatus = pthread_setschedparam((pthread_t)hThread, policy, &param);
    if(dwStatus != 0)
    {
        ALOGE("Osal>Unable to Set thread Priority=0x%x: Error=0x%x",(size_t)hThread,dwStatus);
        return OSALSTATUS_FAILED;
    }
    return OSALSTATUS_SUCCESS;
}

static void * phOsal_ThreadProcedure(void *lpParameter)
{
    return lpParameter;
}

OSALSTATUS phOsal_SemaphoreCreate(void        **hSemaphore,
                                 uint8_t     bInitialValue,
                                 uint8_t     bMaxValue)
{
    int32_t status=0;
    ALOGD("Osal>%s:Entry:0x%x",__FUNCTION__,(size_t)hSemaphore);

    if(hSemaphore == NULL)
    {
        ALOGE("Osal>Invalid Semaphore Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    *hSemaphore = malloc(sizeof(sem_t));
    if(*hSemaphore == NULL)
    {
        ALOGE("Osal>Unable to allocate memory for semaphore");
        return OSALSTATUS_FAILED;
    }

    status = sem_init(*hSemaphore, 0, bInitialValue);
    if(status == -1)
    {
        ALOGE ("Osal> Semaphore creation failed:status:%d\n",status);
        return OSALSTATUS_FAILED;
    }
    ALOGD("Osal> Semaphore Created:0x%x",(size_t)*hSemaphore);
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}


OSALSTATUS phOsal_SemaphorePost(void *hSemaphore)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry:0x%x",__FUNCTION__,(size_t)hSemaphore);
    if(hSemaphore == NULL)
    {
        ALOGE("Osal>Invalid Semaphore Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(sem_getvalue(hSemaphore,&checkval) == -1)
    {
        ALOGE ("Osal> Semaphore Not available\n");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(sem_post(hSemaphore) == -1)
   {
       ALOGE ("Osal> error in sem Post\n");
       return OSALSTATUS_INVALID_PARAMS;
   }

    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_SemaphoreWait(void *hSemaphore,
                                uint32_t timeout_ms)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry:0x%x",__FUNCTION__,(size_t)hSemaphore);
    if(hSemaphore == NULL)
    {
        ALOGE("Osal>Invalid Semaphore Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(sem_getvalue(hSemaphore,&checkval) == -1)
    {
        ALOGE ("Osal> Semaphore Not available\n");
        return OSALSTATUS_INVALID_PARAMS;
    }
    ALOGD("Osal>%s:semvalue=%d",__FUNCTION__,checkval);

     if(timeout_ms == 0)
     {
         if(sem_wait(hSemaphore) == -1)
         {
             ALOGE ("Osal> Error in Semaphore infinite wait !!\n");
             return OSALSTATUS_INVALID_PARAMS;
         }
     }
     else
     {
         struct timespec xtms;
         int32_t status=0;
         if(clock_gettime(CLOCK_REALTIME, &xtms) == -1)
         {
             ALOGE ("Osal> Error in Getting current CPU time !!\n");
             return OSALSTATUS_INVALID_PARAMS;
         }

         /*Extract seconds and nanoseconds information from time in milliseconds*/
         xtms.tv_sec  += (time_t)timeout_ms/1000;
         xtms.tv_nsec += ((long)(timeout_ms%1000))*(1000000);

         while ((status = sem_timedwait(hSemaphore, &xtms)) == -1 && errno == EINTR)
         {
             ALOGE("Osal>Error in sem_timedwait restart it\n");
             continue;       /* Restart if interrupted by handler */
         }
         /* Check what happened */
         if (status == -1)
         {
             if (errno == ETIMEDOUT)
             {
                 ALOGE("Osal>sem_timedwait() timed out\n");
                 return OSALSTATUS_SEM_TIMEOUT;
             }
             else
             {
                 ALOGE("Osal>sem_timedwait");
                 return OSALSTATUS_FAILED;
             }
         }
         else
         {
             ALOGD("Osal>sem_timedwait() succeeded\n");
         }
     }
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_SemaphoreDelete(void *hSemaphore)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry:0x%x",__FUNCTION__,(size_t)hSemaphore);
    if(hSemaphore == NULL)
    {
        ALOGE("Osal>Invalid Semaphore Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(sem_getvalue(hSemaphore,&checkval) == -1)
    {
        ALOGE ("Osal> Semaphore Not available\n");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(sem_destroy(hSemaphore) == -1)
    {
       ALOGE ("Osal> Semaphore Destroy Failed\n");
       return OSALSTATUS_FAILED;
    }

    free(hSemaphore);
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexCreate(void        **hMutex)
{
    int32_t status=0;
    ALOGD("Osal>%s:Entry",__FUNCTION__);

    if(hMutex == NULL)
    {
        ALOGE("Osal>Invalid Mutex Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    *hMutex = malloc(sizeof(pthread_mutex_t));
    if(*hMutex == NULL)
    {
        ALOGE("Osal>Unable to allocate memory for mutex");
        return OSALSTATUS_FAILED;
    }

    status = pthread_mutex_init(*hMutex, 0);
    if(status == -1)
    {
        ALOGE ("Osal> Mutex creation failed:status:%d\n",status);
        return OSALSTATUS_FAILED;
    }
    ALOGD("Osal> Mutex Created\n");
    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexLock(void *hMutex)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry",__FUNCTION__);
    if(hMutex == NULL)
    {
        ALOGE("Osal>Invalid Mutex Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(pthread_mutex_lock(hMutex) == -1)
    {
        ALOGE ("Osal> Error in Mutex Lock\n");
        return OSALSTATUS_INVALID_PARAMS;
    }

    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexUnlock(void *hMutex)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry",__FUNCTION__);
    if(hMutex == NULL)
    {
        ALOGE("Osal>Invalid Mutex Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(pthread_mutex_unlock(hMutex) == -1)
    {
        ALOGE ("Osal> Error in Mutex UnLock\n");
        return OSALSTATUS_INVALID_PARAMS;
    }

    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_MutexDelete(void *hMutex)
{
    uint32_t checkval;
    ALOGD("Osal>%s:Entry",__FUNCTION__);
    if(hMutex == NULL)
    {
        ALOGE("Osal>Invalid Mutex Handle");
        return OSALSTATUS_INVALID_PARAMS;
    }

    if(pthread_mutex_destroy(hMutex) == -1)
    {
        ALOGE ("Osal> Error in Mutex Destroy\n");
        return OSALSTATUS_INVALID_PARAMS;
    }

    free(hMutex);

    ALOGD("Osal>%s:Exit",__FUNCTION__);
    return OSALSTATUS_SUCCESS;
}

OSALSTATUS phOsal_Init(pphOsal_Config_t pOsalConfig)
{
    return OSALSTATUS_SUCCESS;
}

void phOsal_Delay(uint32_t dwDelayInMs)
{
    usleep(dwDelayInMs*1000); /**< Converting milliseconds to Microseconds */
}

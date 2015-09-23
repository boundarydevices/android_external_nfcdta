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

/*!
 * \file phDTA_T2TTest.c
 *
 * Project: NFC DTA
 *
 */
/* Preprocessor includes for different platform */
#ifdef WIN32
#include <windows.h>
#include "phNfcTypes.h"
#else
#include <utils/Log.h>
#include "data_types.h"
#endif
#include "phMwIf.h"
#include "phDTALib.h"
#include "phDTAOSAL.h"
#include "phOsal_LinkList.h"
#include "phOsal_Queue.h"
#include "phDTATst.h"

/*Constant T2T Data for predefined memory layouts*/
static const uint8_t gs_T2TPattern1[] = {0x03, 0x00, 0xCC, 0x0A};
static const uint8_t gs_T2TPattern2[] = {0x00, 0x00, 0x00, 0x01};
static const uint8_t gs_T2TPattern3[] = {0x01, 0x74, 0x65, 0x78};
static const uint8_t gs_T2TPattern4[] = {0x74, 0x2F, 0x70, 0x6C};
static const uint8_t gs_T2TPattern5[] = {0x61, 0x69, 0x6E, 0x31};
static const uint8_t gs_T2TPattern6[] = {0x30, 0xFE, 0x00, 0x00};
static const uint8_t gs_T2TPattern7[] = {0x03, 0x13, 0xCC, 0x0A};
static uint8_t gs_T2TBuff[400]  = {0};

static const uint8_t gs_T2TStaticLayout1[] = {0x03,0x00,0xD1,0x01};
static const uint8_t gs_T2TStaticLayout2[] = {0x06,0x55,0x01,0x6E};
static const uint8_t gs_T2TStaticLayout3[] = {0x2E,0x63,0x6F,0x6D};
static const uint8_t gs_T2TStaticLayout4[] = {0xFE,0x00,0x00,0x00};
static const uint8_t gs_T2TStaticLayout5[] = {0x03,0x0A,0xD1,0x01};

static const uint8_t gs_T2TDynamicLayout1[] = {0x03,0x00,0x07,0xEC};
static const uint8_t gs_T2TDynamicLayout2[] = {0xC5,0x01,0x00,0x00};
static const uint8_t gs_T2TDynamicLayout3[] = {0x07,0xE5,0x55,0x01};
static const uint8_t gs_T2TDynamicLayout4[] = {0xFE,0x00,0x00,0x00};
static const uint8_t gs_T2TDynamicLayout5[] = {0x03,0x0A,0x07,0xEC};

static const uint8_t gs_T2TDynamicLayoutLockCtrl1[] = {0x44,0x03,0x00,0xD1};
static const uint8_t gs_T2TDynamicLayoutLockCtrl2[] = {0x01,0x20,0x54,0x02};
static const uint8_t gs_T2TDynamicLayoutLockCtrl3[] = {0x65,0x6E,0x54,0x68};
static const uint8_t gs_T2TDynamicLayoutLockCtrl4[] = {0x69,0x73,0x20,0x69};
static const uint8_t gs_T2TDynamicLayoutLockCtrl5[] = {0x73,0x20,0x61,0x6E};
static const uint8_t gs_T2TDynamicLayoutLockCtrl6[] = {0x20,0x4E,0x44,0x45};
static const uint8_t gs_T2TDynamicLayoutLockCtrl7[] = {0x46,0x20,0x74,0x65};
static const uint8_t gs_T2TDynamicLayoutLockCtrl8[] = {0x78,0x74,0x20,0x6D};
static const uint8_t gs_T2TDynamicLayoutLockCtrl9[] = {0x65,0x73,0x73,0x61};
static const uint8_t gs_T2TDynamicLayoutLockCtrl10[] = {0x67,0x65,0x00,0xFE};
static const uint8_t gs_T2TDynamicLayoutLockCtrl11[] = {0x44,0x03,0x24,0xD1};

static const uint8_t gs_T2TStaticIntlzMemoryLayout1[] = {0xE1,0x10,0x06,0x0F}; /*updating CC to make Read-Only*/
static const uint8_t gs_T2TStaticIntlzMemoryLayout2[] = {0x79,0xC8,0xFF,0xFF}; /* lock bytes*/

static const uint8_t gs_T2TStaticInlzMemCapabilityContainer[]= {0xE1,0x10,0xFE,0x0F};
static const uint8_t gs_T2TStaticInlzMemLockBytes[]          = {0x79,0xC8,0xFF,0xFF};
static uint8_t gs_T2TStaticInlzMemWrite[]                    = {0xA2,0x00,0xFF,0xFF,0xFF,0xFF}; /*lock bytes set to one*/

static const uint8_t gs_T2TDynamicIntlzMemoryLayoutwithTLV1[] = {0xE1,0x10,0x12,0x0F};
static const uint8_t gs_T2TDynamicIntlzMemoryLayoutwithTLV2[] = {0xA9,0x48,0xFF,0xFF};
static const uint8_t gs_T2TDynamicIntlzMemoryLayoutwithTLV3[] = {0xFF,0xFF,0xFF,0xFF};

extern phDtaLib_sHandle_t g_DtaLibHdl;
static uint8_t gs_miscBuffer[400];             /**< Misc buffer used for compare */
static uint16_t gui_miscBufferLength;          /**< Length for Misc array  */
extern BOOLEAN gui_flag_t2t_sleep;

#define T2T_READ_BLOCK(dwMwIfStatus,psTagParams,mwIfHdl,blkNum)  {\
        psTagParams->eT2TCmd = PHMWIF_T2T_READ_CMD;\
        psTagParams->sBuffParams.pbBuff = gs_T2TBuff;\
        psTagParams->dwBlockNum = blkNum;\
        dwMwIfStatus = phMwIf_TagCmd(mwIfHdl, PHMWIF_PROTOCOL_T2T,psTagParams);\
        if(dwMwIfStatus != MWIFSTATUS_SUCCESS)\
            break;}

#define T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,mwIfHdl,blkNum,tagData)  {\
        psTagParams->eT2TCmd = PHMWIF_T2T_WRITE_CMD;\
        psTagParams->dwBlockNum = blkNum;\
        psTagParams->sBuffParams.pbBuff = tagData;\
        psTagParams->sBuffParams.dwBuffLength = sizeof(tagData);\
        dwMwIfStatus = phMwIf_TagCmd(mwIfHdl, PHMWIF_PROTOCOL_T2T,psTagParams);\
        if(dwMwIfStatus != MWIFSTATUS_SUCCESS)\
            break;}

#define T2T_SECTOR_SELECT(dwMwIfStatus,psTagParams,mwIfHdl,SectorNum)  {\
        psTagParams->eT2TCmd = PHMWIF_T2T_SECTOR_SELECT_CMD;\
        psTagParams->dwSectorNum = SectorNum;\
        psTagParams->sBuffParams.pbBuff = NULL;\
        psTagParams->sBuffParams.dwBuffLength = 0;\
        dwMwIfStatus = phMwIf_TagCmd(mwIfHdl, PHMWIF_PROTOCOL_T2T,psTagParams);\
        if(dwMwIfStatus != MWIFSTATUS_SUCCESS)\
            break;}

#define NUM_T2T_SECTOR2_DLB 0x08

/**
*T2T Tag read/write operations based on pattern number
*/
DTASTATUS phDtaLibi_T2TOperations(phDtaLib_sTestProfile_t TestProfile)
{
    MWIFSTATUS dwMwIfStatus = 0;
    uint32_t   dwBlkNum     = 0;
    phDtaLib_sHandle_t *dtaLibHdl = &g_DtaLibHdl;
    phMwIf_uTagParams_t   sTagParams;
    phMwIf_sT2TParams_t*   psTagParams = (phMwIf_sT2TParams_t*)&sTagParams;

    LOG_FUNCTION_ENTRY;
    memset(psTagParams, 0, sizeof(phMwIf_uTagParams_t));

    phOsal_LogDebugString((const uint8_t*)"DTALib> :", (const uint8_t*)__FUNCTION__);
    phOsal_LogDebugU32h((const uint8_t*)"PatternNum = ", TestProfile.Pattern_Number);
    switch(TestProfile.Pattern_Number)
    {
        case 0x0000:
        {

          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x00)
          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)

          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TPattern1)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05,gs_T2TPattern2)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x06,gs_T2TPattern3)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x07,gs_T2TPattern4)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x08,gs_T2TPattern5)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x09,gs_T2TPattern6)
          T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TPattern7)

         }
        break;
        case 0x0001:
        {
          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x00)
          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x03)

          memcpy(gs_miscBuffer,psTagParams->sBuffParams.pbBuff, psTagParams->sBuffParams.dwBuffLength);
          gui_miscBufferLength = psTagParams->sBuffParams.dwBuffLength;
          if ( (gs_miscBuffer[2] == 0x06) || (gs_miscBuffer[2] == 0xFE) )
          {
               phOsal_LogDebug("(gs_miscBuffer[2] == 0x06) || (gs_miscBuffer[2] == 0xFE)");
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)


          }
          else if(gs_miscBuffer[2] == 0x12)
          {
                phOsal_LogDebug("gs_miscBuffer[2] == 0x12");

                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x09)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0D)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x11)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x15)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x19)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x1D)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x21)
                T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x24)
          }
        }
        break;
        case 0x0002:
        {
          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x00)

          memcpy(gs_miscBuffer,psTagParams->sBuffParams.pbBuff, psTagParams->sBuffParams.dwBuffLength);
          gui_miscBufferLength = psTagParams->sBuffParams.dwBuffLength;

          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)

          if( gs_miscBuffer[4*3+2] == 0x06 )
          {
            if(gs_miscBuffer[4*3+3] == 0x0F )
            {
              phOsal_LogError((const uint8_t*)"DTALib>T2T:Read Only Static Memory Layout \n");
              break;
            }
            phOsal_LogDebug((const uint8_t*)"DTALib>T2T:Initialized Static Memory Layout \n");

            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TStaticLayout1)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05,gs_T2TStaticLayout2)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x06,gs_T2TStaticLayout3)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x07,gs_T2TStaticLayout4)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TStaticLayout5)
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
          else if( gs_miscBuffer[4*3+2] == 0xFE )
          {
            phOsal_LogDebug((const uint8_t*)"DTALib>T2T Initialized Dynamic Memory Layout \n");
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TDynamicLayout1)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05,gs_T2TDynamicLayout2)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x06,gs_T2TDynamicLayout3)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x07,gs_T2TDynamicLayout4)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04,gs_T2TDynamicLayout5)
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
          else if( gs_miscBuffer[4*3+2] == 0x12 )
          {

            phOsal_LogDebug((const uint8_t*)"DTALib>T2T: Initialized Dynamic Memory Layout with Lock control TLV\n");
            T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x28)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05,gs_T2TDynamicLayoutLockCtrl1)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x06,gs_T2TDynamicLayoutLockCtrl2)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x07,gs_T2TDynamicLayoutLockCtrl3)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x08,gs_T2TDynamicLayoutLockCtrl4)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x09,gs_T2TDynamicLayoutLockCtrl5)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0A,gs_T2TDynamicLayoutLockCtrl6)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0B,gs_T2TDynamicLayoutLockCtrl7)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0C,gs_T2TDynamicLayoutLockCtrl8)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0D,gs_T2TDynamicLayoutLockCtrl9)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x0E,gs_T2TDynamicLayoutLockCtrl10)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x05,gs_T2TDynamicLayoutLockCtrl11)
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
        }
        break;
        case 0x0003:
        {
          T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x00)

          memcpy(gs_miscBuffer,psTagParams->sBuffParams.pbBuff, psTagParams->sBuffParams.dwBuffLength);
          gui_miscBufferLength = psTagParams->sBuffParams.dwBuffLength;

          if( gs_miscBuffer[4*3+2] == 0x06 ) /* 48 bytes*/
          {

            phOsal_LogDebug((const uint8_t*)"DTALib>T2T:Initialized Static Memory Layout \n");
            T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x03,gs_T2TStaticIntlzMemoryLayout1)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x02,gs_T2TStaticIntlzMemoryLayout2)
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
          else if( gs_miscBuffer[4*3+2] == 0xFE )
          {
            phOsal_LogDebug((const uint8_t*)"DTALib>T2T:Dynamic Memory Layout without lock control TLV \n");
            T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x03,gs_T2TStaticInlzMemCapabilityContainer)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x02,gs_T2TStaticInlzMemLockBytes)
            T2T_SECTOR_SELECT(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,2)

            for(dwBlkNum = 0;dwBlkNum < NUM_T2T_SECTOR2_DLB; dwBlkNum++)
            {
                phOsal_LogDebugU32h((const uint8_t*)"DTA_T2T> block number =  ",dwBlkNum);
                if(dwBlkNum == 7)
                {
                    gs_T2TStaticInlzMemWrite[1]  = dwBlkNum;
                    gs_T2TStaticInlzMemWrite[5]  = 0x00; /* Internal bytes */
                }
                else
                {
                    gs_T2TStaticInlzMemWrite[1] = dwBlkNum;
                }

                /*Transceive is causing timeouts, Need to debug. As of now using raw frame*/
                dwMwIfStatus = phMwIf_SendRawFrame(dtaLibHdl->mwIfHdl,
                                                 (uint8_t *) gs_T2TStaticInlzMemWrite,
                                                 (uint16_t)sizeof(gs_T2TStaticInlzMemWrite));
                if(dwMwIfStatus != DTASTATUS_SUCCESS)
                {
                    phOsal_LogError((const uint8_t*)"DTALib>T2T: Error Failed to Transceive data !! \n");
                    break;
                }
            }
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
          else if( gs_miscBuffer[4*3+2] == 0x12 ) /* for LT */
          {
            phOsal_LogDebug((const uint8_t*)"DTALib>T2T:Initialized Dynamic Memory Layout with Lock control TLV\n");
            T2T_READ_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x04)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x03,gs_T2TDynamicIntlzMemoryLayoutwithTLV1)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x02,gs_T2TDynamicIntlzMemoryLayoutwithTLV2)
            T2T_WRITE_BLOCK(dwMwIfStatus,psTagParams,dtaLibHdl->mwIfHdl,0x28,gs_T2TDynamicIntlzMemoryLayoutwithTLV3)
            phOsal_Delay(200); /*for last sending packet!!! we have to wait until UT3 recognized the last packet of test!!*/
          }
        }
        break;
        default:
          phOsal_LogError((const uint8_t*)"DTALib>T2T:Error Pattern Number not valid for T2T !! \n");
          break;
      }
    phMwIf_NfcDeactivate(dtaLibHdl->mwIfHdl,PHMWIF_DEACTIVATE_TYPE_SLEEP);
    phMwIf_NfcDeactivate(dtaLibHdl->mwIfHdl,PHMWIF_DEACTIVATE_TYPE_IDLE);
    LOG_FUNCTION_EXIT;
    return dwMwIfStatus;
}

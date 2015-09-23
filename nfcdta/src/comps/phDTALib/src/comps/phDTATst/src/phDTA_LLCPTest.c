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

/*
 * phDTA_LLCPTest.c
 *
 *  Created on: Jul 7, 2014
 *      Author: nxp
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

#define PHDTALIB_SERVER_MIU ((uint16_t)128)
#define PHDTALIB_SERVER_RW  ((uint8_t)16)
#define PHDTALIB_CLIENT_MIU ((uint16_t)128)
#define PHDTALIB_CLIENT_RW  ((uint8_t)16)
#define PHDTALIB_MAX_LLCP_DATA 512
static  uint8_t gs_LlcpConnLessData[512]={0};
extern  phDtaLib_sHandle_t g_DtaLibHdl;
/**
 * Operations related to LLCP starts below
 */
DTASTATUS phDtaLibi_LlcpOperations(phDtaLib_sTestProfile_t* psTestProfile,
                                   phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                   phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo)
{
    MWIFSTATUS dwMwIfStatus;
    phDtaLib_sHandle_t *psDtaLibHdl = &g_DtaLibHdl;
    phMwIf_sLlcpClientConnectParams_t  sConnectPrms;
    uint8_t                  bRemoteSapCoEchoOut;
    LOG_FUNCTION_ENTRY;

    if(eLlcpEvtType == PHMWIF_LLCP_SERVER_CONN_REQ_EVT)
    {
        /*Accept the connection request*/
        phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:Accepting Client Connection with Following Params");
        phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Client SAP=0x",puLlcpEvtInfo->sConnReq.bRemoteSap);
        phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Client RW=0x",puLlcpEvtInfo->sConnReq.bRemoteRw);
        phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Client MIU=0x",puLlcpEvtInfo->sConnReq.wRemoteMiu);
        psDtaLibHdl->pvCORemoteClientConnHandle = puLlcpEvtInfo->sConnReq.wConnHandle;
        dwMwIfStatus = phMwIf_LlcpServerAcceptClientConn(psDtaLibHdl->mwIfHdl,
                                                         (void *)(((char*)0)+(puLlcpEvtInfo->sConnReq.wServerHandle)),
                                                         PHDTALIB_SERVER_MIU,
                                                         PHDTALIB_SERVER_RW,
                                                         (void *)(((char*)0)+(puLlcpEvtInfo->sConnReq.wConnHandle)));
        if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
        {
            phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not Accept Connection", (const uint8_t*)__FUNCTION__);
            return dwMwIfStatus;
        }
        if(psTestProfile->Pattern_Number == PHDTALIB_LLCP_CO_SET_NAME_OR_CL)
        {/*Connect to server by Service name*/
            phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:CO:Connect by name ");
            /*Connect client to Remote Server*/
            phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Connecting Client to LLCP Remote Server SAP=0x",PHDTALIB_SAP_LT_CO_OUT_DEST);
            sConnectPrms.eConnectType         = PHMWIF_SERVER_CONNECT_BY_NAME;
            sConnectPrms.bSap                 = 0;
            sConnectPrms.pbServerServiceName  = gs_abOutboundCoService;
            sConnectPrms.pfMwIfLlcpClientCb   = phDtaLibi_EvtCb;
            sConnectPrms.pvApplHdl            = (void*)psDtaLibHdl;
            sConnectPrms.wClientMiu           = PHDTALIB_CLIENT_MIU;
            sConnectPrms.bClientRw            = PHDTALIB_CLIENT_RW;
            dwMwIfStatus = phMwIf_LlcpClientConnect(psDtaLibHdl->mwIfHdl,
                                                    &sConnectPrms,
                                                    &psDtaLibHdl->pvCORemoteServerConnHandle);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
        }
        else if(psTestProfile->Pattern_Number == PHDTALIB_LLCP_CO_SET_SNL_OR_CL)
        {/*If pattern is to get the SAP using SNL and then connect, Do SDP and get the SAP*/
            phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:CO:Getting SAP using SDP");
            dwMwIfStatus = phMwIf_LlcpServiceDiscovery( psDtaLibHdl->mwIfHdl,
                                                        "urn:nfc:sn:dta-co-echo-out",
                                                        &bRemoteSapCoEchoOut);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
            /*Connect client to Remote Server*/
            phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Connecting Client to LLCP Remote Server SAP=0x",PHDTALIB_SAP_LT_CO_OUT_DEST);
            sConnectPrms.eConnectType         = PHMWIF_SERVER_CONNECT_BY_SAP;
            sConnectPrms.bSap                 = bRemoteSapCoEchoOut;
            sConnectPrms.pbServerServiceName  = NULL;
            sConnectPrms.pfMwIfLlcpClientCb   = phDtaLibi_EvtCb;
            sConnectPrms.pvApplHdl            = (void*)psDtaLibHdl;
            sConnectPrms.wClientMiu           = PHDTALIB_CLIENT_MIU;
            sConnectPrms.bClientRw            = PHDTALIB_CLIENT_RW;
            dwMwIfStatus = phMwIf_LlcpClientConnect(psDtaLibHdl->mwIfHdl,
                                                    &sConnectPrms,
                                                    &psDtaLibHdl->pvCORemoteServerConnHandle);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
        }
        else
        {
            phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:CO:SAP value set to LT_CO_OUT_DEST");
            bRemoteSapCoEchoOut = PHDTALIB_SAP_LT_CO_OUT_DEST;
            /*Connect client to Remote Server*/
            phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Connecting Client to LLCP Remote Server SAP=0x",PHDTALIB_SAP_LT_CO_OUT_DEST);
            sConnectPrms.eConnectType         = PHMWIF_SERVER_CONNECT_BY_SAP;
            sConnectPrms.bSap                 = bRemoteSapCoEchoOut;
            sConnectPrms.pbServerServiceName  = NULL;
            sConnectPrms.pfMwIfLlcpClientCb   = phDtaLibi_EvtCb;
            sConnectPrms.pvApplHdl            = (void*)psDtaLibHdl;
            sConnectPrms.wClientMiu           = PHDTALIB_CLIENT_MIU;
            sConnectPrms.bClientRw            = PHDTALIB_CLIENT_RW;
            dwMwIfStatus = phMwIf_LlcpClientConnect(psDtaLibHdl->mwIfHdl,
                                                    &sConnectPrms,
                                                    &psDtaLibHdl->pvCORemoteServerConnHandle);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
        }

        /*Wait for Data to start LLCP Transaction*/
/*
        phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:Waiting for Data from Remote Client");
        sLlcpData.pbBuff       = gs_LlcpData;
        sLlcpData.dwBuffLength = PHDTALIB_MAX_LLCP_DATA;
        dwMwIfStatus = phMwIf_LlcpRecvData(psDtaLibHdl->mwIfHdl,
                                           psDtaLibHdl->pvCORemoteClientConnHandle,
                                           &sLlcpData);
        if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
        {
            phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
            return dwMwIfStatus;
        }
        phDtaLibi_PrintBuffer(sLlcpData.pbBuff,sLlcpData.dwBuffLength,(const uint8_t*)"DTALib>LLCP:Received Data");
*/
    }

    LOG_FUNCTION_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * Operations to be done after remote client disconnects from local connection oriented server
 */
DTASTATUS phDtaLibi_LlcpHandleCoClientDisconnect(phDtaLib_sTestProfile_t* psTestProfile,
                                                 phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                                 phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo)
{
    MWIFSTATUS dwMwIfStatus;
    phDtaLib_sHandle_t *psDtaLibHdl = &g_DtaLibHdl;
    LOG_FUNCTION_ENTRY;

    /*Disconnect Client from Remote Server*/
    dwMwIfStatus = phMwIf_LlcpClientDisconnect(psDtaLibHdl->mwIfHdl,
                                               psDtaLibHdl->pvCORemoteServerConnHandle);
    if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
    {
        phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not Disconnect from Remote server", (const uint8_t*)__FUNCTION__);
        return dwMwIfStatus;
    }

    LOG_FUNCTION_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * Operations related to LLCP starts below
 */
DTASTATUS phDtaLibi_LlcpConnLessOperations(phDtaLib_sTestProfile_t* psTestProfile,
                                           phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                           phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo)
{
    MWIFSTATUS dwMwIfStatus;
    phDtaLib_sHandle_t*  psDtaLibHdl = &g_DtaLibHdl;
    phMwIf_sBuffParams_t sLlcpConnLessData;
    uint8_t              bRemoteSapClEchoOut;
    uint8_t              bRemoteClientSap;
    uint8_t const abStartOfLlcpTestCmd[] = { 0x53, 0x4F, 0x54 };
    LOG_FUNCTION_ENTRY;

    if(eLlcpEvtType == PHMWIF_LLCP_SERVER_CONN_LESS_DATA_EVT)
    {
        /*Accept the connection request*/
        phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:ConnectionLessServerParams:");
        phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Client SAP=",puLlcpEvtInfo->sLlcpConnLessData.bRemoteSap);
        phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:ConnLessRemoteHandle=",puLlcpEvtInfo->sLlcpConnLessData.wConnLessHandle);
        psDtaLibHdl->pvCLRemoteClientConnLessHandle = puLlcpEvtInfo->sLlcpConnLessData.wConnLessHandle;
        bRemoteClientSap = puLlcpEvtInfo->sLlcpConnLessData.bRemoteSap;

        /*Wait for Data to start LLCP Transaction*/
        phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:Waiting for Data from Remote Client");
        sLlcpConnLessData.pbBuff       = gs_LlcpConnLessData;
        sLlcpConnLessData.dwBuffLength = PHDTALIB_MAX_LLCP_DATA;
        dwMwIfStatus = phMwIf_LlcpConnLessRecvData(psDtaLibHdl->mwIfHdl,
                                           psDtaLibHdl->pvCLRemoteClientConnLessHandle,
                                           &bRemoteClientSap,
                                           &sLlcpConnLessData);
        if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
        {
            phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
            return dwMwIfStatus;
        }

        /*Check if the data received is to indicate start of LLCP test
         * If its the start, Send SNL*/
        phDtaLibi_PrintBuffer(sLlcpConnLessData.pbBuff,sLlcpConnLessData.dwBuffLength,(const uint8_t*)"DTALib>LLCP:Received Data");
        if ((memcmp((const void *) sLlcpConnLessData.pbBuff,
                    (const void *) abStartOfLlcpTestCmd,
                           sizeof(abStartOfLlcpTestCmd)) == 0))
        {
            phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:Received Start Of Test Cmd ");
            dwMwIfStatus = phMwIf_LlcpServiceDiscovery( psDtaLibHdl->mwIfHdl,
                                                        "urn:nfc:sn:dta-cl-echo-out",
                                                        &bRemoteSapClEchoOut);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not connect to Remote client", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
            phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:RemoteClEchoOut Sap value=",bRemoteSapClEchoOut);
            psDtaLibHdl->bRemoteSapClEchoOut = bRemoteSapClEchoOut;
        }
        else
        {/*Its a loopback data.Send it back*/
            phOsal_LogDebug((const uint8_t*)"DTALib>LLCP:Waiting for TDELAY_CL");
            phOsal_Delay(TDELAY_CL_IN_MS);
            phOsal_LogDebugU32h((const uint8_t*)"DTALib>LLCP:Send Loopback Data back to SAP:",bRemoteSapClEchoOut);
            dwMwIfStatus = phMwIf_LlcpConnLessSendData(psDtaLibHdl->mwIfHdl,
                                                       psDtaLibHdl->bRemoteSapClEchoOut,
                                                       &sLlcpConnLessData);
            if (dwMwIfStatus != MWIFSTATUS_SUCCESS)
            {
                phOsal_LogErrorString((const uint8_t*)"DTALib>LLCP::Could not Send Connless Data", (const uint8_t*)__FUNCTION__);
                return dwMwIfStatus;
            }
        }
    }

    LOG_FUNCTION_EXIT;
    return DTASTATUS_SUCCESS;
}

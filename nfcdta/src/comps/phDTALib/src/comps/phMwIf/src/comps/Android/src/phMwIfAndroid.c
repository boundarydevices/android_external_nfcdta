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
************************* Header Files ****************************************
*/

/*< Standard Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <semaphore.h>
/* #include <sys/time.h> */
#include <sys/resource.h>

#include <utils/Log.h>
#include "data_types.h"
#include "OverrideLog.h"

#include "nci_defs.h"
#include <nfa_api.h>
#include <nfa_rw_api.h>
#include <nfa_snep_api.h>
#include <nfa_ce_api.h>
#include <nfa_ee_api.h>
#include <nfa_hci_api.h>
#include <nfa_p2p_api.h>
#include <ndef_utils.h>
#include "dlfcn.h"

#include "phDTAOSAL.h"
#include "phOsal_LinkList.h"
#include "phOsal_Queue.h"
#include "phMwIf.h"
#include "phMwIfAndroid.h"
#include "phMwIfLib_NfcAdaptWrap.h"

typedef void (tNFA_NXP_NCI_RSP_CBACK) (UINT8 event, UINT16 param_len, UINT8 *p_param);

uint32_t        g_dwEvtType;
static char    APP_NAME[]                 = "Nxp_Uicc_Dta";
static UINT8   bProtocol;

/*NFA EE Events,NFA P2P events and NFA Conn callback event values are sent to same queue
and some of these events have same value.To avoid it, offset is added to
EE events*/
#define  NFA_EE_EVT_OFFSET  100
#define  NFA_P2P_EVT_OFFSET 200
#define  NFA_NXPCFG_EVT_OFFSET 220
#define  NFA_DM_EVT_OFFSET 150
#define  MAX_CONFIG_CMD_LEN 100
#define  NCI_RSP_EVT 0x42

uint8_t gs_paramBuffer[400];/**< Buffer for passing Data during operations */
uint32_t gs_sizeParamBuffer;
volatile tNFA_STATUS gx_status = NFA_STATUS_FAILED;

/**< Secure Element testing variables */
tNFA_HANDLE NfaUiccHandle = 0x402;
tNFA_HANDLE NfaEseHandle  = 0x4C0;
tNFA_HANDLE NfaAidHandle;
static uint8_t Pgu_event; /**< To indicate the Last event that occurred from any Call Back */

static uint8_t Pgu_disoveredDeviceCount = 0;/**< To Store the number of devices detected in discovery */
static uint8_t Pgs_cbTempBuffer[400];/**< Buffer for Data copy in Call back */
static uint16_t Pgui_cbTempBufferLength;/**< Length of CB buffer */
BOOLEAN gb_device_connected = FALSE; /**< Indicates if the device is connected or not */
BOOLEAN gb_device_ndefcompliant = FALSE; /**< Indicates the is NDEF compliant or not */
BOOLEAN gb_discovery_notify = FALSE; /**< To be used when multiple devices are detected */
tNFA_ACTIVATED      gx_device; /**<  Tag/Device Data Detected on Connect */
tNFA_CE_ACTIVATED   gx_se_device;
tNFA_NDEF_DETECT    gx_ndef_detected;
tNFA_NDEF_EVT_DATA  gx_ndef_data;/**< Variable containing NDEF data when Ndef RD/WR is performed */
tNFA_HANDLE         gx_ndef_type_handle;
tNFA_DISC_RESULT    gx_discovery_result;
uint8_t gs_Uicc_Aid[] = {0xA0,0x00,0x00,0x00,0x18,0x30,0x80,0x05,0x00,0x65,0x63,0x68,0x6F,0x00};
uint8_t gs_Uicc_Aid_len = 0x0E;
tNFA_HANDLE NfaHostHandle           = 0x400;
tNFA_HANDLE NfaHciHandle;

/*LLCP related constants*/
#define LLCP_DATA_LINK_TIMEOUT    2000

static phMwIf_sHandle_t  g_mwIfHandle;

typedef enum Dta_RecoveryState
{
    eState_RestartDiscovery = 0x00,
    eState_DiscoveryStop,
    eState_Invalid = 0xFF
}Dta_RecoveryState_t;

/**
* Initialize Middleware Interface
* Initialize Middleware and all the required resources
*/
MWIFSTATUS phMwIf_Init(void** mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    OSALSTATUS dwOsalStatus = OSALSTATUS_FAILED;
    phOsal_QueueCreateParams_t sQueueCreatePrms;

    ALOGD("MwIf>%s:enter",__FUNCTION__);
    phMwIfi_IncreaseStackSize();
    phMwIfi_GlobalDtaModeInit();
    NFA_EnableDtamode(NFA_DTA_DEFAULT_MODE);
    if(phMwIfi_ConfigureDriver(mwIfHdl) != NFA_STATUS_OK)
    {
        ALOGE("MwIf>%s:ConfigDriver Failed",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    ALOGD("DTALib> Creating queue to push/pull messages sent from clbaks to dtalib\n");
    sQueueCreatePrms.memHdl = NULL;
    sQueueCreatePrms.MemAllocCb = phMwIfi_MemAllocCb;
    sQueueCreatePrms.MemFreeCb = phMwIfi_MemFreeCb;
    sQueueCreatePrms.wQLength = 10;
    sQueueCreatePrms.eOverwriteMode = PHOSAL_QUEUE_NO_OVERWRITE;

    dwOsalStatus = phOsal_QueueCreate(&mwIfHdl->pvQueueHdl, &sQueueCreatePrms);
    if(dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGE("DTALib> Unable to create Queue");
        return MWIFSTATUS_FAILED;
    }

    dwOsalStatus = phOsal_SemaphoreCreate(&mwIfHdl->pvSemIntgrnThrd, 0, 0);
    if(dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGE("MwIf>%s:sem Create Failed",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    mwIfHdl->blStopIntegrationThrd=FALSE;
    dwOsalStatus = phOsal_ThreadCreate(&mwIfHdl->sIntegrationThrdHdl,
                                       phMwIfi_IntegrationThread,
                                       mwIfHdl);
    if( dwOsalStatus != OSALSTATUS_SUCCESS)
    {
            ALOGD("MwIf>%s:Thread Create Failed1",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    if(phMwIfi_StackInit(mwIfHdl) != NFA_STATUS_OK)
    {
        ALOGE("MwIf>%s:Stack Init Failed",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    *mwIfHandle = (void*)mwIfHdl;
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* De-Initialize Middle-ware Interface Library
*/
MWIFSTATUS phMwIf_DeInit(void* mwIfHandle)
{
    OSALSTATUS        dwOsalStatus;
    phMwIf_sHandle_t* mwIfHdl = mwIfHandle;
    MWIFSTATUS        dwMwIfStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    dwMwIfStatus = phMwIfi_StackDeInit();
    if(dwMwIfStatus != MWIFSTATUS_SUCCESS)
    {
        ALOGE("MwIf>%s:Error in Deinitializing Stack",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    mwIfHdl->blStopIntegrationThrd=TRUE;
    dwOsalStatus = phOsal_SemaphorePost(mwIfHdl->pvSemIntgrnThrd);
    if(dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGE("DTALib> Unable to Semaphore Post");
        return MWIFSTATUS_FAILED;
    }

    dwOsalStatus = phOsal_ThreadDelete(mwIfHdl->sIntegrationThrdHdl);
    if( dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGD("MwIf>%s:Thread Create Failed1",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    dwOsalStatus = phOsal_SemaphoreDelete(mwIfHdl->pvSemIntgrnThrd);
    if(dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGE("DTALib> Unable to Delete Semaphore");
        return MWIFSTATUS_FAILED;
    }

    dwOsalStatus = phOsal_QueueDestroy(mwIfHdl->pvQueueHdl);
    if(dwOsalStatus != OSALSTATUS_SUCCESS)
    {
        ALOGE("DTALib> Unable to Delete Queue");
        return MWIFSTATUS_FAILED;
    }

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}
/**
* Register callbacks which will be invoked when required data is available
*/
MWIFSTATUS phMwIf_RegisterCallback(void* mwIfHandle,
                                   void* applHdl,
                                   phMWIf_EvtCb_t mwIfCb)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:enter",__FUNCTION__);
    if((mwIfCb == NULL) || (mwIfHandle == NULL))
        return MWIFSTATUS_INVALID_PARAM;

    mwIfHdl->appCbHdl = applHdl;
    mwIfHdl->mwIfCb    = mwIfCb;
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Get Required Middleware and firmware versions
*/
MWIFSTATUS phMwIf_VersionInfo(void* mwIfHandle,
                              phMwIf_sVersionInfo_t* versionInfo)
{
    tNFC_FW_VERSION fwVersion;
    tNFA_MW_VERSION mwVersion;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    fwVersion = nfc_ncif_getFWVersion();
    mwVersion = NFA_GetMwVersion();
    versionInfo->fwVerMajor = fwVersion.major_version;
    versionInfo->fwVerMinor = fwVersion.minor_version;
    versionInfo->mwVerMajor = mwVersion.major_version;
    versionInfo->mwVerMinor = mwVersion.minor_version;
    ALOGD("MwIf>FwVersion:RomCodeVersion=0x%x,Major=0x%x,Minor=0x%x,",
        fwVersion.rom_code_version,fwVersion.major_version,fwVersion.minor_version);
    ALOGD("MwIf>MWVersion:Major=0x%x,Minor=0x%x,",
        mwVersion.major_version,mwVersion.minor_version);

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Set configuration for NFC controller before starting the discovery loop
 */
MWIFSTATUS phMwIf_SetConfig(void* mwIfHandle,
                             uint8_t uConfigID,
                             uint8_t ulen,
                             uint8_t * psBuf)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter;ParameterID 0x%02X \n",__FUNCTION__,uConfigID);
    phMwIfi_PrintBuffer(psBuf,ulen,"MwIf> SetConfig Data: ");
    gx_status = NFA_SetConfig((tNFA_PMID)uConfigID,(UINT8)ulen,
                                          (UINT8 *)psBuf);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,
            "MwIf> Error Could not start SetConfig !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_DM_EVT_OFFSET+NFA_DM_SET_CONFIG_EVT),5000,
            "MwIf>Failed to recv SetConfig",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:Exit",__FUNCTION__);

    return NFA_STATUS_OK;
}

/**
 * Set proprietary configuration for NFC controller before starting the discovery loop
 */
MWIFSTATUS phMwIf_SetConfigProp(void* mwIfHandle,
                             uint16_t uConfigID,
                             uint8_t ulen,
                             uint8_t * psBuf)
{
    MWIFSTATUS       dwMwIfStatus;
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    uint8_t abConfigCmd[MAX_CONFIG_CMD_LEN];    /*={0x20, 0x02, bCmdLen, config no, uConfigID, ulen, psBuf}*/
    uint8_t bConfigIDCheck=0,bCmdLen=0;
    uint16_t wTotalLength=0;

    ALOGD("MwIf>%s:Enter;ParameterID 0x%02X \n",__FUNCTION__,uConfigID);

    abConfigCmd[wTotalLength++] = 0x20;
    abConfigCmd[wTotalLength++] = 0x02;
    bConfigIDCheck =  (uConfigID & 0xFF00) >> 8;
    if(bConfigIDCheck == 0xA0)
    {
        bCmdLen = bCmdLen + 2;
    }
    else
    {
        bCmdLen = bCmdLen + 1;
    }

    abConfigCmd[wTotalLength++] = bCmdLen + ulen + 2;   /* 1 for config number and 1 for len */
    abConfigCmd[wTotalLength++] = 0x01; /* Sending only one Configurations command or ID at one time */

    ALOGD("MwIf>DEBUG> ParameterID %X \n",bConfigIDCheck);
    if(bConfigIDCheck == 0xA0)
    {
        abConfigCmd[wTotalLength++] = (uConfigID & 0xFF00) >> 8;
        abConfigCmd[wTotalLength++] = (uConfigID & 0x00FF);
    }
    else
    {
        abConfigCmd[wTotalLength++] = uConfigID;
    }
    abConfigCmd[wTotalLength++] = ulen;
    memcpy(abConfigCmd+(wTotalLength),psBuf,ulen);
    wTotalLength = wTotalLength + ulen;
    phMwIfi_PrintBuffer(abConfigCmd, (wTotalLength),"DEBUG> Setconfig proprietary command:");
    phMwIfi_SendNxpNciCommand(mwIfHdl, wTotalLength, abConfigCmd, phMwIfi_NfaNxpNciPropCommRspCallback);

    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Configure and start the discovery loop
*/
MWIFSTATUS phMwIf_EnableDiscovery(void* mwIfHandle,
                                  phMwIf_sDiscCfgPrms_t* discCfgParams)
{
    tNFA_TECHNOLOGY_MASK discPollconfig;
    tNFA_TECHNOLOGY_MASK discListenConfig;
    MWIFSTATUS       dwMwIfStatus;
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD ("MwIf> %s: Enter\n",__FUNCTION__);
    /*Save the configuration*/
    mwIfHdl->sDiscCfg = *discCfgParams;
    mwIfHdl->blDiscAnalogMode = discCfgParams->discParams.blAnalogMode;
    ALOGD ("MwIf> %s: Analog=0x%x Enter\n",__FUNCTION__,mwIfHdl->blDiscAnalogMode);
    discPollconfig = (UINT8) (discCfgParams->discParams.Poll_A * NFA_TECHNOLOGY_MASK_A)|
                             (discCfgParams->discParams.Poll_B * NFA_TECHNOLOGY_MASK_B)|
                             (discCfgParams->discParams.Poll_F * NFA_TECHNOLOGY_MASK_F);
    discListenConfig =(UINT8) (discCfgParams->discParams.Listen_A * NFA_TECHNOLOGY_MASK_A)|
                              (discCfgParams->discParams.Listen_B * NFA_TECHNOLOGY_MASK_B)|
                              (discCfgParams->discParams.Listen_F * NFA_TECHNOLOGY_MASK_F);

    phMwIfi_DiscoveryConfig(discPollconfig, discListenConfig, 1000);
    ALOGD ("MwIf> Discovery configuration Done \n");

    /* RF Discovery Loop to Start */
    dwMwIfStatus = phMwIfi_DiscoveryStart(mwIfHdl);
    if(dwMwIfStatus != MWIFSTATUS_SUCCESS)
    {
        ALOGD ("MwIf> Error in Discovery Start \n");
        return MWIFSTATUS_FAILED;
    }
    mwIfHdl->eDeviceState = DEVICE_IN_DISCOVERY_STARTED_STATE;

    ALOGD ("%s: Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* start the discovery loop
*/
MWIFSTATUS phMwIfi_DiscoveryStart(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD ("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_StartRfDiscovery();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start RfDiscovery !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_RF_DISCOVERY_STARTED_EVT,5000,
            "MwIf> Error! in Discovery Start",&(mwIfHdl->sLastQueueData));
    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* stop the discovery loop
*/
MWIFSTATUS phMwIfi_DiscoveryStop(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD ("MwIf>%s:Enter\n",__FUNCTION__);
    /*If the device is already disconnected,stop isn't reqd*/
    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState == DEVICE_IN_IDLE_STATE)
    {/*Flush the remaining events in the queue*/
        phOsal_QueueFlush(mwIfHdl->pvQueueHdl);
    }
    else
    {
        gx_status = NFA_StopRfDiscovery();
        PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf>ERROR To start StopRfDiscovery !!\n");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_RF_DISCOVERY_STOPPED_EVT,5000,
                "MwIf>ERROR! in Discovery Stop",&(mwIfHdl->sLastQueueData));
    }
    mwIfHdl->blDiscAnalogMode = FALSE;
    /*Reset Buffer Data length to avoid using previous chained data */
    Pgui_cbTempBufferLength = 0;

    /* All Polling technologies are disabling */
    gx_status = NFA_DisablePolling ();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not stop DisablePolling !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_POLL_DISABLED_EVT,5000,
            "MwIf> ERROR Polling Disable",&(mwIfHdl->sLastQueueData));

    /* All Listening technologies are disabling */
    gx_status = NFA_DisableListening ();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not stop DisableListening !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_LISTEN_DISABLED_EVT,5000,
            "MwIf> ERROR Listening Disable",&(mwIfHdl->sLastQueueData));

    if(mwIfHdl->sDiscCfg.enableCE)
    {
        if(mwIfHdl->sDiscCfg.ceType == PHMWIF_UICC_CE)
        {
            ALOGD("MwIf> Disable NFA_CeConfigureUiccListenTech\n");
            gx_status = NFA_CeConfigureUiccListenTech(NfaUiccHandle, 0x00);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not Disable ConfigureUiccListen !! \n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_UICC_LISTEN_CONFIGURED_EVT,5000,
                    "MwIf> ERROR in Disable ConfigureUiccListen",&(mwIfHdl->sLastQueueData));
        }
        else if(mwIfHdl->sDiscCfg.ceType == PHMWIF_SE_CE)
        {
            ALOGD("MwIf> Disable NFA_CeConfigureEseListenTech\n");
            gx_status = NFA_CeConfigureEseListenTech(NfaEseHandle, 0x00);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not Disable ConfigureEseListen !! \n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_ESE_LISTEN_CONFIGURED_EVT,5000,
                    "MwIf> ERROR Disable ConfigureEseListen",&(mwIfHdl->sLastQueueData));
        }
        else if(mwIfHdl->sDiscCfg.ceType == PHMWIF_HOST_CE)
        {
            ALOGD ("MwIf> HCE AID Handle De-registration =0x%x\n",NfaAidHandle);
            gx_status = NFA_CeDeregisterAidOnDH(NfaAidHandle);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"NFA CE_AID De-registration Fail!! \n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_DEREGISTERED_EVT,5000,
                    "MwIf> ERROR disable P2P listening) !! \n",&(mwIfHdl->sLastQueueData));
        }
        mwIfHdl->sDiscCfg.enableCE = FALSE;
    }
    else
    {
        gx_status = NFA_SetP2pListenTech(0x00);
        PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Request to disable P2P listening !! \n");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SET_P2P_LISTEN_TECH_EVT,5000,
                "MwIf> ERROR disable P2P listening) !! \n",&(mwIfHdl->sLastQueueData));
    }
    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Stop and disable discovery
*/
MWIFSTATUS phMwIf_DisableDiscovery(void* mwIfHandle)
{
    MWIFSTATUS dwMwStatus=0;
    ALOGD ("MwIf>%s:Enter\n",__FUNCTION__);
    dwMwStatus = phMwIfi_DiscoveryStop(mwIfHandle);
    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
    return dwMwStatus;
}
/**
* Initialize secure execution environment.
*/
MWIFSTATUS phMwIf_EeInit(void* mwIfHandle,
                         phMwIf_eCeDevType_t eDevType)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_STATUS_FAILED;

    /*Register Callback for NFCEE Events*/
    gx_status = NFA_EeRegister(phMwIfi_NfaEeCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf>ERROR EE Register !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_REGISTER_EVT,5000,
            "MwIf>ERROR in EE register",&(mwIfHdl->sLastQueueData));

    /*Get the list of EE's*/
    ALOGD("MwIf> Get Information of all EE's of type 0x%x\n",eDevType);
    gx_status = phMwIfi_EeGetInfo(mwIfHdl,eDevType);
    if (gx_status != NFA_STATUS_OK)
    {
        ALOGE("MwIf> Error getting Secure Element info \n");
        return gx_status;
    }
    ALOGD("MwIf>%s:SE GetInfo Done\n",__FUNCTION__);

    phMwIfi_HciRegister(mwIfHdl);
    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
    return gx_status;
}

/**
* Configure secure execution environment.
*/
MWIFSTATUS phMwIf_EeConfigure(void* mwIfHandle,
                              phMwIf_eCeDevType_t eDevType,
                              phMwIf_sRfTechs_t*  psRoutingTech)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    tNFA_TECHNOLOGY_MASK technologiesSwitchOn;
    tNFA_HANDLE       nfaEeHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    technologiesSwitchOn = (UINT8) (psRoutingTech->Listen_A * NFA_TECHNOLOGY_MASK_A)|
                             (psRoutingTech->Listen_B * NFA_TECHNOLOGY_MASK_B)|
                             (psRoutingTech->Listen_F * NFA_TECHNOLOGY_MASK_F);
    ALOGD("MwIf>Device Type:0x%x,technologiesSwitchOn=0x%x\n",eDevType,technologiesSwitchOn);

    if(eDevType == PHMWIF_UICC_CE)
    {
        nfaEeHandle = NfaUiccHandle;
    }
    else if(eDevType == PHMWIF_SE_CE)
    {
        nfaEeHandle = NfaEseHandle;
    }
    else
    {
        ALOGE("MwIf>%s:Invalid Param eDevType\n",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }
    gx_status = phMwIfi_EeSetDefaultTechRouting(nfaEeHandle,technologiesSwitchOn,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Tech Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_SET_TECH_CFG_EVT,5000,
            "MwIf> ERROR in EeSetDefaultTechRouting",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Going for Proto Route Set\n");
    gx_status = phMwIfi_EeSetDefaultProtoRouting(nfaEeHandle,NFA_PROTOCOL_MASK_ISO_DEP,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Proto Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_SET_PROTO_CFG_EVT,5000,
            "MwIf> ERROR in EeSetDefaultProtoRouting",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Going for EE Update now \n");
    gx_status = NFA_EeUpdateNow();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Update Now !!\n");

    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* DeInitialize secure execution environment.
*/
MWIFSTATUS phMwIf_EeDeInit(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);

    /*DERegister Callback for NFCEE Events*/
    gx_status = NFA_EeDeregister(phMwIfi_NfaEeCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE DeRegister !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_DEREGISTER_EVT, 5000,
            "MwIf> ERROR in EeDeregister",&(mwIfHdl->sLastQueueData));

    /*Deregister application*/
    gx_status = NFA_HciDeregister(APP_NAME);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR HCI DeRegister !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_HCI_DEREGISTER_EVT,5000,
            "MwIf> ERROR in HciDeregister",&(mwIfHdl->sLastQueueData));

    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Initialize Host Card Emulation
*/
MWIFSTATUS phMwIf_HceInit(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_STATUS_FAILED;

    /*Register Callback for NFCEE Events*/
    gx_status = NFA_EeRegister(phMwIfi_NfaEeCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf>ERROR EE Register !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET + NFA_EE_REGISTER_EVT),5000,
            "MwIf>ERROR in EE register",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Configure routing and params for Host Card Emulation
*/
MWIFSTATUS phMwIf_HceConfigure(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_STATUS_FAILED;

    ALOGD("MwIf> Going for configure NCI params for HCE\n");
    phMwIfi_HceConfigNciParams(mwIfHdl);

    ALOGD("MwIf> Clear tech route for Host\n");
    gx_status = phMwIfi_EeSetDefaultTechRouting(NfaHostHandle,0x0,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Tech Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET+NFA_EE_SET_TECH_CFG_EVT),5000,
            "MwIf> ERROR in EeSetDefaultTechRouting",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Clear proto route for Host\n");
    gx_status = phMwIfi_EeSetDefaultProtoRouting(NfaHostHandle,0x0,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Proto Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET+NFA_EE_SET_PROTO_CFG_EVT),5000,
            "MwIf> ERROR in EeSetDefaultProtoRouting ",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Going for Tech Route Set\n");
    gx_status = phMwIfi_EeSetDefaultTechRouting(NfaHostHandle,0x3,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Tech Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET + NFA_EE_SET_TECH_CFG_EVT),5000,
            "MwIf> ERROR in EeSetDefaultTechRouting",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Going for Proto Route Set\n");
    gx_status = phMwIfi_EeSetDefaultProtoRouting(NfaHostHandle,NFA_PROTOCOL_MASK_ISO_DEP,0x0,0x0);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Set Default Proto Route !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET+NFA_EE_SET_PROTO_CFG_EVT),5000,
            "MwIf> ERROR in EeSetDefaultProtoRouting ",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Going for EE Update Now\n");
    gx_status = NFA_EeUpdateNow();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Update Now !!\n");
    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);

    return MWIFSTATUS_SUCCESS;
}

/**
* DeInitialize Host Card Emulation
*/
MWIFSTATUS phMwIf_HceDeInit(void* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_STATUS_FAILED;

    /*DERegister Callback for NFCEE Events*/
    gx_status = NFA_EeDeregister(phMwIfi_NfaEeCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE DeRegister");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_EE_EVT_OFFSET+NFA_EE_DEREGISTER_EVT), 5000,
            "MwIf> ERROR in EeDeregister",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * This function is for operations on tag(Read/Write).
 * Type of tag is assumed to be the previouly activated one
 */
MWIFSTATUS phMwIf_TagOperation(void* mwIfHandle,
                               phMwIf_eTagOpsType_t       eTagOpsType,
                               phMwIf_uTagOpsParams_t*    psTagOpsParams)
{
    MWIFSTATUS dwMwIfStatus;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);

    if(!mwIfHandle)
        return MWIFSTATUS_INVALID_PARAM;

    switch(eTagOpsType)
    {
    case PHMWIF_TAGOP_CHECK_NDEF:
    dwMwIfStatus = phMwIfi_CheckNdef(mwIfHandle,
                                    (phMwIf_sNdefDetectParams_t*)psTagOpsParams);
    break;
    case PHMWIF_TAGOP_READ_NDEF:
    dwMwIfStatus = phMwIfi_ReadNdef(mwIfHandle,
                                    (phMwIf_sBuffParams_t*)psTagOpsParams);
    break;
    case PHMWIF_TAGOP_WRITE_NDEF:
    dwMwIfStatus = phMwIfi_WriteNdef(mwIfHandle,
                                    ((phMwIf_sBuffParams_t*)psTagOpsParams)->pbBuff,
                                    ((phMwIf_sBuffParams_t*)psTagOpsParams)->dwBuffLength);
    break;
    case PHMWIF_TAGOP_SET_READONLY:
    dwMwIfStatus = phMwIfi_SetTagReadOnly(mwIfHandle,
                                            TRUE);
    break;
    default:
    ALOGE("MwIf>%s:Error:Wrong Tag Operations Type",__FUNCTION__);
    dwMwIfStatus = MWIFSTATUS_FAILED;
    }
    if(dwMwIfStatus != MWIFSTATUS_SUCCESS)
    {
        ALOGE("MwIf>%s:Error in Tag Operation",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * This function is for specific command for particular tag type
 */
MWIFSTATUS phMwIf_TagCmd(void*                  mwIfHandle,
                         phMwIf_eProtocolType_t eProtocolType,
                         phMwIf_uTagParams_t*   psTagParams)
{
    MWIFSTATUS dwMwIfStatus;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);

    if(!mwIfHandle)
        return MWIFSTATUS_INVALID_PARAM;

    switch(eProtocolType)
    {
    case PHMWIF_PROTOCOL_T1T:
    break;
    case PHMWIF_PROTOCOL_T2T:
        dwMwIfStatus = phMwIfi_HandleT2TCmd(mwIfHandle,
                                            (phMwIf_sT2TParams_t*)psTagParams);
    break;
    case PHMWIF_PROTOCOL_T3T:
    break;
    case PHMWIF_PROTOCOL_ISO_DEP:
    break;
    case PHMWIF_PROTOCOL_NFC_DEP:
    break;
    case PHMWIF_PROTOCOL_INVALID:
    default:
    ALOGE("MwIf>%s:Error:Wrong Tag Command Type",__FUNCTION__);
    dwMwIfStatus = MWIFSTATUS_FAILED;
    }
    if(dwMwIfStatus != MWIFSTATUS_SUCCESS)
    {
        ALOGE("MwIf>%s:Error in Tag Operation",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}
/**
 * Handle commands specific to T2T Tags
 * */
MWIFSTATUS phMwIfi_HandleT2TCmd(void*                  mwIfHandle,
                                phMwIf_sT2TParams_t*   psTagParams)
{
    MWIFSTATUS dwMwIfStatus;
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);

    if((!mwIfHandle) || (!psTagParams))
        return MWIFSTATUS_INVALID_PARAM;

    switch(psTagParams->eT2TCmd)
    {
        case PHMWIF_T2T_READ_CMD: /*Read block of data*/
        {
            ALOGD("MwIf>%s:Reading data from Block=%d",__FUNCTION__,psTagParams->dwBlockNum);
            gx_status = NFA_RwT2tRead((UINT8)psTagParams->dwBlockNum);
            PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,"MwIf> Error starting T2T Blk read !! \n");

            /*Wait until Read is completed*/
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_READ_CPLT_EVT,5000,
            "MwIf>Error in Tag Read ",&(mwIfHdl->sLastQueueData));

            memcpy(psTagParams->sBuffParams.pbBuff,Pgs_cbTempBuffer,Pgui_cbTempBufferLength);
            psTagParams->sBuffParams.dwBuffLength = Pgui_cbTempBufferLength;
            /*Reset Buffer Data length to avoid using previous chained data */
            Pgui_cbTempBufferLength = 0;
            phMwIfi_PrintBuffer(psTagParams->sBuffParams.pbBuff, psTagParams->sBuffParams.dwBuffLength,"T2T Data Read:");
         }
        break;
        case PHMWIF_T2T_WRITE_CMD: /*Write Block of data*/
        {
            ALOGD("MwIf>%s:Writing data to Block=%d",__FUNCTION__,psTagParams->dwBlockNum);
            phMwIfi_PrintBuffer(psTagParams->sBuffParams.pbBuff,4,"T2T Data Write:");
            gx_status = NFA_RwT2tWrite((UINT8)psTagParams->dwBlockNum,(UINT8 *)psTagParams->sBuffParams.pbBuff);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK, gx_status,"MwIf> Error starting T2T Blk write !!\n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_WRITE_CPLT_EVT, 5000,
            "MwIf> Tag/Device T2T Blk write Error (SEM) !!",&(mwIfHdl->sLastQueueData));
        }
        break;
        case PHMWIF_T2T_SECTOR_SELECT_CMD:
        {
             ALOGD("MwIf>%s:Selecting Sector=%d",__FUNCTION__,psTagParams->dwSectorNum);
             gx_status = NFA_RwT2tSectorSelect(psTagParams->dwSectorNum);
             PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status, "MwIf> Error Could not start NDEF Detection !! \n");
             PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SELECT_CPLT_EVT,5000, "MwIf> Error Check NDEF Error (SEM) !! \n",&(mwIfHdl->sLastQueueData));
        }
        break;
        case PHMWIF_T2T_RAW_CMD:
        break;
        default:
            ALOGE("MwIf>%s:Error:Wrong Tag Command Type",__FUNCTION__);
            return MWIFSTATUS_FAILED;
        break;
    }
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Read NDEF data from tag
*/
MWIFSTATUS phMwIfi_ReadNdef(void* mwIfHandle,
                            phMwIf_sBuffParams_t* psBuffParams)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_RwReadNDef();
    PH_ON_ERROR_RETURN(NFA_STATUS_OK, gx_status,"MwIf> ERROR in NFA_RWReadNDef !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_READ_CPLT_EVT,5000,
            "MwIf> ERROR in NFA_RWReadNDef",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Set the Tag as read only
 */
MWIFSTATUS phMwIfi_SetTagReadOnly(void* mwIfHandle,
                                  BOOLEAN readOnly)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);
    gx_status = NFA_RwSetTagReadOnly(readOnly);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,"MwIf> Error starting NDEF Transition to Read Only");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SET_TAG_RO_EVT,5000,
            "MwIf>Error in Tag Read Only Transition",&(mwIfHdl->sLastQueueData));
    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 *Write NDEF data to Tag
 */
MWIFSTATUS phMwIfi_WriteNdef(void* mwIfHandle,
                                    uint8_t* pData,
                                    uint32_t length)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);

    gx_status = phMwIfi_CreateNdefMsg(mwIfHandle,
                                      pData,
                                      length);

    gx_status = NFA_RwWriteNDef(gs_paramBuffer,gs_sizeParamBuffer);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR in NFA_RWWriteNDef !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_WRITE_CPLT_EVT, 5000,
            "MwIf> ERROR in NFA_RWWriteNDef",&(mwIfHdl->sLastQueueData));
    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Check whether Tag is NDEF compliant or not
 */
MWIFSTATUS phMwIfi_CheckNdef(void*                       mwIfHandle,
                            phMwIf_sNdefDetectParams_t* psNdefDetectParams)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter\n",__FUNCTION__);

    gx_status = NFA_RwDetectNDef();
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,
        "MwIf> Error Could not start NDEF Detection !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_NDEF_DETECT_EVT,5000,
            "MwIf> Error Check NDEF Error (SEM) !! \n",&(mwIfHdl->sLastQueueData));

    if(!gb_device_ndefcompliant)
    {
      ALOGD("MwIf> Tag is an NOT NDEF Complaint \n");
      return MWIFSTATUS_FAILED;
    }

    *psNdefDetectParams= mwIfHdl->sNdefDetectParams;
    ALOGD("MwIf> Tag is an NDEF Complaint \n");
    ALOGD("MwIf>%s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
* Increase Stack Size as the current stack size alotted isn't sufficient for the complete app
*/
void phMwIfi_IncreaseStackSize(void)
{
    struct rlimit xrlim;
    const rlim_t required_size = 10L * 1024L * 1024L;/**< min stack size = 10MB */
    int32_t current_size;
    /*Get Current Stack Size*/
    current_size = getrlimit(RLIMIT_STACK,&xrlim);
    if(current_size != 0)
    {
        PH_HANDLE_ERROR_EN(current_size, "MwIf> ERROR getrlimit");
    }
    else
    {
        ALOGD("MwIf> Current Stack: %ld MB \n", ( ((uint32_t)(xrlim.rlim_cur))
                                                   /(1024L*1024L) ));
        /* < Verify the Size */
        if(xrlim.rlim_cur < required_size)
        {
            xrlim.rlim_cur = required_size;
            /* < Set the Parameters */
            current_size = setrlimit(RLIMIT_STACK,(const struct rlimit *)&xrlim);
            if(current_size != 0)
                PH_HANDLE_ERROR_EN(current_size, "MwIf> ERROR setrlimit\n");
            ALOGD("MwIf> Stack Increase of 10MB Successful \n");
        }
        else
        {
            ALOGE("MwIf> ERROR Can't set the required stack size !! \n");
        }
    }
}

void phMwIfi_GlobalDtaModeInit(void)
{
    ALOGD ("MwIf>Setting DTA mode in Mw\n");
    initializeGlobalAppDtaMode();
}

tNFA_STATUS phMwIfi_ConfigureDriver(phMwIf_sHandle_t *mwIfHdl)
{
    gx_status = NFA_STATUS_FAILED;
    ALOGD ("MwIf> NfcAdaptation Initialize\n");
    mwIfHdl->nfcAdapter = NfcAdaptation_Create();
    NfcAdaptation_Initialize(mwIfHdl->nfcAdapter);
    ALOGD ("MwIf> Getting function pointers for HAL entry functions\n");
    mwIfHdl->halentrypoint = NfcAdaptation_GetHalEntryFuncs(mwIfHdl->nfcAdapter);
    ALOGD ("MwIf> NFA_Init\n");
    NFA_Init(mwIfHdl->halentrypoint);
    return MWIFSTATUS_SUCCESS;
}

tNFA_STATUS phMwIfi_StackInit(phMwIf_sHandle_t *mwIfHdl)
{
    ALOGD ("MwIf> NFA Enable \n");
    gx_status = NFA_Enable (phMwIfi_NfaDevMgmtCallback,
                             phMwIfi_NfaConnCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not Start \
                                   NFA Enable\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DM_EVT_OFFSET+NFA_DM_ENABLE_EVT,15000,
            "MwIf> ERROR NFA Enable (SEM)",&(mwIfHdl->sLastQueueData));
    return MWIFSTATUS_SUCCESS;
}

tNFA_STATUS phMwIfi_StackDeInit()
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    ALOGD ("MwIf>%s:Enter",__FUNCTION__);
    /**< Now Deinit the Stack */
    ALOGD ("MwIf> NFA_Disable\n");
    gx_status = NFA_Disable(TRUE);
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DM_EVT_OFFSET+NFA_DM_DISABLE_EVT,5000,
            "MwIf> ERROR NFA Disable",&(mwIfHdl->sLastQueueData));
    if(gx_status != NFA_STATUS_OK)
    {
        ALOGE("MwIf> Error in NFA Disable with Status Code !! - \n");
    }

    ALOGD ("MwIf> NFA Disable Done \n");
    return MWIFSTATUS_SUCCESS;
}

void* phMwIfi_IntegrationThread(void *arg)
{
    OSALSTATUS dwOsalStatus;
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    phMwIf_sDiscCfgPrms_t sPrevdiscCfgParams;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    while(1)
    {
        ALOGD("MwIf>%s:Waiting for Semaphore",__FUNCTION__);
        dwOsalStatus = phOsal_SemaphoreWait(mwIfHdl->pvSemIntgrnThrd,0);
        if(dwOsalStatus != OSALSTATUS_SUCCESS)
        {
            ALOGE("MwIf>%s:Error! in wait:Exiting Thread",__FUNCTION__);
            break;
        }

        if(mwIfHdl->blStopIntegrationThrd == TRUE)
        {
            ALOGD("MwIf>%s:Stopping thread",__FUNCTION__);
            break;
        }
        if(gb_discovery_notify)
        {
            int rfInterface;
            UINT8 protocol = gx_discovery_result.discovery_ntf.protocol;
            gb_discovery_notify = FALSE;
            ALOGD("MwIf>%s:Handling Discovery Notification",__FUNCTION__);
            phMwIfi_MapRfInterface(&protocol, &rfInterface);

            gx_status = NFA_Select(gx_discovery_result.discovery_ntf.rf_disc_id,
                                   gx_discovery_result.discovery_ntf.protocol,
                                   rfInterface);
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SELECT_RESULT_EVT,5000,
                    "MwIf> ERROR NFA Select",&(mwIfHdl->sLastQueueData));
        }


        /*Restart the discovery*/
        sPrevdiscCfgParams = mwIfHdl->sDiscCfg;
        phMwIf_DisableDiscovery(mwIfHdl);
        phMwIf_EnableDiscovery(mwIfHdl,&sPrevdiscCfgParams);
    }
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return NULL;
}

/**
 * \brief Registers ndef callback
 * /return NFA_STATUS_OK - if the initialization was successful
 *         NFA_STATUS_FAILED - in case of errors with Semaphore post
 */
tNFA_STATUS phMwIfi_NDEFregister()
{
  phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
  uint8_t Pgs_NdefTypeText[1]={'T'};/**< NDEF Type for registering the Handler */

  gx_status = NFA_RegisterNDefTypeHandler((BOOLEAN)TRUE,
        (tNFA_TNF)NFA_TNF_DEFAULT,
        (UINT8 *)Pgs_NdefTypeText,
        sizeof(Pgs_NdefTypeText),
        &phMwIfi_NfaNdefCallback);
  PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,
      "MwIf> ERROR in Starting Register NDEF Handler !! \n");
  PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_NDEF_REGISTER_EVT,5000,
          "MwIf> ERROR Register NDEF Handler",&(mwIfHdl->sLastQueueData));
  return gx_status;
}

/**
 * \brief Configure Discovery
 * /return NFA_STATUS_OK - if the discovery configuration was successful
 *         NFA_STATUS_FAILED - in case of errors
 */
tNFA_STATUS phMwIfi_DiscoveryConfig(tNFA_TECHNOLOGY_MASK xpollmask,
                                    tNFA_TECHNOLOGY_MASK xlistenmask,
                                    uint16_t uiLoopMs)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    ALOGD ("MwIf> %s:Enter:PollMask:0x%x,ListenMask:0x%x\n",__FUNCTION__,xpollmask,xlistenmask);

    if(mwIfHdl->blDiscAnalogMode)
    {
        gx_status = NFA_EnablePolling (NFA_TECHNOLOGY_MASK_A |
                                       NFA_TECHNOLOGY_MASK_B |
                                       NFA_TECHNOLOGY_MASK_F);
        PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,
                "MwIf> Error Could not start EnablePolling !! \n");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_POLL_ENABLED_EVT,5000,
                "MwIf> ERROR Polling Enable",&(mwIfHdl->sLastQueueData));
        ALOGD ("MwIf> NFA_SetP2pListenTech\n");
        gx_status = NFA_SetP2pListenTech(NFA_TECHNOLOGY_MASK_F);
        PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start SetP2pListen !! \n");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SET_P2P_LISTEN_TECH_EVT,5000,
                "MwIf> ERROR SetP2pListen (SEM) !! \n",&(mwIfHdl->sLastQueueData));
        if(mwIfHdl->sDiscCfg.enableCE)
        {
            if(mwIfHdl->sDiscCfg.ceType == PHMWIF_UICC_CE)
            {
                ALOGD("MwIf> NFA_CeConfigureUiccListenTech\n");
                gx_status = NFA_CeConfigureUiccListenTech(NfaUiccHandle, NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start ConfigureUiccListen !! \n");
                PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_UICC_LISTEN_CONFIGURED_EVT,5000,
                        "MwIf> ERROR ConfigureUiccListen (SEM) !! \n",&(mwIfHdl->sLastQueueData));
            }
            else
            {
                ALOGD("MwIf> NFA_CeSetListenDepTech\n");
                gx_status = NFA_CeSetIsoDepListenTech(NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start SetIsoDepListenTech !! \n");

                ALOGD("MwIf> Going for AID Registration on DH\n");
                phMwIfi_CeRegisterAID(mwIfHdl);
            }
        }
    }
    else
    {
        gx_status = NFA_EnablePolling (xpollmask);
        PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,
                "MwIf> Error Could not start EnablePolling !! \n");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_POLL_ENABLED_EVT,5000,
                "MwIf> ERROR Polling Enable",&(mwIfHdl->sLastQueueData));
        ALOGD("MwIf>ceMode=0x%x; UICC initialised=0x%x",mwIfHdl->sDiscCfg.enableCE, mwIfHdl->sDiscCfg.ceType);
        if(mwIfHdl->sDiscCfg.enableCE)
        {
            if(mwIfHdl->sDiscCfg.ceType == PHMWIF_UICC_CE)
            {
                ALOGD("MwIf> NFA_CeConfigureUiccListenTech\n");
                gx_status = NFA_CeConfigureUiccListenTech(NfaUiccHandle, xlistenmask);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start ConfigureUiccListen !! \n");
                PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_UICC_LISTEN_CONFIGURED_EVT,5000,
                        "MwIf> ERROR ConfigureUiccListen",&(mwIfHdl->sLastQueueData));
            }
            else if(mwIfHdl->sDiscCfg.ceType == PHMWIF_SE_CE)
            {
                ALOGD("MwIf> NFA_CeConfigureEseListenTech\n");
                gx_status = NFA_CeConfigureEseListenTech(NfaEseHandle, xlistenmask);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start ConfigureEseListen !! \n");
                PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_ESE_LISTEN_CONFIGURED_EVT,5000,
                        "MwIf> ERROR ConfigureEseListen",&(mwIfHdl->sLastQueueData));
            }
            else if(mwIfHdl->sDiscCfg.ceType == PHMWIF_HOST_CE)
            {
                if(xlistenmask & NFA_TECHNOLOGY_MASK_F)
                {
                    /*HCE/HBE is supported only for Type A & B*/
                    ALOGD("MwIf> HCE/HBE Type F is not supported. Masking Type F\n");
                    xlistenmask = xlistenmask & (~NFA_TECHNOLOGY_MASK_F);
                }
                ALOGD("MwIf> NFA_CeSetListenDepTech\n");
                gx_status = NFA_CeSetIsoDepListenTech(xlistenmask);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start SetIsoDepListenTech !! \n");

                ALOGD("MwIf> Going for AID Registration on DH\n");
                phMwIfi_CeRegisterAID(mwIfHdl);
            }
            else
            {
                ALOGE("MwIf>Invalid CE Type Configured");
            }
        }
        else
        {
            if(xlistenmask & NFA_TECHNOLOGY_MASK_B)
            {
                /*P2P is supported only for Type A & F*/
                ALOGD("MwIf> P2P Type B is not supported. Masking Type B\n");
                xlistenmask = xlistenmask & (~NFA_TECHNOLOGY_MASK_B);
            }
            ALOGD ("MwIf> NFA_SetP2pListenTech\n");
            gx_status = NFA_SetP2pListenTech(xlistenmask);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not start SetP2pListen !! \n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_SET_P2P_LISTEN_TECH_EVT,5000,
                    "MwIf> ERROR SetP2pListen",&(mwIfHdl->sLastQueueData));
        }
    }
    gx_status = NFA_EnableListening();
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> Error Could not Enable Listening !! \n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_LISTEN_ENABLED_EVT,5000,
            "MwIf> ERROR Enable Listen",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf> Set Discovery Loop Duration to %d milliSecond\n",uiLoopMs);
    gx_status = NFA_SetRfDiscoveryDuration(uiLoopMs);/**< n Second Discovery Duration */
    PH_ON_ERROR_EXIT(NFA_STATUS_OK,2,"MwIf> ERROR Failed Set Discovery Duration to correct time !! \n");
    ALOGD ("MwIf> %s:Exit\n",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * \brief This is used by BCM stack to trigger all ndef related events/data etc
 */
void phMwIfi_NfaNdefCallback (tNFA_NDEF_EVT xevent, tNFA_NDEF_EVT_DATA *px_data)
{
    uint16_t i;
    phMwIf_sQueueData_t* psQueueData = NULL;
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    ALOGD ("MwIf>%s:Enter:Event=0x%02x",__FUNCTION__,xevent);

    phMwIfi_PrintNDEFEventCode(xevent);
    Pgu_event = xevent;
    if(px_data != NULL)
    {
        ALOGD ("MwIf>Status = ");
        phMwIfi_PrintNfaStatusCode(px_data->ndef_reg.status);
        gx_status = px_data->ndef_reg.status;

        switch(xevent)
        {
            case NFA_NDEF_REGISTER_EVT:
                gx_ndef_type_handle = px_data->ndef_reg.ndef_type_handle;
                break;
            case NFA_NDEF_DATA_EVT:
                if(px_data->ndef_data.len != 0 && gx_status == NFA_STATUS_OK)
                {
                    if(px_data->ndef_data.len < 400)
                    {
                        /* Copy the Data */
                        Pgui_cbTempBufferLength = px_data->ndef_data.len;
                        memcpy(Pgs_cbTempBuffer,px_data->ndef_data.p_data,
                                Pgui_cbTempBufferLength);
                        gx_ndef_data = *px_data;

                        ALOGD ("MwIf> ******* NDEF DATA *******\n");
                        ALOGD ("MwIf> ndef_type_handle = %u\n",
                                px_data->ndef_data.ndef_type_handle);
                        ALOGD ("MwIf> NDEF MSG Len = %u\n",
                                Pgui_cbTempBufferLength);
                        ALOGD ("MwIf> NDEF Message(Char) = ");
                        for(i=0;i<Pgui_cbTempBufferLength;i++)
                        {
                            ALOGD("%c",Pgs_cbTempBuffer[i]);
                        }
                        ALOGD("\n");
                        gx_status = NFA_STATUS_OK;
                    }
                    else
                    {
                        ALOGE("MwIf> Error Data too big %ld > 400 !! \n",
                                px_data->ndef_data.len);
                        gx_status = NFA_STATUS_BAD_LENGTH;
                    }
                    phMwIfi_PrintBuffer(px_data->ndef_data.p_data,px_data->ndef_data.len,
                                    "MwIf> NDEF Message(RAW) = ");
                }
                else
                {
                    ALOGE("MwIf>Error no Data received !! \n");
                    gx_status = NFA_STATUS_FAILED;
                }
                break;
            default:
                ALOGE("MwIf>Error EVENT code not defined !!\n");
                gx_status = NFA_STATUS_BAD_RESP;
                break;
        }
    }

    psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
    memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
    psQueueData->dwEvtType = xevent;
    if(px_data)
        psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*px_data;
    if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
    {
        ALOGE("DTA_HCIREGCB> Error Could not Post Semaphore !!\n");
        gx_status = NFA_STATUS_FAILED;
    }
    ALOGD ("MwIf>%s:Exit\n",__FUNCTION__);
}

/**
 * \brief This is callback function used by BCM stack to trigger all the events
 */
void phMwIfi_NfaDevMgmtCallback (UINT8 uevent, tNFA_DM_CBACK_DATA *px_data)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    phMwIf_sQueueData_t* psQueueData = NULL;
    ALOGD ("MwIf>%s:Enter:Event=0x%02x",__FUNCTION__,uevent);
    phMwIfi_PrintDMEventCode(uevent);
    Pgu_event = uevent;
    if(px_data != NULL)
    {
        ALOGD ("MwIf>Status = ");
        phMwIfi_PrintNfaStatusCode(px_data->status);
        gx_status = px_data->status;
    }
    /**< Check if there was not any Timeout */
    if(uevent == NFA_DM_NFCC_TIMEOUT_EVT)
    {
        ALOGE ("MwIf> Error TIMEOUT EVENT occurred !! \n");
        gx_status = NFA_STATUS_TIMEOUT;
        //return;
    }
    else if (uevent == NFA_DM_RF_FIELD_EVT)
    {
        if(px_data->rf_field.rf_field_status == NFA_DM_RF_FIELD_ON)
            ALOGE("MwIf>RF_Field ON Event !! \n");
        else
            ALOGE("MwIf>RF_Field OFF Event !! \n");
        gx_status = NFA_STATUS_BUSY;
        return;
    }

    psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
    memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
    psQueueData->dwEvtType = (NFA_DM_EVT_OFFSET+uevent);
    if(px_data)
        psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*px_data;
    if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
    {
        ALOGE("Mwif> Error Could not send DM event to q\n");
        gx_status = NFA_STATUS_FAILED;
    }
    ALOGD ("MwIf>%s:Exit",__FUNCTION__);
}

/**
 * \brief This is callback function used by BCM stack to trigger all nfc events
 * and data events
 */
void phMwIfi_NfaConnCallback (UINT8 uevent, tNFA_CONN_EVT_DATA *px_data)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    phMwIf_sQueueData_t* psQueueData = NULL;
    phMWIf_eDiscType_t  eDiscType;
    phMWIf_uEvtInfo_t   uEvtInfo;
    BOOLEAN             bCallbackReqd=FALSE;
    BOOLEAN             bPushToQReqd = TRUE;
    phMWIf_eEvtType_t   eMwIfEvtType;
    ALOGD ("MwIf>%s:Enter:Event=0x%02x",__FUNCTION__,uevent);
    Pgu_event = uevent;
    if(px_data != NULL)/* Get the Status */
    {
        ALOGD ("MwIf>Status = 0x%02x - ",px_data->status);
        gx_status = px_data->status;
        switch(uevent)
        {
            case NFA_POLL_ENABLED_EVT:
            break;
            case NFA_POLL_DISABLED_EVT:
            break;
            case NFA_LISTEN_ENABLED_EVT:
            break;
            case NFA_LISTEN_DISABLED_EVT:
            break;
            case NFA_DISC_RESULT_EVT:
            {
                tNFA_DISC_RESULT disc_result;
                memcpy(&disc_result,&px_data->disc_result, sizeof(tNFA_DISC_RESULT));
                if(!Pgu_disoveredDeviceCount)
                {
                    memcpy(&gx_discovery_result, &px_data->disc_result, sizeof(tNFA_DISC_RESULT));
                }
                ++Pgu_disoveredDeviceCount; /* Increment counter for each notification */

                if(disc_result.discovery_ntf.more == 0)
                {
                    Pgu_disoveredDeviceCount = 0;/* Clear the Counter */
                    gb_discovery_notify = TRUE;
                    phOsal_SemaphorePost(mwIfHdl->pvSemIntgrnThrd);
                }
                else
                {
                    return;
                }
            }
            break;
            case NFA_SELECT_RESULT_EVT:
                /*gb_device_connected = TRUE;*/
                mwIfHdl->eDeviceState = DEVICE_IN_POLL_ACTIVE_STATE;
            break;
            case NFA_DEACTIVATE_FAIL_EVT:
            break;
            case NFA_ACTIVATED_EVT:
            {
                bProtocol = px_data->activated.activate_ntf.protocol;
                phMwIfi_HandleActivatedEvent(&(px_data->activated),
                                             &bPushToQReqd,
                                             &bCallbackReqd,
                                             &uEvtInfo,
                                             &eMwIfEvtType);
                memcpy(&gx_device,&(px_data->activated),sizeof(tNFA_ACTIVATED));
                ALOGD("MwIf>Device Connected");
                gx_status = NFC_STATUS_OK;
            }/*case NFA_ACTIVATED_EVT:*/
            break;
            case NFA_DEACTIVATED_EVT:
            {
                tNFA_DEACTIVATED    *deactivated = px_data;
                gb_device_ndefcompliant = FALSE;
                bPushToQReqd = TRUE;
                /*gb_device_connected = FALSE;*/
                ALOGD("MwIf>Device DisConnected");
                bCallbackReqd = TRUE;
                if(deactivated->type == PHMWIF_DEACTIVATE_TYPE_IDLE)
                {
                    mwIfHdl->eDeviceState = DEVICE_IN_IDLE_STATE;
                }
                eMwIfEvtType = PHMWIF_DEACTIVATED_EVT;
                uEvtInfo.eDeactivateType = deactivated->type;
                /*If deactivated event is received in between Chained Data transfer,
                Reset Buffer Data length to avoid using previous chained data */
                Pgui_cbTempBufferLength = 0;
            }
            break;
            case NFA_TLV_DETECT_EVT :
            break;
            case NFA_NDEF_DETECT_EVT :
            {
                gb_device_ndefcompliant = FALSE;
                ALOGD ("MwIf> NDEF Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->ndef_detect.status);
                if(px_data->ndef_detect.status == NFA_STATUS_OK)
                {
                    ALOGD ("MwIf> Protocol = ");
                    phMwIfi_PrintNfaProtocolCode (px_data->ndef_detect.protocol);
                    ALOGD ("MwIf>max size = %u\n",
                            (unsigned int)px_data->ndef_detect.max_size);
                    ALOGD ("MwIf>cur_size = %u\n",
                            (unsigned int)px_data->ndef_detect.cur_size);
                    ALOGD ("MwIf>flags = 0x%02x\n", px_data->ndef_detect.flags);
                    gb_device_ndefcompliant = TRUE;
                    /* copy the NDEF data */
                    memcpy(&gx_ndef_detected,&(px_data->ndef_detect),
                            sizeof(tNFA_NDEF_DETECT));
                    mwIfHdl->sNdefDetectParams.dwStatus          = px_data->ndef_detect.status;
                    mwIfHdl->sNdefDetectParams.eProtocolType     = px_data->ndef_detect.protocol;
                    mwIfHdl->sNdefDetectParams.dwMaxSize         = px_data->ndef_detect.max_size;
                    mwIfHdl->sNdefDetectParams.dwCurSize         = px_data->ndef_detect.cur_size;
                    mwIfHdl->sNdefDetectParams.bNdefTypeFlags    = px_data->ndef_detect.flags;

                }
            }
            break;
            case NFA_DATA_EVT:
            {
                gx_status = NFA_STATUS_FAILED;  /* < Default Status */
                if(px_data->data.p_data == NULL)
                {
                    ALOGD("MwIf>Error Data pointer invalid !! \n");
                    gx_status = NFA_STATUS_BAD_RESP;
                    bCallbackReqd = FALSE;
                    bPushToQReqd  = FALSE;
                }
                else
                {
                    /* Check for Size invalidation before copy */
                    if(px_data->data.len >= 400)
                    {
                        ALOGD("MwIf> Error Data too big %d > 400 !! \n",
                        px_data->data.len);
                        gx_status = NFA_STATUS_BAD_LENGTH;
                        bCallbackReqd = FALSE;
                        bPushToQReqd  = FALSE;
                    }
                    else
                    {
                        memcpy(Pgs_cbTempBuffer+Pgui_cbTempBufferLength,px_data->data.p_data,px_data->data.len);
                        Pgui_cbTempBufferLength += px_data->data.len;
                        gx_status = NFA_STATUS_OK;
/*DATA chaining needs to be handled by Applicaion from L-release onwards
 * In KK release and before, it was handled in middleware*/
#ifdef APP_HANDLE_DATA_CHAINING
                        if(px_data->status == NFA_STATUS_CONTINUE)
                        {/*If its a chained Data dont publish the event. Wait for the last Data event*/
                            ALOGD("MwIf>Chained Data of Size=%d, Wait for last Data event before publishing\n",
                                    px_data->data.len);
                            bCallbackReqd = FALSE;
                            bPushToQReqd  = FALSE;
                        }
                        else
#endif
                        {
                            ALOGD("MwIf>Complete Data block received");
                            phMwIfi_PrintBuffer(Pgs_cbTempBuffer,Pgui_cbTempBufferLength,"MwIf>Data =");
                            if(gx_device.activate_ntf.protocol == NFC_PROTOCOL_NFC_DEP)
                            {
                                bCallbackReqd = FALSE;
                                bPushToQReqd = TRUE;
                            }
                        }
                    }
                }

            }
            break;
            case NFA_SELECT_CPLT_EVT:
                if(px_data->status == NFA_STATUS_OK)
                {
                    ALOGD("MwIf> Sector Selected Successfully...\n");
                }
            break;
            case NFA_READ_CPLT_EVT:
            break;
            case NFA_WRITE_CPLT_EVT:
            break;
            case NFA_LLCP_ACTIVATED_EVT:
            {
                phMWIf_eLlcpEvtType_t  eLlcpEvtType;
                phMwIf_uLlcpEvtInfo_t  sLlcpEvtInfo;
                bCallbackReqd = FALSE;
                ALOGD ("MwIf>Calling LLCP ACTIVATED EVENT");
                eLlcpEvtType = PHMWIF_LLCP_ACTIVATED_EVT;
                 if(mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb != NULL)
                  {
                      ALOGD ("MwIf>Calling LLCP Cback:MwIfEvt=0x%x,MwEvt=0x%x",eLlcpEvtType,uevent);
                      mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb(mwIfHdl,
                                                                    mwIfHdl->sLlcpPrms.sLlcpInitPrms.pvApplHdl,
                                                                    eLlcpEvtType,
                                                                    &sLlcpEvtInfo);
                  }
                  else
                  {
                      ALOGE("MwIf>App Callback not registered");
                  }
            }
            break;
            case NFA_LLCP_DEACTIVATED_EVT:
            {
                phMWIf_eLlcpEvtType_t  eLlcpEvtType;
                phMwIf_uLlcpEvtInfo_t  sLlcpEvtInfo;
                bCallbackReqd = FALSE;
                ALOGD ("MwIf>Calling LLCP DEACTIVATED EVENT");
                eLlcpEvtType = PHMWIF_LLCP_DEACTIVATED_EVT;
                 if(mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb != NULL)
                  {
                      ALOGD ("MwIf>Calling LLCP Cback:MwIfEvt=0x%x,MwEvt=0x%x",eLlcpEvtType,uevent);
                      mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb(mwIfHdl,
                                                                    mwIfHdl->sLlcpPrms.sLlcpInitPrms.pvApplHdl,
                                                                    eLlcpEvtType,
                                                                    &sLlcpEvtInfo);
                  }
                  else
                  {
                      ALOGE("MwIf>App Callback not registered");
                  }
            }
            break;
            case NFA_PRESENCE_CHECK_EVT :
            break;
            case NFA_FORMAT_CPLT_EVT :
            break;
            case NFA_I93_CMD_CPLT_EVT:
            break;
            case NFA_SET_TAG_RO_EVT:
            break;
            case NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT:
            break;
            case NFA_EXCLUSIVE_RF_CONTROL_STOPPED_EVT:
            break;
            case NFA_CE_REGISTERED_EVT:
            {
                NfaAidHandle = px_data->ce_registered.handle;
                ALOGE("MwIf> HCE AID Handle = 0x%x !!\n",NfaAidHandle);
                if(!NfaAidHandle)
                {
                    ALOGE("MwIf> Error registering AID Handle on DH !!\n");
                }
                else
                {
                    ALOGD("MwIf> Successfully registered AID Handle on DH\n");
                }
            }
            break;
            case NFA_CE_DEREGISTERED_EVT:
            {
                NfaAidHandle = px_data->ce_deregistered.handle;
                if(NfaAidHandle)
                {
                    ALOGD("MwIf> Successfully Deregistered AID Handle on DH\n");
                }
                else
                {
                    ALOGE("MwIf> Error Deregistering AID Handle on DH !!\n");
                }
            }
            break;
            case NFA_CE_DATA_EVT:
            {
                int i=0;
                Pgui_cbTempBufferLength = px_data->ce_data.len;
                memcpy(Pgs_cbTempBuffer,px_data->ce_data.p_data,Pgui_cbTempBufferLength);
                phMwIfi_PrintBuffer(Pgs_cbTempBuffer,Pgui_cbTempBufferLength,"MwIf> CE DATA Received = ");
                bCallbackReqd = TRUE;
                eMwIfEvtType = PHMWIF_CE_DATA_EVT;
                uEvtInfo.sData.dwSize  = px_data->ce_data.len;
                uEvtInfo.sData.pvDataBuf = (void*)Pgs_cbTempBuffer;
            }
            break;
            case NFA_CE_ACTIVATED_EVT :
                /*gb_device_connected = TRUE;*/
                mwIfHdl->eDeviceState = DEVICE_IN_POLL_ACTIVE_STATE;
                ALOGD ("MwIf> *******CE ACTIVATION DATA *******\n");
                phMwIfi_PrintNfaActivationParams(&px_data->ce_activated.activate_ntf);
                phMwIfi_CopyActivationPrms(&uEvtInfo.sActivationPrms,&px_data->ce_activated.activate_ntf);
                memcpy(&gx_se_device,&(px_data->ce_activated),sizeof(tNFA_CE_ACTIVATED));
                bCallbackReqd = TRUE;
                eMwIfEvtType = PHMWIF_CE_ACTIVATED_EVT;
                gx_status = NFC_STATUS_OK;
            break;
            case NFA_CE_DEACTIVATED_EVT:
                /*gb_device_connected = FALSE;*/

                ALOGD ("MwIf> NFA_CE_DEACTIVATED_EVT px_data->ce_deactivated.type = 0x%x",px_data->ce_deactivated.type);
                if(px_data->ce_deactivated.type == NFA_DEACTIVATE_TYPE_IDLE)
                {
                    mwIfHdl->eDeviceState = DEVICE_IN_IDLE_STATE;
                    ALOGD ("MwIf> NFA CE DeActivation type is IDLE");
                }
                else if(px_data->ce_deactivated.type == NFA_DEACTIVATE_TYPE_DISCOVERY)
                {
                    mwIfHdl->eDeviceState = DEVICE_IN_DISCOVERY_STARTED_STATE;
                    ALOGD ("MwIf> NFA CE DeActivation type is DISCOVERY");
                }
                else if(px_data->ce_deactivated.type == NFA_DEACTIVATE_TYPE_SLEEP)
                {
                    ALOGD ("MwIf> NFA CE DeActivation type is SLEEP");
                }
                bCallbackReqd = TRUE;
                eMwIfEvtType = PHMWIF_DEACTIVATED_EVT;
                uEvtInfo.eDeactivateType = px_data->ce_deactivated.type;
            break;
            case NFA_CE_LOCAL_TAG_CONFIGURED_EVT:
            break;
            case NFA_CE_NDEF_WRITE_START_EVT:
            break;
            case NFA_CE_NDEF_WRITE_CPLT_EVT:
            break;
            case NFA_CE_UICC_LISTEN_CONFIGURED_EVT:
            break;
            case NFA_RF_DISCOVERY_STARTED_EVT:
            break;
            case NFA_RF_DISCOVERY_STOPPED_EVT :
            break;
            case NFA_UPDATE_RF_PARAM_RESULT_EVT :
            break;
            case NFA_SET_P2P_LISTEN_TECH_EVT :
            break;
            case NFA_RW_INTF_ERROR_EVT :
                ALOGE("MwIf>EVENT NFA_RW_INTF_ERROR_EVT !! \n");
                bPushToQReqd = TRUE;
            break;
            default:
                ALOGE("MwIf>EVENT code not defined !! \n");
                gx_status = NFA_STATUS_BAD_RESP;
            break;
        }
    }
    else
    {
        ALOGE("MwIf>NO DATA !! \n");
    }

    if(bPushToQReqd)
    {
        psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
        memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
        psQueueData->dwEvtType = uevent;
        if(px_data)
            psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*px_data;
        ALOGE("MwIf>Pushing event %d to Queue",uevent);
        if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
        {
            ALOGE("DTA_HCIREGCB> Error Could not Post Semaphore !!\n");
            gx_status = NFA_STATUS_FAILED;
        }
    }
    else
    {
        ALOGE("Mwif>Skipping Event 0x%x not pushed to queue\n",uevent);
    }

    /*Update the users of Mwif about event received using callback*/
    if(bCallbackReqd)
    {
        if(mwIfHdl->mwIfCb != NULL)
        {
            ALOGD ("MwIf>Calling appCback:MwIfEvt=0x%x,MwEvt=0x%x",eMwIfEvtType,uevent);
            mwIfHdl->mwIfCb(mwIfHdl,
            mwIfHdl->appCbHdl,
            eMwIfEvtType,
            &uEvtInfo);
        }
        else
        {
            ALOGE("MwIf>App Callback not registered");
        }
    }
    ALOGD ("MwIf>%s:Exit",__FUNCTION__);
}

/**
 * \brief Function to Print the Buffer with a specific Message prompt
 * \param pubuf - 8bit Buffer to be printed
 * \param uilen - length of the Buffer to be printed
 * \param psmessage - Message to be printed before printing the Buffer
 */
void phMwIfi_PrintBuffer(uint8_t *pubuf,uint16_t uilen,const char *pcsmessage)
{
    uint16_t i;
    ALOGD("%s",pcsmessage);
    for(i = 0;i<uilen;i++)
    {
        ALOGD("0x%02X ",pubuf[i]);
    }
    ALOGD(" LEN=%d \n",uilen);
}

/**
 * \brief Function to Print the Buffer with a specific Message prompt
 * \param puibuf - 16bit Buffer to be printed
 * \param uilen - length of the Buffer to be printed
 * \param psmessage - Message to be printed before printing the Buffer
 */
void phMwIfi_PrintBuffer16(uint16_t *puibuf,uint16_t uilen,const char *pcsmessage)
{
    uint16_t i;
    ALOGD("%s",pcsmessage);
    for(i = 0;i<uilen;i++)
    {
        ALOGD("0x%04X ",puibuf[i]);
    }
    ALOGD(" LEN=%d \n",uilen);
}

void phMwIfi_PrintSNEPEventCode (tNFA_SNEP_EVT xevent)
{
   switch (xevent)
   {
       case NFA_SNEP_REG_EVT:
           ALOGD ("NFA_SNEP_REG_EVT\n");
       break;
       case NFA_SNEP_ACTIVATED_EVT:
           ALOGD ("NFA_SNEP_ACTIVATED_EVT\n");
       break;
       case NFA_SNEP_DEACTIVATED_EVT:
           ALOGD ("NFA_SNEP_DEACTIVATED_EVT\n");
       break;
       case NFA_SNEP_CONNECTED_EVT:
           ALOGD ("NFA_SNEP_CONNECTED_EVT\n");
       break;
       case NFA_SNEP_GET_REQ_EVT:
           ALOGD ("NFA_SNEP_GET_REQ_EVT\n");
       break;
       case NFA_SNEP_PUT_REQ_EVT:
           ALOGD ("NFA_SNEP_PUT_REQ_EVT\n");
       break;
       case NFA_SNEP_GET_RESP_EVT:
           ALOGD ("NFA_SNEP_GET_RESP_EVT\n");
       break;
       case NFA_SNEP_PUT_RESP_EVT:
           ALOGD ("NFA_SNEP_PUT_RESP_EVT\n");
       break;
       case NFA_SNEP_DISC_EVT:
           ALOGD ("NFA_SNEP_DISC_EVT\n");
       break;
       case NFA_SNEP_ALLOC_BUFF_EVT:
           ALOGD ("NFA_SNEP_ALLOC_BUFF_EVT\n");
       break;
       case NFA_SNEP_GET_RESP_CMPL_EVT:
           ALOGD ("NFA_SNEP_GET_RESP_CMPL_EVT\n");
       break;
       case NFA_SNEP_DEFAULT_SERVER_STARTED_EVT:
           ALOGD ("NFA_SNEP_DEFAULT_SERVER_STARTED_EVT\n");
       break;
       case NFA_SNEP_DEFAULT_SERVER_STOPPED_EVT:
           ALOGD ("NFA_SNEP_DEFAULT_SERVER_STOPPED_EVT\n");
       break;
       default:
           ALOGD ("Unknown event\n");
       break;
    }
}

void phMwIfi_PrintNfaEeEventCode (tNFA_EE_EVT xevent)
{
    switch (xevent)
    {
        case NFA_EE_DISCOVER_EVT:
            ALOGD ("NFA_EE_DISCOVER_EVT\n");
        break;
        case NFA_EE_REGISTER_EVT:
            ALOGD ("NFA_EE_REGISTER_EVT\n");
        break;
        case NFA_EE_DEREGISTER_EVT:
            ALOGD ("NFA_EE_DEREGISTER_EVT\n");
        break;
        case NFA_EE_MODE_SET_EVT:
            ALOGD ("NFA_EE_MODE_SET_EVT\n");
        break;
        case NFA_EE_ADD_AID_EVT:
            ALOGD ("NFA_EE_ADD_AID_EVT\n");
        break;
        case NFA_EE_REMOVE_AID_EVT:
            ALOGD ("NFA_EE_REMOVE_AID_EVT\n");
        break;
        case NFA_EE_SET_TECH_CFG_EVT:
            ALOGD ("NFA_EE_SET_TECH_CFG_EVT\n");
        break;
        case NFA_EE_SET_PROTO_CFG_EVT:
            ALOGD ("NFA_EE_SET_PROTO_CFG_EVT\n");
        break;
        case NFA_EE_CONNECT_EVT:
            ALOGD ("NFA_EE_CONNECT_EVT\n");
        break;
        case NFA_EE_DATA_EVT:
            ALOGD ("NFA_EE_DATA_EVT\n");
        break;
        case NFA_EE_DISCONNECT_EVT:
            ALOGD ("NFA_EE_DISCONNECT_EVT\n");
        break;
        case NFA_EE_NEW_EE_EVT:
            ALOGD ("NFA_EE_NEW_EE_EVT\n");
        break;
        case NFA_EE_ACTION_EVT:
            ALOGD ("NFA_EE_ACTION_EVT\n");
        break;
        case NFA_EE_DISCOVER_REQ_EVT:
            ALOGD ("NFA_EE_DISCOVER_REQ_EVT\n");
        break;
        case NFA_EE_ROUT_ERR_EVT:
            ALOGD ("NFA_EE_ROUT_ERR_EVT\n");
        break;
        case NFA_EE_NO_MEM_ERR_EVT:
            ALOGD ("NFA_EE_NO_MEM_ERR_EVT\n");
        break;
        case NFA_EE_NO_CB_ERR_EVT:
            ALOGD ("NFA_EE_NO_CB_ERR_EVT\n");
        break;
        default:
            ALOGD ("Unknown event\n");
        break;
    }
}

void phMwIfi_PrintNfaProtocolCode (tNFC_PROTOCOL xprotocol)
{
    switch (xprotocol)
    {
        case NFC_PROTOCOL_UNKNOWN:
            ALOGD ("NFC_PROTOCOL_UNKNOWN\n");
        break;
        case NFC_PROTOCOL_T1T:
            ALOGD ("NFC_PROTOCOL_T1T\n");
        break;
        case NFC_PROTOCOL_T2T:
            ALOGD ("NFC_PROTOCOL_T2T\n");
        break;
        case NFC_PROTOCOL_T3T:
            ALOGD ("NFC_PROTOCOL_T3T\n");
        break;
        case NFC_PROTOCOL_ISO_DEP:
            ALOGD ("NFC_PROTOCOL_ISO_DEP\n");
        break;
        case NFC_PROTOCOL_NFC_DEP:
            ALOGD ("NFC_PROTOCOL_NFC_DEP\n");
        break;
        case NFC_PROTOCOL_B_PRIME:
            ALOGD ("NFC_PROTOCOL_B_PRIME\n");
        break;
        case NFC_PROTOCOL_15693:
            ALOGD ("NFC_PROTOCOL_15693\n");
        break;
        case NFC_PROTOCOL_KOVIO:
            ALOGD ("NFC_PROTOCOL_KOVIO\n");
        break;
        default:
            ALOGD ("Unknown Protocol\n");
        break;
    }
}

void phMwIfi_PrintDiscoveryType (tNFC_DISCOVERY_TYPE xmode)
{
    switch (xmode)
    {
        case NFC_DISCOVERY_TYPE_POLL_A:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_A\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_B:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_B\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_F:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_F\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_A_ACTIVE:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_A_ACTIVE\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_F_ACTIVE:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_F_ACTIVE\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_ISO15693:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_ISO15693\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_B_PRIME:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_B_PRIME\n");
        break;
        case NFC_DISCOVERY_TYPE_POLL_KOVIO:
            ALOGD ("NFC_DISCOVERY_TYPE_POLL_KOVIO\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_A:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_A\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_B:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_B\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_F:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_F\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_ISO15693:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_ISO15693\n");
        break;
        case NFC_DISCOVERY_TYPE_LISTEN_B_PRIME:
            ALOGD ("NFC_DISCOVERY_TYPE_LISTEN_B_PRIME\n");
        break;
        default:
            ALOGD ("Unknown discovery type\n");
        break;
    }
}

void phMwIfi_PrintBitRate (tNFC_BIT_RATE xbitrate)
{
    switch (xbitrate)
    {
        case NFC_BIT_RATE_106:
            ALOGD ("NFC_BIT_RATE_106\n");
        break;
        case NFC_BIT_RATE_212:
            ALOGD ("NFC_BIT_RATE_212\n");
        break;
        case NFC_BIT_RATE_424:
            ALOGD ("NFC_BIT_RATE_424\n");
        break;
        case NFC_BIT_RATE_848:
            ALOGD ("NFC_BIT_RATE_848\n");
        break;
        case NFC_BIT_RATE_1696:
            ALOGD ("NFC_BIT_RATE_1696\n");
        break;
        case NFC_BIT_RATE_3392:
            ALOGD ("NFC_BIT_RATE_3392\n");
        break;
        case NFC_BIT_RATE_6784:
            ALOGD ("NFC_BIT_RATE_6784\n");
        break;
       default:
           ALOGD ("Unknown bit rate\n");
       break;
    }
}

void phMwIfi_PrintIntfType (tNFC_INTF_TYPE xtype)
{
    switch (xtype)
    {
        case NFC_INTERFACE_EE_DIRECT_RF:
            ALOGD ("NFC_INTERFACE_EE_DIRECT_RF\n");
        break;
        case NFC_INTERFACE_FRAME:
            ALOGD ("NFC_INTERFACE_FRAME\n");
        break;
        case NFC_INTERFACE_ISO_DEP:
            ALOGD ("NFC_INTERFACE_ISO_DEP\n");
        break;
/*        case NFC_INTERFACE_NDEF:
            ALOGD ("NFC_INTERFACE_NDEF\n");
          break;
*/
        case NFC_INTERFACE_NFC_DEP:
            ALOGD ("NFC_INTERFACE_NFC_DEP\n");
        break;
/*      case NFC_INTERFACE_LLCP_LOW:
            ALOGD ("NFC_INTERFACE_LLCP_LOW\n");
        break;

        case NFC_INTERFACE_LLCP_HIGH:
            ALOGD ("NFC_INTERFACE_LLCP_HIGH\n");
        break;

        case NFC_INTERFACE_VS_T2T_CE:
            ALOGD ("NFC_INTERFACE_VS_T2T_CE\n");
        break;
*/
        default:
            ALOGD ("Unknown interface type\n");
        break;
    }
}

void phMwIfi_PrintNfaStatusCode(tNFA_STATUS xstatus) {
    switch(xstatus)
    {
    case NFA_STATUS_OK:
       ALOGD("NFA_STATUS_OK\n");
       break;
    case NFA_STATUS_REJECTED:
       ALOGD("NFA_STATUS_REJECTED\n");
       break;
    case NFA_STATUS_MSG_CORRUPTED:
       ALOGD("NFA_STATUS_MSG_CORRUPTED\n");
       break;
    case NFA_STATUS_BUFFER_FULL:
       ALOGD("NFA_STATUS_BUFFER_FULL\n");
       break;
    case NFA_STATUS_FAILED:
       ALOGD("NFA_STATUS_FAILED\n");
       break;
    case NFA_STATUS_NOT_INITIALIZED:
       ALOGD("NFA_STATUS_NOT_INITIALIZED\n");
       break;
    case NFA_STATUS_SYNTAX_ERROR :
       ALOGD("NFA_STATUS_SYNTAX_ERROR\n");
       break;
    case NFA_STATUS_SEMANTIC_ERROR :
       ALOGD("NFA_STATUS_SEMANTIC_ERROR\n");
       break;
    case NFA_STATUS_UNKNOWN_GID:
        ALOGD("NFA_STATUS_UNKNOWN_GID\n");
        break;
    case NFA_STATUS_UNKNOWN_OID:
        ALOGD("NFA_STATUS_UNKNOWN_OID\n");
        break;
    case NFA_STATUS_INVALID_PARAM:
        ALOGD("NFA_STATUS_INVALID_PARAM\n");
        break;
    case NFA_STATUS_MSG_SIZE_TOO_BIG:
        ALOGD("NFA_STATUS_MSG_SIZE_TOO_BIG\n");
        break;
    case NFA_STATUS_ALREADY_STARTED:
        ALOGD("NFA_STATUS_ALREADY_STARTED\n");
        break;
    case NFA_STATUS_ACTIVATION_FAILED:
        ALOGD("NFA_STATUS_ACTIVATION_FAILED\n");
        break;
    case NFA_STATUS_TEAR_DOWN:
        ALOGD("NFA_STATUS_TEAR_DOWN\n");
        break;
    case NFA_STATUS_RF_TRANSMISSION_ERR:
        ALOGD("NFA_STATUS_RF_TRANSMISSION_ERR\n");
        break;
    case NFA_STATUS_RF_PROTOCOL_ERR:
        ALOGD("NFA_STATUS_RF_PROTOCOL_ERR\n");
        break;
    case NFA_STATUS_TIMEOUT:
        ALOGD("NFA_STATUS_TIMEOUT\n");
        break;
    case NFA_STATUS_EE_INTF_ACTIVE_FAIL:
        ALOGD("NFA_STATUS_EE_INTF_ACTIVE_FAIL\n");
        break;
    case NFA_STATUS_EE_TRANSMISSION_ERR:
        ALOGD("NFA_STATUS_EE_TRANSMISSION_ERR\n");
        break;
    case NFA_STATUS_EE_PROTOCOL_ERR:
        ALOGD("NFA_STATUS_EE_PROTOCOL_ERR\n");
        break;
    case NFA_STATUS_EE_TIMEOUT:
        ALOGD("NFA_STATUS_EE_TIMEOUT\n");
        break;
    case NFA_STATUS_CMD_STARTED:
        ALOGD("NFA_STATUS_CMD_STARTED\n");
        break;
    case NFA_STATUS_HW_TIMEOUT:
        ALOGD("NFA_STATUS_HW_TIMEOUTF\n");
        break;
    case NFA_STATUS_CONTINUE:
        ALOGD("NFA_STATUS_CONTINUE\n");
        break;
    case NFA_STATUS_REFUSED:
        ALOGD("NFA_STATUS_REFUSED\n");
        break;
    case NFA_STATUS_BAD_RESP:
        ALOGD("NFA_STATUS_BAD_RESP\n");
        break;
    case NFA_STATUS_CMD_NOT_CMPLTD:
        ALOGD("NFA_STATUS_CMD_NOT_CMPLTD\n");
        break;
    case NFA_STATUS_NO_BUFFERS:
        ALOGD("NFA_STATUS_NO_BUFFERS\n");
        break;
    case NFA_STATUS_WRONG_PROTOCOL:
        ALOGD("NFA_STATUS_WRONG_PROTOCOL\n");
        break;
    case NFA_STATUS_BUSY:
        ALOGD("NFA_STATUS_BUSY\n");
        break;
    case NFA_STATUS_BAD_LENGTH:
        ALOGD("NFA_STATUS_BAD_LENGTH\n");
        break;
    case NFA_STATUS_BAD_HANDLE:
        ALOGD("NFA_STATUS_BAD_HANDLE\n");
        break;
    case NFA_STATUS_CONGESTED:
        ALOGD("NFA_STATUS_CONGESTED\n");
        break;
    default:
       ALOGD("EVENT code not defined\n");
       break;
    }
}

/**
*Display the activation parameters received
*/
void phMwIfi_PrintNfaActivationParams(tNFC_ACTIVATE_DEVT* psActivationNtfPrms)
{
    ALOGD ("MwIf> *****ACTIVATION DATA*****\n");
    ALOGD ("MwIf>p_data->activated.activate_ntf.rf_disc_id = %x\n", psActivationNtfPrms->rf_disc_id);
    ALOGD("Protocol Type - ");
    phMwIfi_PrintNfaProtocolCode (psActivationNtfPrms->protocol);
    ALOGD("Discovery Mode & Technology - ");
    phMwIfi_PrintDiscoveryType (psActivationNtfPrms->rf_tech_param.mode);
    ALOGD("TX Bitrate - ");
    phMwIfi_PrintBitRate (psActivationNtfPrms->tx_bitrate);
    ALOGD("RX Bitrate - ");
    phMwIfi_PrintBitRate (psActivationNtfPrms->rx_bitrate);
    ALOGD("Remote Interface - ");
    phMwIfi_PrintIntfType (psActivationNtfPrms->intf_param.type);
}

/** Display NFA_DM event code */
void phMwIfi_PrintDMEventCode(UINT8 uevent)
{
   switch(uevent)
   {
   case NFA_DM_ENABLE_EVT:
      ALOGD("NFA_DM_ENABLE_EVT\n"); /* Result of NFA_Enable */
      break;
   case NFA_DM_DISABLE_EVT:
      ALOGD("NFA_DM_DISABLE_EVT\n"); /* Result of NFA_Disable */
      break;
   case NFA_DM_SET_CONFIG_EVT:
      ALOGD("NFA_DM_SET_CONFIG_EVT\n"); /* Result of NFA_SetConfig */
      break;
   case NFA_DM_GET_CONFIG_EVT:
      ALOGD("NFA_DM_GET_CONFIG_EVT\n"); /* Result of NFA_GetConfig */
      break;
   case NFA_DM_PWR_MODE_CHANGE_EVT:
      ALOGD("NFA_DM_PWR_MODE_CHANGE_EVT\n"); /* Result of NFA_PowerOffSleepMode */
      break;
   case NFA_DM_RF_FIELD_EVT:
      ALOGD("NFA_DM_RF_FIELD_EVT\n"); /* Status of RF Field */
      break;
   case NFA_DM_NFCC_TIMEOUT_EVT :
      ALOGD("NFA_DM_NFCC_TIMEOUT_EVT\n"); /* NFCC is not responding */
      break;
   case NFA_DM_NFCC_TRANSPORT_ERR_EVT :
      ALOGD("NFA_DM_NFCC_TRANSPORT_ERR_EVT\n"); /* NCI Tranport error */
      break;
   default:
      ALOGD("EVENT code not defined\n");
      break;
   }
}

/** Display NFA_CN event code */
void phMwIfi_PrintCNEventCode(UINT8 uevent)
{
   switch(uevent)
   {
   case NFA_POLL_ENABLED_EVT:
      ALOGD("NFA_POLL_ENABLED_EVT\n");
      break;
   case NFA_POLL_DISABLED_EVT:
      ALOGD("NFA_POLL_DISABLED_EVT\n");
      break;
   case NFA_LISTEN_ENABLED_EVT:
      ALOGD("NFA_LISTEN_ENABLED_EVT\n");
   break;
   case NFA_LISTEN_DISABLED_EVT:
      ALOGD("NFA_LISTEN_DISABLED_EVT\n");
   break;
   case NFA_DISC_RESULT_EVT:
      ALOGD("NFA_DISC_RESULT_EVT\n");
      break;
   case NFA_SELECT_RESULT_EVT:
      ALOGD("NFA_SELECT_RESULT_EVT\n");
      break;
   case NFA_DEACTIVATE_FAIL_EVT:
      ALOGD("NFA_DEACTIVATE_FAIL_EVT\n");
      break;
   case NFA_ACTIVATED_EVT:
      ALOGD("NFA_ACTIVATED_EVT\n");
      break;
   case NFA_DEACTIVATED_EVT:
       ALOGD("NFA_DEACTIVATED_EVT\n");
      break;
   case NFA_TLV_DETECT_EVT :
      ALOGD("NFA_TLV_DETECT_EVT\n");
      break;
   case NFA_NDEF_DETECT_EVT :
      ALOGD("NFA_NDEF_DETECT_EVT\n");
      break;
   case NFA_DATA_EVT:
      ALOGD("NFA_DATA_EVT\n");
      break;
   case NFA_SELECT_CPLT_EVT:
      ALOGD("NFA_SELECT_CPLT_EVT\n");
      break;
   case NFA_READ_CPLT_EVT:
      ALOGD("NFA_READ_CPLT_EVT\n");
      break;
   case NFA_WRITE_CPLT_EVT:
      ALOGD("NFA_WRITE_CPLT_EVT\n");
      break;
   case NFA_LLCP_ACTIVATED_EVT:
      ALOGD("NFA_LLCP_ACTIVATED_EVT\n");
      break;
   case NFA_LLCP_DEACTIVATED_EVT:
      ALOGD("NFA_LLCP_DEACTIVATED_EVT\n");
      break;
   case NFA_PRESENCE_CHECK_EVT :
      ALOGD("NFA_PRESENCE_CHECK_EVT\n");
      break;
   case NFA_FORMAT_CPLT_EVT :
      ALOGD("NFA_FORMAT_CPLT_EVT\n");
      break;
   case NFA_I93_CMD_CPLT_EVT:
      ALOGD("NFA_I93_CMD_CPLT_EVT\n");
      break;
   case NFA_SET_TAG_RO_EVT:
      ALOGD("NFA_SET_TAG_RO_EVT\n");
      break;
   case NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT:
      ALOGD("NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT\n");
      break;
   case NFA_EXCLUSIVE_RF_CONTROL_STOPPED_EVT:
      ALOGD("NFA_EXCLUSIVE_RF_CONTROL_STOPPED_EVT\n");
      break;
   case NFA_CE_REGISTERED_EVT:
      ALOGD("NFA_CE_REGISTERED_EVT\n");
      break;
   case NFA_CE_DEREGISTERED_EVT:
      ALOGD("NFA_CE_DEREGISTERED_EVT\n");
      break;
   case NFA_CE_DATA_EVT :
      ALOGD("NFA_CE_DATA_EVT\n");
      break;
   case NFA_CE_ACTIVATED_EVT :
      ALOGD("NFA_CE_ACTIVATED_EVT\n");
      break;
   case NFA_CE_DEACTIVATED_EVT:
      ALOGD("NFA_CE_DEACTIVATED_EVT\n");
      break;
   case NFA_CE_LOCAL_TAG_CONFIGURED_EVT:
      ALOGD("NFA_CE_LOCAL_TAG_CONFIGURED_EVT\n");
      break;
   case NFA_CE_NDEF_WRITE_START_EVT:
      ALOGD("NFA_CE_NDEF_WRITE_START_EVT\n");
      break;
   case NFA_CE_NDEF_WRITE_CPLT_EVT:
      ALOGD("NFA_CE_NDEF_WRITE_CPLT_EVT\n");
      break;
   case NFA_CE_UICC_LISTEN_CONFIGURED_EVT:
      ALOGD("NFA_CE_UICC_LISTEN_CONFIGURED_EVT\n");
      break;
   case NFA_RF_DISCOVERY_STARTED_EVT:
      ALOGD("NFA_RF_DISCOVERY_STARTED_EVT\n");
      break;
   case NFA_RF_DISCOVERY_STOPPED_EVT :
      ALOGD("NFA_RF_DISCOVERY_STOPPED_EVT\n");
      break;
   case NFA_UPDATE_RF_PARAM_RESULT_EVT :
      ALOGD("NFA_UPDATE_RF_PARAM_RESULT_EVT\n");
      break;
   case NFA_SET_P2P_LISTEN_TECH_EVT :
      ALOGD("NFA_SET_P2P_LISTEN_TECH_EVT\n");
      break;
   case NFA_RW_INTF_ERROR_EVT :
      ALOGD("NFA_RW_INTF_ERROR_EVT\n");
      break;
   default:
      ALOGD("EVENT code not defined\n");
      break;
   }
}

void phMwIfi_PrintNDEFEventCode(tNFA_NDEF_EVT xevent)
{
    switch (xevent)
    {
        case NFA_NDEF_REGISTER_EVT:
            ALOGD ("NFA_NDEF_REGISTER_EVT\n");
            break;

        case NFA_NDEF_DATA_EVT:
            ALOGD ("NFA_NDEF_DATA_EVT\n");
            break;

        default:
            ALOGD ("Unknown event type\n");
            break;
    }
}

void phMwIfi_PrintNfaHciEventCode(tNFA_HCI_EVT xevent)
{
    switch(xevent)
    {
        case NFA_HCI_REGISTER_EVT:
            ALOGD ("NFA_HCI_REGISTER_EVT\n");
        break;
        case NFA_HCI_DEREGISTER_EVT:
            ALOGD ("NFA_HCI_DEREGISTER_EVT\n");
        break;
        case NFA_HCI_GET_GATE_PIPE_LIST_EVT:
            ALOGD ("NFA_HCI_GET_GATE_PIPE_LIST_EVT\n");
        break;
        case NFA_HCI_ALLOCATE_GATE_EVT:
            ALOGD ("NFA_HCI_ALLOCATE_GATE_EVT\n");
        break;
        case NFA_HCI_DEALLOCATE_GATE_EVT:
            ALOGD ("NFA_HCI_DEALLOCATE_GATE_EVT\n");
        break;
        case NFA_HCI_CREATE_PIPE_EVT:
            ALOGD ("NFA_HCI_CREATE_PIPE_EVT\n");
        break;
        case NFA_HCI_OPEN_PIPE_EVT:
            ALOGD ("NFA_HCI_OPEN_PIPE_EVT\n");
        break;
        case NFA_HCI_CLOSE_PIPE_EVT:
            ALOGD ("NFA_HCI_CLOSE_PIPE_EVT\n");
        break;
        case NFA_HCI_DELETE_PIPE_EVT:
            ALOGD ("NFA_HCI_DELETE_PIPE_EVT\n");
        break;
        case NFA_HCI_HOST_LIST_EVT:
            ALOGD ("NFA_HCI_HOST_LIST_EVT\n");
        break;
        case NFA_HCI_INIT_EVT:
            ALOGD ("NFA_HCI_INIT_EVT\n");
        break;
        case NFA_HCI_EXIT_EVT:
            ALOGD ("NFA_HCI_EXIT_EVT\n");
        break;
        case NFA_HCI_RSP_RCVD_EVT:
            ALOGD ("NFA_HCI_RSP_RCVD_EVT\n");
        break;
        case NFA_HCI_RSP_SENT_EVT:
            ALOGD ("NFA_HCI_RSP_SENT_EVT\n");
        break;
        case NFA_HCI_CMD_SENT_EVT:
            ALOGD ("NFA_HCI_CMD_SENT_EVT\n");
        break;
        case NFA_HCI_EVENT_SENT_EVT:
            ALOGD ("NFA_HCI_EVENT_SENT_EVT\n");
        break;
        case NFA_HCI_CMD_RCVD_EVT:
            ALOGD ("NFA_HCI_CMD_RCVD_EVT\n");
        break;
        case NFA_HCI_EVENT_RCVD_EVT:
            ALOGD ("NFA_HCI_EVENT_RCVD_EVT\n");
        break;
        case NFA_HCI_GET_REG_CMD_EVT:
            ALOGD ("NFA_HCI_GET_REG_CMD_EVT\n");
        break;
        case NFA_HCI_SET_REG_CMD_EVT:
            ALOGD ("NFA_HCI_SET_REG_CMD_EVT\n");
        break;
        case NFA_HCI_GET_REG_RSP_EVT:
            ALOGD ("NFA_HCI_GET_REG_RSP_EVT\n");
        break;
        case NFA_HCI_SET_REG_RSP_EVT:
            ALOGD ("NFA_HCI_SET_REG_RSP_EVT\n");
        break;
        case NFA_HCI_ADD_STATIC_PIPE_EVT:
            ALOGD ("NFA_HCI_ADD_STATIC_PIPE_EVT\n");
        break;
        default:
            ALOGD ("Unknown event\n");
        break;
    }
}

void phMwIfi_NfaEeCallback(tNFA_EE_EVT xevent,tNFA_EE_CBACK_DATA *px_data)
{
    ALOGE("**** EeRegister_Cback ****\n");
    ALOGE("DTA_EEREGCB> Event = 0x%02x - ", xevent);
    phMwIf_sQueueData_t* psQueueData = NULL;
    phMwIf_sHandle_t *   mwIfHdl = &g_mwIfHandle;
    phMwIfi_PrintNfaEeEventCode (xevent);
    tNFA_EE_DISCOVER_REQ info = px_data->discover_req;
    if(px_data != NULL)
    {
        ALOGE("DTA_EEREGCB> Callback Data Status = ");
        phMwIfi_PrintNfaStatusCode(px_data->status);
        gx_status = px_data->status;
        switch (xevent)
        {
            case NFA_EE_REGISTER_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Register Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->status);
            }
            break;
            case NFA_EE_DEREGISTER_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Deregister Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->status);
            }
            break;
            case NFA_EE_MODE_SET_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Mode Set Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->mode_set.status);
            }
            break;
            case NFA_EE_SET_TECH_CFG_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Set Tech Route Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->set_tech);
            }
            break;
            case NFA_EE_SET_PROTO_CFG_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Set Tech Route Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->set_proto);
            }
            break;
            case NFA_EE_ACTION_EVT:
            {
                ALOGE("DTA_EEREGCB> EE Action Event = ");
                tNFA_EE_ACTION action = px_data->action;
                if (action.trigger == NFC_EE_TRIG_RF_PROTOCOL)
                    ALOGE("RF Protocol Triggered\n");
                else if (action.trigger == NFC_EE_TRIG_RF_TECHNOLOGY)
                    ALOGE("RF Technology Triggered\n");
                else
                    ALOGE("Unknown Technology/Protocol Triggered\n");
            }
            break;
            case NFA_EE_DISCOVER_REQ_EVT:
            {
                /*Set the configuration for UICC/ESE */
                ALOGE("DTA_EEREGCB> No.of Secure Elements Discovered = %d \n", info.num_ee);
                if(info.num_ee)
                    ALOGE("DTA_EEREGCB> UICC/ESE Info Stored Successfully\n");
                else
                    ALOGE("DTA_EEREGCB> Check UICC...\n");
            }
            break;
            case NFA_EE_NO_CB_ERR_EVT:
                ALOGE("DTA_EEREGCB> NFA_EE_NO_CB_ERR_EVT \n");
            break;
            case NFA_EE_ADD_AID_EVT:
            {
                ALOGE("DTA_EEREGCB> NFA_EE_ADD_AID_EVT \n");
            }
            break;
            case NFA_EE_REMOVE_AID_EVT:
            {
                ALOGE("DTA_EEREGCB> NFA_EE_REMOVE_AID_EVT \n");
            }
            break;
            case NFA_EE_NEW_EE_EVT:
            {
                ALOGE("DTA_EEREGCB> NFA_EE_NEW_EE_EVT \n");
            }
            break;
            default:
                ALOGE("Default Event");
            break;
        }

        psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
        memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
        psQueueData->dwEvtType = NFA_EE_EVT_OFFSET + xevent;
        if(px_data)
            psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*px_data;
        if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
        {
            ALOGE("DTA_HCIREGCB> Error Could not Post Semaphore !!\n");
            gx_status = NFA_STATUS_FAILED;
        }
    }
}

tNFA_STATUS phMwIfi_EeGetInfo(phMwIf_sHandle_t *mwIfHdl,
                              phMwIf_eCeDevType_t eDevType)
{
    gx_status = NFA_STATUS_FAILED;
    uint8_t numNfcEE = 5; /*Default*/
    uint8_t numNfcEEPresent = 0;
    uint8_t i = 0;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    /*Get Number of EE's from middleware and their related information*/
    gx_status = NFA_EeGetInfo(&numNfcEE, mwIfHdl->infoNfcEE);
    phMwIfi_PrintNfaStatusCode(gx_status);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Get Info !!\n");
    ALOGD("Total Number of EE = %d\n",numNfcEE);
    if(numNfcEE == 0)
    {
        ALOGE("NO SE found\n");
        return NFA_STATUS_FAILED;
    }
    mwIfHdl->numNfcEE = numNfcEE;

    /*Check if the EE present are valid*/
    for (i=0; i<numNfcEE; i++)
    {
        if ((mwIfHdl->infoNfcEE[i].num_interface != 0) && (mwIfHdl->infoNfcEE[i].ee_interface[0] != NCI_NFCEE_INTERFACE_HCI_ACCESS))
        {
            numNfcEEPresent++;
        }
    }
    ALOGE("numNfcEEPresent = %d\n",numNfcEEPresent);
    if (numNfcEEPresent == 0)
    {
        ALOGE("NO Valid SE found\n");
        return NFA_STATUS_FAILED;
    }

    /*Make sure EE is active*/
    for(i=0; i<=numNfcEEPresent; i++)
    {
        if ((mwIfHdl->infoNfcEE[i].ee_handle == NfaUiccHandle) && (eDevType == PHMWIF_UICC_CE))
        {
            if(mwIfHdl->infoNfcEE[i].ee_status == 0x00)
            {/*EE Connected and Enabled*/
                gx_status = NFA_STATUS_OK;
                break;
            }
            else if(mwIfHdl->infoNfcEE[i].ee_status == 0x01)
            {/*Connected but Disabled...So activate it*/
                ALOGD("MwIf>NFA_EeModeSet \n");
                gx_status = NFA_EeModeSet(NfaUiccHandle, NFC_MODE_ACTIVATE);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Mode Set !!\n");
                PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_MODE_SET_EVT, 5000,
                        "MwIf> ERROR NFA Stop EE Mode Set (SEM) !! \n",&(mwIfHdl->sLastQueueData));
                break;
            }
            else
            {
                gx_status = NFA_STATUS_FAILED;
                ALOGE("MwIf>Invalid UICC");
                break;
            }
        }
        else if((mwIfHdl->infoNfcEE[i].ee_handle == NfaEseHandle) && (eDevType == PHMWIF_SE_CE))
        {
            if(mwIfHdl->infoNfcEE[i].ee_status == 0x00)
            {/*EE Connected and Enabled*/
                gx_status = NFA_STATUS_OK;
                break;
            }
            else if(mwIfHdl->infoNfcEE[i].ee_status == 0x01)
            {/*Connected but Disabled...So activate it*/
                ALOGD("MwIf>NFA_EeModeSet \n");
                gx_status = NFA_EeModeSet(NfaEseHandle, NFC_MODE_ACTIVATE);
                PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR EE Mode Set !!\n");
                PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_EE_EVT_OFFSET+NFA_EE_MODE_SET_EVT, 5000,
                        "MwIf> ERROR NFA Stop EE Mode Set (SEM) !! \n",&(mwIfHdl->sLastQueueData));
                break;
            }
            else
            {
                gx_status = NFA_STATUS_FAILED;
                ALOGE("MwIf>Invalid SE");
                break;
            }
        }
        else
        {
            gx_status = NFA_STATUS_FAILED;
            ALOGE("MwIf>Invalid EE");
        }
    }

    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return gx_status;
}

tNFA_STATUS phMwIfi_HciRegister(phMwIf_sHandle_t *mwIfHdl)
{
    uint8_t i = 0;
    for (i=0; i<mwIfHdl->numNfcEE; i++)
    {
        if ((mwIfHdl->infoNfcEE[i].num_interface > 0) && (mwIfHdl->infoNfcEE[i].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS) )
        {
            ALOGD("MwIf>Found HCI network,Trying hci register. i=%d\n",i);
            gx_status = NFA_HciRegister(APP_NAME, phMwIfi_NfaHciCallback, TRUE);
            PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR HCI Register !!\n");
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_HCI_REGISTER_EVT,5000,
                    "MwIf> ERROR NFA Stop HCI Register (SEM) !! \n",&(mwIfHdl->sLastQueueData));
        }
    }
    return NFA_STATUS_OK;
}

void phMwIfi_NfaHciCallback(tNFA_HCI_EVT xevent, tNFA_HCI_EVT_DATA *px_data)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    ALOGD("MwIf>%s:enter:Event=0x%02x",__FUNCTION__,xevent);
    phMwIf_sQueueData_t* psQueueData = NULL;
    phMwIfi_PrintNfaHciEventCode (xevent);
    if(px_data != NULL)
    {
        ALOGD("MwIf>Callback Data Status = ");
        phMwIfi_PrintNfaStatusCode(px_data->status);
        gx_status = px_data->status;
        switch (xevent)
        {
            case NFA_HCI_REGISTER_EVT:
            {
                ALOGD("MwIf> HCI Registration Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->hci_register.status);
                NfaHciHandle = px_data->hci_register.hci_handle;
                ALOGD("MwIf> HCI Handle = %d\n", NfaHciHandle);
            }
            break;
            case NFA_HCI_DEREGISTER_EVT:
            {
                ALOGD("DTA_HCIREGCB> HCI De-Registration Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->hci_deregister.status);
            }
            break;
            case NFA_HCI_EXIT_EVT:
            {
                ALOGD("DTA_HCIREGCB> HCI Exit Event Status = ");
                phMwIfi_PrintNfaStatusCode(px_data->hci_exit.status);
            }
            break;
            default:
                ALOGD("HCI default Event");
            break;
        }

        psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
        memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
        psQueueData->dwEvtType = xevent;
        if(px_data)
            psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*px_data;
        if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
        {
            ALOGE("DTA_HCIREGCB> Error Could not Post Semaphore !!\n");
            gx_status = NFA_STATUS_FAILED;
        }
    }
    ALOGD("MwIf>%s:exit",__FUNCTION__);
}

MWIFSTATUS phMwIfi_HceConfigNciParams(phMwIf_sHandle_t* mwIfHandle)
{
    gs_paramBuffer[0] = 0x02;
    phMwIf_SetConfig(mwIfHandle,0x5C, 1, gs_paramBuffer);/*DID Support for HCE Type A*/
    gs_paramBuffer[0] = 0x01;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LB_SENSB_INFO, 1, gs_paramBuffer);
    gs_paramBuffer[0] = 0x01;
    gs_paramBuffer[1] = 0x02;
    gs_paramBuffer[2] = 0x03;
    gs_paramBuffer[3] = 0x04;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LB_NFCID0, 4, gs_paramBuffer);
    gs_paramBuffer[0] = 0x00;
    gs_paramBuffer[1] = 0x00;
    gs_paramBuffer[2] = 0x00;
    gs_paramBuffer[3] = 0x00;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LB_APPDATA, 4, gs_paramBuffer);
    gs_paramBuffer[0] = 0x08;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LB_SFGI, 1, gs_paramBuffer);
    gs_paramBuffer[0] = 0x01;/*DID Support for HCE Type B*/
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LB_ADC_FO, 1, gs_paramBuffer);
    gs_paramBuffer[0] = 0x00;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_LI_BIT_RATE, 1, gs_paramBuffer);
    gs_paramBuffer[0] = 0x08;
    phMwIf_SetConfig(mwIfHandle,NCI_PARAM_ID_FWI, 1, gs_paramBuffer);
    ALOGE("MwIf> TypeAB set configs done !!\n");
    return NFA_STATUS_OK;
}

MWIFSTATUS phMwIfi_CeRegisterAID(phMwIf_sHandle_t* mwIfHandle)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    gx_status = NFA_CeRegisterAidOnDH (gs_Uicc_Aid, gs_Uicc_Aid_len, phMwIfi_NfaConnCallback);
    PH_ON_ERROR_EXIT(NFA_STATUS_OK, 2,"MwIf> ERROR Registering AID on DH !!\n");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_CE_REGISTERED_EVT, 5000,
            "MwIf> ERROR NFA Stop Register AID on DH",&(mwIfHdl->sLastQueueData));

    return NFA_STATUS_OK;
}

MWIFSTATUS phMwIfi_CreateNdefMsg(phMwIf_sHandle_t *mwIfHdl,
                                 uint8_t* pData,
                                 uint32_t dwLength)
{
    uint32_t u32psize=0;
    /* Initialize the Message */
    NDEF_MsgInit((UINT8 *)gs_paramBuffer,(UINT32)400,
              (UINT32 *)&u32psize);
    if(mwIfHdl->sNdefDetectParams.eProtocolType == PHMWIF_PROTOCOL_T1T)
    {
        ALOGD("MwIf> Make an NDEF message \n");
        gx_status = NDEF_MsgAddRec((UINT8 *) gs_paramBuffer,
                                    (UINT32) 400, (UINT32 *) &u32psize,
                                    0x01, /* TNF NFC Forum External */
                                    (UINT8 *) "U",
                                    (UINT8) 1, /* Type */
                                    (UINT8 *) NULL,
                                    (UINT8) 0, /* ID */
                                    (UINT8 *) pData,
                                    (UINT32) dwLength ); /* Pay load */
    }
    else if(mwIfHdl->sNdefDetectParams.eProtocolType == PHMWIF_PROTOCOL_ISO_DEP)
    {
        gx_status = NDEF_MsgAddRec ((UINT8 *)gs_paramBuffer,
                        (UINT32)400,
                            (UINT32 *)&u32psize,
                        0x01, /* TNF NFC Forum External */
                        (UINT8 *)NULL,
                            (UINT8)0,/* Type */
                        (UINT8 *)NULL,
                            (UINT8)0, /* ID */
                        (UINT8 *)pData,
                            (UINT32)dwLength /* Pay load */
                        );

    }
    if (gx_status != NFA_STATUS_OK)
    {
        ALOGE("MwIf> Error Could not form the NDEF message !! \n");
        return MWIFSTATUS_FAILED;
    }
    /* Write the NDEF Message */
    gs_sizeParamBuffer = u32psize; /* Down size for u16 */
    return MWIFSTATUS_SUCCESS;
}

MWIFSTATUS phMwIf_Transceive(void* mwIfHandle,
                           void* pvInBuff,
                           uint32_t dwLenInBuff,
                           void* pvOutBuff,
                           uint32_t* dwLenOutBuff)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    OSALSTATUS dwOsalStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGE("MwIf>%s:Device Disconnected Already",__FUNCTION__);
        return NFA_STATUS_FAILED;
    }

    phMwIfi_PrintBuffer(pvInBuff,dwLenInBuff,"MwIf> TXVR Sending = ");
    if(dwLenInBuff > 400)
    {
      ALOGE("MwIf> Error Buffer length %d > 400 !! \n",dwLenInBuff);
      return MWIFSTATUS_INVALID_PARAM;
    }
    if(pvInBuff == NULL)
    {
      ALOGE("MwIf> Invalid Buffer Ptr");
      return MWIFSTATUS_INVALID_PARAM;
    }
    /*Remove previous events in queue as wait for new data event is required*/
/*
    ALOGD("MwIf>%s:Flushing!! All Data in Queue ",__FUNCTION__);
    do
    {
        dwOsalStatus = phOsal_QueuePull(mwIfHdl->pvQueueHdl,(void**)&pvEvtData,1);
        if(dwOsalStatus != OSALSTATUS_Q_UNDERFLOW)
        {
            ALOGD("MwIf>%s:Flushed! event recvd in Q=0x%x",__FUNCTION__,pvEvtData->dwEvtType);
            free(pvEvtData);
        }
    }while(dwOsalStatus != OSALSTATUS_Q_UNDERFLOW);
    */

    /* Send the Frame*/
    ALOGD("MwIf>%s:Sending Raw Buffer",__FUNCTION__);
    gx_status = NFA_SendRawFrame(pvInBuff,(UINT16)dwLenInBuff,NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,"MwIf> Error in performing Transceive(TX) !! \n");

    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGD("MwIf>%s:Device Disconnected",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }


    ALOGD("MwIf>%s:Waiting for Data",__FUNCTION__);
    /*PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DATA_EVT,20000,
            "MwIf> Error in Transceive(RX) (SEM) !! \n",&(mwIfHdl->sLastQueueData));
            */
    switch(bProtocol)
    {
       case NFC_PROTOCOL_ISO_DEP:
       {
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DATA_EVT,30000,
                    "MwIf> Error in Transceive(RX) (SEM) !! \n",&(mwIfHdl->sLastQueueData));
       }
       break;
       case NFC_PROTOCOL_NFC_DEP:
       {
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DATA_EVT,20000,
                    "MwIf> Error in Transceive(RX) (SEM) !! \n",&(mwIfHdl->sLastQueueData));
       }
       break;
       default:
            PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DATA_EVT,5000,
                    "MwIf> Error in Transceive(RX) (SEM) !! \n",&(mwIfHdl->sLastQueueData));
       break;
    }
    if(Pgu_event != NFA_DATA_EVT || gx_status != NFA_STATUS_OK)
    {
      ALOGE("MwIf> Error Did not get back data in TXVR (RX) !! \n");
      return MWIFSTATUS_FAILED;
    }
    phMwIfi_PrintBuffer(Pgs_cbTempBuffer,Pgui_cbTempBufferLength,"MwIf> TXVR Received = ");

    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGD("MwIf>%s:Device Disconnected",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }

    /* Make the Result Array */
    memcpy(pvOutBuff,Pgs_cbTempBuffer,Pgui_cbTempBufferLength);
    *dwLenOutBuff = Pgui_cbTempBufferLength;
    /*Reset Buffer Data length to avoid using previous chained data */
    Pgui_cbTempBufferLength = 0;
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

MWIFSTATUS phMwIf_SendRawFrame(void* mwIfHandle,
                                     void* pvInBuff,
                                     uint32_t dwLenInBuff)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGE("MwIf>%s:Device Disconnected Already",__FUNCTION__);
        return NFA_STATUS_FAILED;
    }

    phMwIfi_PrintBuffer(pvInBuff,dwLenInBuff,"MwIf> TXVR Sending = ");
    if(dwLenInBuff > 400)
    {
      ALOGE("MwIf> Error Buffer length %d > 400 !! \n",dwLenInBuff);
      return MWIFSTATUS_INVALID_PARAM;
    }
    if(pvInBuff == NULL)
    {
      ALOGE("MwIf> Invalid Buffer Ptr",dwLenInBuff);
      return MWIFSTATUS_INVALID_PARAM;
    }

    ALOGD("MwIf>%s:Sending Raw Buffer",__FUNCTION__);
    /* Send the Frame */
    gx_status = NFA_SendRawFrame(pvInBuff,(UINT16)dwLenInBuff,NFA_DM_DEFAULT_PRESENCE_CHECK_START_DELAY);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status,"MwIf> Error in performing Transceive(TX) !! \n");
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

void* phMwIfi_MemAllocCb(void* memHdl, uint32_t size) {
    void* addr = malloc(size);
    ALOGD("DTALib> Allocating mem Size=%d , Addr=0x%x", size, addr);
    return addr;
}
int32_t phMwIfi_MemFreeCb(void* memHdl, void* ptrToMem) {
    ALOGD("DTALib> Freeing mem Addr=0x%x", ptrToMem);
    free(ptrToMem);
    return 0;
}

void phMwIfi_MapRfInterface(UINT8* protocol, int *rf_interface)
{
    switch(*protocol)
    {
    case NFC_PROTOCOL_NFC_DEP:
        *rf_interface = NFC_INTERFACE_NFC_DEP;
        ALOGE("NFC-DEP interface");
        break;

    case NFC_PROTOCOL_ISO_DEP:
        *rf_interface = NFC_INTERFACE_ISO_DEP;
        ALOGE("ISO-DEP interface");
        break;
    case NFC_PROTOCOL_MIFARE:
        *rf_interface = NFC_INTERFACE_FRAME;
        ALOGE("FRAME RF interface");
        break;
    default:
        ALOGE("No matching protocol");
        break;
    }
}

/**
 * Initialize LLCP stack
 */
MWIFSTATUS phMwIf_LlcpInit(void*                     pvMwIfHandle,
                           phMwIf_sLlcpInitParams_t* psLlcpInitPrms)
{
    MWIFSTATUS           dwMwIfStatus;
    phMwIf_sHandle_t*    mwIfHdl = pvMwIfHandle;
    uint32_t             dwSizeofGenBytes;
    tNFA_STATUS          bNfaStatus;
    tNFA_P2P_EVT_DATA*   psP2pEventData;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !psLlcpInitPrms)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    NFA_EnableDtamode(NFA_DTA_LLCP_MODE);

    /*Set LLCP params in MW Stack*/
    bNfaStatus = NFA_P2pSetLLCPConfig (LLCP_MAX_MIU,
                                       LLCP_OPT_VALUE,
                                       PHMWIF_LLCP_WAITING_TIME,
                                       PHMWIF_LLCP_LTO_VALUE,
                                       0, /*use 0 for infinite timeout for symmetry procedure when acting as initiator*/
                                       0, /*use 0 for infinite timeout for symmetry procedure when acting as target*/
                                       PHMWIF_LLCP_DELAY_RESP_TIME,
                                       LLCP_DATA_LINK_TIMEOUT,
                                       LLCP_DELAY_TIME_TO_SEND_FIRST_PDU);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error in P2PSetLLCPConfig");

    /*Set Poll Mode General Bytes for ATR_REQ required by LLCP DTA*/
    dwMwIfStatus = phMwIf_SetConfig( pvMwIfHandle,
                                     NCI_PARAM_ID_ATR_REQ_GEN_BYTES,
                                     psLlcpInitPrms->sGenBytesInitiator.dwBuffLength,
                                     psLlcpInitPrms->sGenBytesInitiator.pbBuff);
    PH_ON_ERROR_RETURN(MWIFSTATUS_SUCCESS,dwMwIfStatus,
            "MwIf> Error Could not SetConfig ATR_REQ_GEN_BYTES for  LLCP");

    /*Set Listen Mode General Bytes for ATR_RESP required by LLCP DTA in  Mode*/
    dwMwIfStatus = phMwIf_SetConfig( pvMwIfHandle,
                                     NCI_PARAM_ID_ATR_RES_GEN_BYTES,
                                     psLlcpInitPrms->sGenBytesTarget.dwBuffLength,
                                     psLlcpInitPrms->sGenBytesTarget.pbBuff);
    PH_ON_ERROR_RETURN(MWIFSTATUS_SUCCESS,dwMwIfStatus,
            "MwIf> Error Could not SetConfig ATR_RES_GEN_BYTES for  LLCP");

    /*Register client and get client handle.
     * This client is used for SDP service request to send SNL*/
    bNfaStatus = NFA_P2pRegisterClient (NFA_P2P_LLINK_TYPE,
                                        phMwIfi_NfaLlcpClientCallback);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not register SDP Client");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_REG_CLIENT_EVT,5000,
            "MwIf>ERROR in receving client register event for SDP",&(mwIfHdl->sLastQueueData));
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnLessSDPClient.wClientHandle = psP2pEventData->reg_client.client_handle;
    ALOGD("MwIf>%s:SDP Connless Client Registration successful:EvtBlk=0x%x",__FUNCTION__,&(mwIfHdl->sLastQueueData));
    phMwIfi_PrintBuffer((uint8_t*)&(mwIfHdl->sLastQueueData),12,"phMwIf_LlcpInit:ClientConnlessEvt:");
    phMwIfi_PrintBuffer((uint8_t*)&(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData),12,"phMwIf_LlcpInit:P2PEvtData:");

    /*Register client and get client handle.
     * This client is used for Sending Connectionless Data to remote SAP*/
    bNfaStatus = NFA_P2pRegisterClient (NFA_P2P_LLINK_TYPE,
                                        phMwIfi_NfaLlcpClientCallback);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not register client for sending conn less data");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_REG_CLIENT_EVT,5000,
            "MwIf>ERROR in receving client register event for conn less data",&(mwIfHdl->sLastQueueData));
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnLessClient.wClientHandle = psP2pEventData->reg_client.client_handle;

    /*Register client and get the client handle for connection oriented Data*/
    bNfaStatus = NFA_P2pRegisterClient (NFA_P2P_DLINK_TYPE,
                                        phMwIfi_NfaLlcpClientCallback);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not register P2P Client");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_REG_CLIENT_EVT,5000,
            "MwIf>ERROR in receving client register event",&(mwIfHdl->sLastQueueData));
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnOrientedClient.wClientHandle = psP2pEventData->reg_client.client_handle;
    ALOGD("MwIf>%s:Client Registration successful",__FUNCTION__);

    /*Save Init params*/
    mwIfHdl->sLlcpPrms.sLlcpInitPrms = *psLlcpInitPrms;

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Register connection oriented server with service name and sap
 */
MWIFSTATUS  phMwIf_LlcpRegisterServerConnOriented(void*                         pvMwIfHandle,
                                                  phMwIf_sLlcpSrvrRegnParams_t* psLlcpSrvrPrms,
                                                  void**                        ppvServerHandle)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !psLlcpSrvrPrms || !ppvServerHandle)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    bNfaStatus = NFA_P2pRegisterServer ( psLlcpSrvrPrms->bSap,
                                        NFA_P2P_DLINK_TYPE,
                                        psLlcpSrvrPrms->pbServiceName,
                                        phMwIfi_NfaLlcpServerCallback);

    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not register P2P server");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_REG_SERVER_EVT,5000,
            "MwIf>ERROR in getting server event",&(mwIfHdl->sLastQueueData));

    /*Copy server handle received in NFA_P2P_REG_SERVER_EVT*/
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnOrientedServer.wServerHandle = psP2pEventData->reg_server.server_handle;
    *ppvServerHandle = psP2pEventData->reg_server.server_handle;

    /*Save Server registration params*/
    mwIfHdl->sLlcpPrms.sConnOrientedServer.sSrvrRegnPrms = *psLlcpSrvrPrms;

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Register connection oriented server with service name and sap
 */
MWIFSTATUS  phMwIf_LlcpRegisterServerConnLess(void*                         pvMwIfHandle,
                                              phMwIf_sLlcpSrvrRegnParams_t* psLlcpSrvrPrms,
                                              void**                        ppvServerHandle)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !psLlcpSrvrPrms || !ppvServerHandle)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    bNfaStatus = NFA_P2pRegisterServer ( psLlcpSrvrPrms->bSap,
                                        NFA_P2P_LLINK_TYPE,
                                        psLlcpSrvrPrms->pbServiceName,
                                        phMwIfi_NfaLlcpConnLessServerCallback);

    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not register P2P server");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_REG_SERVER_EVT,5000,
            "MwIf>ERROR in getting server event",&(mwIfHdl->sLastQueueData));

    /*Copy server handle received in NFA_P2P_REG_SERVER_EVT*/
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnOrientedServer.wServerHandle = psP2pEventData->reg_server.server_handle;
    *ppvServerHandle = psP2pEventData->reg_server.server_handle;

    /*Save Server registration params*/
    mwIfHdl->sLlcpPrms.sConnOrientedServer.sSrvrRegnPrms = *psLlcpSrvrPrms;

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Connect to a remote server
 */
MWIFSTATUS phMwIf_LlcpClientConnect( void*                               pvMwIfHandle,
                                     phMwIf_sLlcpClientConnectParams_t*  psConnectPrms,
                                     void**                              ppvRemoteServerConnHandle)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !psConnectPrms || !ppvRemoteServerConnHandle)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    if(psConnectPrms->eConnectType == PHMWIF_SERVER_CONNECT_BY_NAME)
    {/*Connect to server by using server service name instead of SAP*/
        ALOGD("MwIf>Connecting to Server:%s",psConnectPrms->pbServerServiceName);
        bNfaStatus = NFA_P2pConnectByName(mwIfHdl->sLlcpPrms.sConnOrientedClient.wClientHandle,
                                          psConnectPrms->pbServerServiceName,
                                          psConnectPrms->wClientMiu,
                                          psConnectPrms->bClientRw);
        PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not connect to server by service name");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_CONNECTED_EVT,5000,
                "MwIf>ERROR in receving client register event",&(mwIfHdl->sLastQueueData));
    }
    else
    {/*Connect to server by using SAP instead of service name*/
        ALOGD("MwIf>Connecting to Server:SAP=0x%x",psConnectPrms->bSap);
        bNfaStatus = NFA_P2pConnectBySap(mwIfHdl->sLlcpPrms.sConnOrientedClient.wClientHandle,
                                         psConnectPrms->bSap,
                                         psConnectPrms->wClientMiu,
                                         psConnectPrms->bClientRw);
        PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not connect to server by SAP");
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_CONNECTED_EVT,5000,
                "MwIf>ERROR in receving client register event",&(mwIfHdl->sLastQueueData));
    }

    /*Save client connect params*/
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
    mwIfHdl->sLlcpPrms.sConnOrientedClient.wRemoteServerConnHandle = psP2pEventData->connected.conn_handle;
    *ppvRemoteServerConnHandle = psP2pEventData->connected.conn_handle;
    mwIfHdl->sLlcpPrms.sConnOrientedClient.sLlcpClientConnectPrms = *psConnectPrms;

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Disconnect Client from remote server
 */
MWIFSTATUS phMwIf_LlcpClientDisconnect( void*     pvMwIfHandle,
                                        void*     pvConnHandle)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pvConnHandle)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    bNfaStatus = NFA_P2pDisconnect(pvConnHandle,
                                   TRUE);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not Disconnect client from P2P");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_DISC_EVT,5000,
            "MwIf>ERROR in Disconnecting client connection",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;

}

/**
 * Accept the connection requested by remote client
 */
MWIFSTATUS phMwIf_LlcpServerAcceptClientConn(void*       pvMwIfHandle,
                                             void*       pvServerHandle,
                                             uint16_t    wServerMiu,
                                             uint8_t     bServerRw,
                                             void*       pvConnHandle)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pvServerHandle || !pvConnHandle)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    bNfaStatus = NFA_P2pAcceptConn(pvConnHandle,
                                   wServerMiu,
                                   bServerRw);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not Accept the connection");
    /*
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_CONNECTED_EVT,5000,
            "MwIf>ERROR in Acceping client connection",&(mwIfHdl->sLastQueueData));
*/
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Wait for data on the connection handle
 */
MWIFSTATUS phMwIf_LlcpRecvData(void*                     pvMwIfHandle,
                               void*                     pvConnHandle,
                               phMwIf_sBuffParams_t*     psData)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    BOOLEAN  blReqdDataFound = FALSE;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    tNFA_STATUS bNfaStatus;
    BOOLEAN     blMoreDataRemaining;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pvConnHandle || !psData)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    /*Wait until data is received for the connection handle provided*/
    do
    {
        PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_DATA_EVT,5000,
                "MwIf>ERROR in LLCP receivedata:Didnt get P2P_DATA_EVT",&(mwIfHdl->sLastQueueData));
        psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);
        ALOGD("MwIf>%s:pvConnHandle=0x%x,Data recvd From Handle=0x%x",__FUNCTION__,pvConnHandle,mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData.data.handle);
        if(pvConnHandle == psP2pEventData->data.handle)
        {
            ALOGD("MwIf>Received data event from required connection");
            blReqdDataFound = TRUE;
        }
    }while(blReqdDataFound);

    /*Check whether the connection is connection oriented or connection less*/
    if(pvConnHandle == mwIfHdl->sLlcpPrms.sConnOrientedServer.wRemoteClientConnHandle)
    {
        ALOGD("MwIf>Get P2P data");
        bNfaStatus = NFA_P2pReadData (pvConnHandle,
                         psData->dwBuffLength,
                         &psData->dwBuffLength,
                         psData->pbBuff,
                         &blMoreDataRemaining);
        PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not Read Data");
    }

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Send data to remote device connected by connection handle
 */
MWIFSTATUS phMwIf_LlcpSendData(void*                     pvMwIfHandle,
                               void*                     pvConnHandle,
                               phMwIf_sBuffParams_t*     psData)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pvConnHandle || !psData)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    /*Check whether the connection is connection oriented or connection less*/
    if(pvConnHandle == mwIfHdl->sLlcpPrms.sConnOrientedClient.wRemoteServerConnHandle)
    {
        ALOGD("MwIf>Send P2P data");
        bNfaStatus = NFA_P2pSendData (pvConnHandle,
                                     psData->dwBuffLength,
                                     psData->pbBuff);
        PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not send Data");
    }

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Wait for data on the connection handle
 */
MWIFSTATUS phMwIf_LlcpConnLessRecvData(void*                     pvMwIfHandle,
                                       void*                     pvConnHandle,
                                       uint8_t*                  pbRemoteSap,
                                       phMwIf_sBuffParams_t*     psData)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    BOOLEAN  blReqdDataFound = FALSE;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    tNFA_STATUS bNfaStatus;
    BOOLEAN     blMoreDataRemaining;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pvConnHandle || !psData)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    ALOGD("MwIf>Get Unnumbered Info");
    bNfaStatus = NFA_P2pReadUI (pvConnHandle,
                         psData->dwBuffLength,
                         pbRemoteSap,
                         &psData->dwBuffLength,
                         psData->pbBuff,
                         &blMoreDataRemaining);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not Read UI Data");

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Send connection less data to remote SAP
 */
MWIFSTATUS phMwIf_LlcpConnLessSendData(void*                     pvMwIfHandle,
                                       uint8_t                   bRemoteSap,
                                       phMwIf_sBuffParams_t*     psData)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_STATUS bNfaStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !bRemoteSap || !psData || !psData->dwBuffLength || !psData->pbBuff)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    ALOGD("MwIf>Send P2P data");
    bNfaStatus = NFA_P2pSendUI(mwIfHdl->sLlcpPrms.sConnLessClient.wClientHandle,
                               bRemoteSap,
                               psData->dwBuffLength,
                               psData->pbBuff);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
                "MwIf> Error Could not send Data");

    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Get the Remote Server SAP for corressponding service name
 */
MWIFSTATUS phMwIf_LlcpServiceDiscovery( void*     pvMwIfHandle,
                                        uint8_t*  pbServiceName,
                                        uint8_t*  pbSAP)
{
    phMwIf_sHandle_t *mwIfHdl = pvMwIfHandle;
    tNFA_P2P_EVT_DATA *psP2pEventData;
    tNFA_STATUS bNfaStatus;
    ALOGD("MwIf>%s:enter",__FUNCTION__);

    if(!pvMwIfHandle || !pbSAP)
    {
        ALOGE("MwIf>%s:Error!:Invalid Params",__FUNCTION__);
        return MWIFSTATUS_INVALID_PARAM;
    }

    bNfaStatus = NFA_P2pGetRemoteSap (mwIfHdl->sLlcpPrms.sConnLessSDPClient.wClientHandle,
                                      pbServiceName);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,bNfaStatus,
            "MwIf> Error Could not Accept the connection");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_P2P_EVT_OFFSET+NFA_P2P_SDP_EVT,5000,
            "MwIf>ERROR in Acceping client connection",&(mwIfHdl->sLastQueueData));
    psP2pEventData = &(mwIfHdl->sLastQueueData.uEvtData.sP2pEvtData);

    *pbSAP = psP2pEventData->sdp.remote_sap;
    ALOGD("MwIf>%s:exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * LLCP Server callback from middleware
 */
void phMwIfi_NfaLlcpServerCallback(tNFA_P2P_EVT eP2pEvent,tNFA_P2P_EVT_DATA *psP2pEventData)
{
    ALOGD("MwIf>%s:Enter: Event = 0x%x",__FUNCTION__,eP2pEvent);
    phMwIf_sHandle_t    *mwIfHdl     = &g_mwIfHandle;
    phMwIf_sLlcpPrms_t  *psLlcpPrms  = &mwIfHdl->sLlcpPrms;
    phMWIf_eLlcpEvtType_t  eLlcpEvtType;
    phMwIf_uLlcpEvtInfo_t  sLlcpEvtInfo;
    phMwIf_sQueueData_t *psQueueData = NULL;
    BOOLEAN             bPushToQReqd = FALSE;
    BOOLEAN             bCallbackReqd = FALSE;
    if(psP2pEventData != NULL)
    {
        switch (eP2pEvent)
        {
            case NFA_P2P_REG_SERVER_EVT:
            {
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_ACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Activated !! \n");
            }
            break;
            case NFA_P2P_DEACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Deactivated !! \n");
            }
            break;
            case NFA_P2P_CONN_REQ_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Client Connection Request \n ");
                eLlcpEvtType = PHMWIF_LLCP_SERVER_CONN_REQ_EVT;
                sLlcpEvtInfo.sConnReq.wServerHandle  = psP2pEventData->conn_req.server_handle;
                sLlcpEvtInfo.sConnReq.wConnHandle    = psP2pEventData->conn_req.conn_handle;
                sLlcpEvtInfo.sConnReq.bRemoteSap     = psP2pEventData->conn_req.remote_sap;
                sLlcpEvtInfo.sConnReq.wRemoteMiu     = psP2pEventData->conn_req.remote_miu;
                sLlcpEvtInfo.sConnReq.bRemoteRw      = psP2pEventData->conn_req.remote_rw;
                bCallbackReqd = TRUE;
                bPushToQReqd = FALSE;
            }
            break;
            case NFA_P2P_CONNECTED_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Connected Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_DISC_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Disconnected Event \n ");
                eLlcpEvtType = PHMWIF_LLCP_CO_REMOTE_CLIENT_DICONNECTED_EVT;
                sLlcpEvtInfo.sDisconnect.bReasonForDisconnect  = psP2pEventData->disc.reason;
                sLlcpEvtInfo.sDisconnect.wConnHandle           = psP2pEventData->disc.handle;
                bPushToQReqd  = FALSE;
                bCallbackReqd = TRUE;
            }
            break;
            case NFA_P2P_DATA_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Data Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_CONGEST_EVT:
            {
                ALOGD("MwIf>LlcpSrvrCb: P2P Link Congestion Event \n");
            }
            break;
            default:
                ALOGE("P2P Unknown Event");
            break;
        }

        if(bPushToQReqd)
        {
            psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
            memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
            psQueueData->dwEvtType = NFA_P2P_EVT_OFFSET + eP2pEvent;
            if(psP2pEventData)
                psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*psP2pEventData;
            if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
            {
                ALOGE("MwIf>%s Error Could not Push to Queue\n",__FUNCTION__);
                gx_status = NFA_STATUS_FAILED;
            }
        }
        else
        {
            ALOGE("Mwif>Skipping Event 0x%x not pushed to queue\n",eP2pEvent);
        }

        /*Update the users of Mwif about event received using callback*/
        if(bCallbackReqd)
        {
            if(mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb != NULL)
            {
                ALOGD ("MwIf>Calling LLCP Cback:MwIfEvt=0x%x,MwEvt=0x%x",eLlcpEvtType,eP2pEvent);
                mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb(mwIfHdl,
                                                              mwIfHdl->sLlcpPrms.sLlcpInitPrms.pvApplHdl,
                                                              eLlcpEvtType,
                                                              &sLlcpEvtInfo);
            }
            else
            {
                ALOGE("MwIf>App Callback not registered");
            }
        }
    }

     ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return;
}

/**
 * LLCP Server callback from middleware
 */
void phMwIfi_NfaLlcpConnLessServerCallback(tNFA_P2P_EVT eP2pEvent,tNFA_P2P_EVT_DATA *psP2pEventData)
{
    ALOGD("MwIf>%s:Enter: Event = 0x%x",__FUNCTION__,eP2pEvent);
    phMwIf_sHandle_t    *mwIfHdl     = &g_mwIfHandle;
    phMwIf_sLlcpPrms_t  *psLlcpPrms  = &mwIfHdl->sLlcpPrms;
    phMWIf_eLlcpEvtType_t  eLlcpEvtType;
    phMwIf_uLlcpEvtInfo_t  sLlcpEvtInfo;
    phMwIf_sQueueData_t *psQueueData = NULL;
    BOOLEAN             bPushToQReqd = FALSE;
    BOOLEAN             bCallbackReqd = FALSE;
    if(psP2pEventData != NULL)
    {
        switch (eP2pEvent)
        {
            case NFA_P2P_REG_SERVER_EVT:
            {
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_ACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Activated !! \n");
            }
            break;
            case NFA_P2P_DEACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Deactivated !! \n");
            }
            break;
            case NFA_P2P_CONNECTED_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Connected Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_DISC_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Discovery Event \n ");
            }
            break;
            case NFA_P2P_DATA_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Data Event ");
                eLlcpEvtType  = PHMWIF_LLCP_SERVER_CONN_LESS_DATA_EVT;
                sLlcpEvtInfo.sLlcpConnLessData.wConnLessHandle = psP2pEventData->data.handle;
                sLlcpEvtInfo.sLlcpConnLessData.bRemoteSap      = psP2pEventData->data.remote_sap;
                bCallbackReqd = TRUE;
            }
            break;
            case NFA_P2P_CONGEST_EVT:
            {
                ALOGD("MwIf>LlcpConnlessSrvrCb: P2P Link Congestion Event \n");
            }
            break;
            default:
                ALOGE("P2P Unknown Event");
            break;
        }

        if(bPushToQReqd)
        {
            psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
            memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
            psQueueData->dwEvtType = NFA_P2P_EVT_OFFSET + eP2pEvent;
            if(psP2pEventData)
                psQueueData->uEvtData = (phMwIf_uCbEvtData_t)*psP2pEventData;
            if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
            {
                ALOGE("MwIf>%s Error Could not Push to Queue\n",__FUNCTION__);
                gx_status = NFA_STATUS_FAILED;
            }
        }
        else
        {
            ALOGE("Mwif>Skipping Event 0x%x not pushed to queue\n",eP2pEvent);
        }

        /*Update the users of Mwif about event received using callback*/
        if(bCallbackReqd)
        {
            if(mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb != NULL)
            {
                ALOGD ("MwIf>Calling LLCP Cback:MwIfEvt=0x%x,MwEvt=0x%x",eLlcpEvtType,eP2pEvent);
                mwIfHdl->sLlcpPrms.sLlcpInitPrms.pfMwIfLlcpCb(mwIfHdl,
                                                              mwIfHdl->sLlcpPrms.sLlcpInitPrms.pvApplHdl,
                                                              eLlcpEvtType,
                                                              &sLlcpEvtInfo);
            }
            else
            {
                ALOGE("MwIf>App Callback not registered");
            }
        }
    }

     ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return;
}

/**
 * LLCP Client callback from middleware
 */
void phMwIfi_NfaLlcpClientCallback(tNFA_P2P_EVT eP2pEvent,tNFA_P2P_EVT_DATA *psP2pEventData)
{
    ALOGD("MwIf>%s:Enter:Event=0x%x",__FUNCTION__,eP2pEvent);
    phMwIf_sHandle_t    *mwIfHdl     = &g_mwIfHandle;
    phMwIf_sLlcpPrms_t  *psLlcpPrms  = &mwIfHdl->sLlcpPrms;
    BOOLEAN             bPushToQReqd = FALSE;
    phMwIf_sQueueData_t *psQueueData = NULL;
    if(psP2pEventData != NULL)
    {
        switch (eP2pEvent)
        {

            case NFA_P2P_REG_CLIENT_EVT:
            {
                ALOGD("MwIf>%s:Client Handle=0x%x",__FUNCTION__,psP2pEventData->reg_client.client_handle);
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_ACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Activated !! \n");
            }
            break;
            case NFA_P2P_DEACTIVATED_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Deactivated !! \n");
            }
            break;
            case NFA_P2P_CONNECTED_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Client Connected to Remote Server \n ");
                /*
                psLlcpPrms->wRemoteServerConnHandle = psP2pEventData->conn_req.conn_handle;
                psLlcpPrms->wRemoteServerMIU        = psP2pEventData->conn_req.remote_miu;
                psLlcpPrms->wRemoteServerRW         = psP2pEventData->conn_req.remote_rw;
                */
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_DISC_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Discovery Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_SDP_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P SDP Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_DATA_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Data Event \n ");
                bPushToQReqd = TRUE;
            }
            break;
            case NFA_P2P_CONGEST_EVT:
            {
                ALOGD("MwIf>LlcpClientCb:  P2P Link Congestion Event \n");
            }
            break;
            default:
                ALOGE("P2P Unknown Event");
            break;
        }

        if(bPushToQReqd)
        {
            psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
            memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
            psQueueData->dwEvtType = NFA_P2P_EVT_OFFSET + eP2pEvent;
            if(psP2pEventData && psQueueData)
            {
                memcpy(&(psQueueData->uEvtData.sP2pEvtData),psP2pEventData,sizeof(tNFA_P2P_EVT_DATA));
            }
            else
            {
                ALOGE("MwIf>%s Error! Invalid P2P data\n",__FUNCTION__);
            }
            if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
            {
                ALOGE("MwIf>%s Error Could not Push to Queue\n",__FUNCTION__);
                gx_status = NFA_STATUS_FAILED;
            }
        }
    }
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return;
}

/**
* Copy Activation parameters received to event Info structure which will passed with ACTIVATED event
*/
void phMwIfi_CopyActivationPrms(phMWIf_sActivatedEvtInfo_t* psActivationPrms,
                                tNFC_ACTIVATE_DEVT*         psActivationNtfPrms)
{
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    /*Copy the activation data to event data structure to be sent*/
    psActivationPrms->bRfDiscId        = psActivationNtfPrms->rf_disc_id;
    psActivationPrms->eProtocolType    = psActivationNtfPrms->protocol;
    psActivationPrms->eRfInterfaceType = psActivationNtfPrms->intf_param.type;
    psActivationPrms->eRxBitRate       = psActivationNtfPrms->rx_bitrate;
    psActivationPrms->eTxBitRate       = psActivationNtfPrms->tx_bitrate;
    psActivationPrms->eActivatedRfTechTypeAndMode = psActivationNtfPrms->rf_tech_param.mode;
    psActivationPrms->eDataXchgRfTechTypeAndMode  = psActivationNtfPrms->data_mode;
    memcpy(&psActivationPrms->uRfTechParams,&psActivationNtfPrms->rf_tech_param.param,sizeof(phMwIf_uNciRfTechParams_t));
    memcpy(&psActivationPrms->uRfIntfParams,&psActivationNtfPrms->intf_param.intf_param,sizeof(phMwIf_uNciRfIntfParams_t));
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return;
}

/**
 * Handle NFA activation events
 */
void phMwIfi_HandleActivatedEvent(tNFA_ACTIVATED*     psActivationPrms,
                                  BOOLEAN*            pblPushToQReqd,
                                  BOOLEAN*            pblCallbackReqd,
                                  phMWIf_uEvtInfo_t*  puEvtInfo,
                                  phMWIf_eEvtType_t*  peMwIfEvtType)
{
    phMwIf_sHandle_t    *mwIfHdl     = &g_mwIfHandle;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    *pblPushToQReqd = FALSE;
    phMwIfi_PrintNfaActivationParams(&psActivationPrms->activate_ntf);
    /*gb_device_connected = TRUE;*/
    mwIfHdl->eDeviceState = DEVICE_IN_POLL_ACTIVE_STATE;
    ALOGD("MwIf>Device Connected %d",__LINE__);

    phMwIfi_CopyActivationPrms(&puEvtInfo->sActivationPrms,&psActivationPrms->activate_ntf);
    switch(psActivationPrms->activate_ntf.protocol)
    {
       case NFC_PROTOCOL_T1T:
       {
           ALOGD ("MwIf> T1T Card - HR[0] = 0x%x HR[1] = 0x%x \n",
           psActivationPrms->params.t1t.hr[0],
           psActivationPrms->params.t1t.hr[1]);
           phMwIfi_PrintBuffer(psActivationPrms->params.t1t.uid,4,"UID =");
           *pblCallbackReqd     = TRUE;
           *peMwIfEvtType         = PHMWIF_T1T_TAG_ACTIVATED_EVT;
       }
       break;
       case NFC_PROTOCOL_T2T:
       {
           phMwIfi_PrintBuffer(psActivationPrms->params.t2t.uid,10,
           "MwIf> T2T Card - UID= ");
           phMwIfi_PrintBuffer(psActivationPrms->activate_ntf.rf_tech_param.param.pa.sens_res,2,"ATQA : ");
           ALOGD("SAK: 0x%02X", psActivationPrms->activate_ntf.rf_tech_param.param.pa.sel_rsp);
           *pblCallbackReqd = TRUE;
           *peMwIfEvtType = PHMWIF_T2T_TAG_ACTIVATED_EVT;
       }
       break;
       case NFC_PROTOCOL_T3T:
       {
           phMwIfi_PrintBuffer16(psActivationPrms->params.t3t.p_system_codes,
           (uint16_t)psActivationPrms->params.t3t.num_system_codes,
           "MwIf> T3T Card - System Codes = ");
           *pblCallbackReqd = TRUE;
           *peMwIfEvtType = PHMWIF_T3T_TAG_ACTIVATED_EVT;
           usleep(1);
       }
       break;
       case NFC_PROTOCOL_ISO_DEP:
       {
           ALOGD("MwIf> T4T Card ");
           if(psActivationPrms->activate_ntf.rf_tech_param.mode == NFC_DISCOVERY_TYPE_POLL_A)
           {
               ALOGD("T4AT Type card \n");
               phMwIfi_PrintBuffer(psActivationPrms->activate_ntf.rf_tech_param.param.pa.nfcid1,
               (uint16_t)psActivationPrms->activate_ntf.rf_tech_param.param.pa.nfcid1_len,
               "UID: ");
               phMwIfi_PrintBuffer(psActivationPrms->activate_ntf.rf_tech_param.param.pa.sens_res,
               2,"ATQA : ");
               ALOGD("SAK: 0x%02X",
               psActivationPrms->activate_ntf.rf_tech_param.param.pa.sel_rsp);
           }
           else
           {
               ALOGD("T4BT Type card \n");
               phMwIfi_PrintBuffer(psActivationPrms->activate_ntf.rf_tech_param.param.pb.nfcid0,
               4,"UID: ");
               phMwIfi_PrintBuffer(psActivationPrms->activate_ntf.rf_tech_param.param.pb.sensb_res,
               psActivationPrms->activate_ntf.rf_tech_param.param.pb.sensb_res_len,
               "ATQB: ");
           }
           *pblCallbackReqd = TRUE;
           *peMwIfEvtType = PHMWIF_ISODEP_ACTIVATED_EVT;
       }
       break;
       case NFC_PROTOCOL_NFC_DEP:
       {
           ALOGD("MwIf> NFC DEP Detected \n");
           switch(psActivationPrms->activate_ntf.rf_tech_param.mode)
           {
           case NFC_DISCOVERY_TYPE_POLL_A:
               ALOGD("MwIf>P2P NFC-A Passive Target Found \n");
           break;
           case NFC_DISCOVERY_TYPE_POLL_F:
               ALOGD("MwIf>P2P NFC-F Passive Target Found \n");
           break;
           case NFC_DISCOVERY_TYPE_POLL_A_ACTIVE:
               ALOGD("MwIf>P2P NFC-A Active Target Found \n");
           break;
           case NFC_DISCOVERY_TYPE_POLL_F_ACTIVE:
               ALOGD("MwIf>P2P NFC-F Active Target Found \n");
           break;
           case NFC_DISCOVERY_TYPE_LISTEN_A:
               ALOGD("MwIf>P2P NFC-A Passive Initiator Found \n");
           break;
           case NFC_DISCOVERY_TYPE_LISTEN_F:
               ALOGD("MwIf>P2P NFC-F Passive Initiator Found \n");
           break;
           case NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE:
               ALOGD("MwIf>P2P NFC-A Active Initiator Found \n");
           break;
           case NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE:
               ALOGD("MwIf>P2P NFC-F Active Initiator Found \n");
           break;
           default:
               ALOGD("MwIf>Invalid NFC DEP type \n");
           break;
           }/*switch(psActivationPrms->activate_ntf.rf_tech_param.mode)*/

           *pblCallbackReqd = TRUE;
           *peMwIfEvtType = PHMWIF_NFCDEP_ACTIVATED_EVT;
       }/*case NFC_PROTOCOL_NFC_DEP:*/
       break;
       case NFC_PROTOCOL_UNKNOWN:
           ALOGE("MwIf> Handling Secure Element test cases \n");
       break;
       default:
           ALOGE("MwIf> Undefined Protocol!! \n");
       break;
   }/*switch(psActivationPrms->activate_ntf.protocol)*/
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return;
}

/**
 * Receive P2P data from middleware
 */
MWIFSTATUS phMwIf_ReceiveP2PData(void *mwIfHandle, void* pvOutBuff, uint32_t* dwLenOutBuff)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGD("MwIf>%s:Device Disconnected",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }
    ALOGD("MwIf>%s:Waiting for Data",__FUNCTION__);
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DATA_EVT,20000,
                    "MwIf> Error in ReceiveP2PData(RX) (SEM) !! \n",&(mwIfHdl->sLastQueueData));
    if(Pgu_event != NFA_DATA_EVT || gx_status != NFA_STATUS_OK)
    {
        ALOGE("MwIf> Error Did to get back data in (RX) !! \n");
        return MWIFSTATUS_FAILED;
    }
    phMwIfi_PrintBuffer(Pgs_cbTempBuffer,Pgui_cbTempBufferLength,"MwIf> DATA Received = ");
    /*if(!gb_device_connected)*/
    if(mwIfHdl->eDeviceState != DEVICE_IN_POLL_ACTIVE_STATE)
    {
        ALOGD("MwIf>%s:Device Disconnected",__FUNCTION__);
        return MWIFSTATUS_FAILED;
    }
    /* Copy the received data, received through NFA_DATA_EVT*/
    memcpy(pvOutBuff,Pgs_cbTempBuffer,Pgui_cbTempBufferLength);
    *dwLenOutBuff = Pgui_cbTempBufferLength;
    /*Reset Buffer Data length to avoid using previous chained data */
    Pgui_cbTempBufferLength = 0;
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

/**
 * Send Deactivate command to NFCC
 */
MWIFSTATUS phMwIf_NfcDeactivate(void*                    mwIfHandle,
                                phMWIf_eDeactivateType_t eDeactType)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    ALOGD("MwIf>eDeactType = %d",eDeactType);
    if(eDeactType == PHMWIF_DEACTIVATE_TYPE_SLEEP || eDeactType == PHMWIF_DEACTIVATE_TYPE_SLEEP_AF)
    {
        ALOGD("MwIf>eDeactType == PHMWIF_DEACTIVATE_TYPE_SLEEP || eDeactType == PHMWIF_DEACTIVATE_TYPE_SLEEP_AF");
        gx_status = NFA_Deactivate(TRUE);
    }
    else if(eDeactType == PHMWIF_DEACTIVATE_TYPE_DISCOVERY || eDeactType == PHMWIF_DEACTIVATE_TYPE_IDLE)
    {
        ALOGD("MwIf>eDeactType == PHMWIF_DEACTIVATE_TYPE_DISCOVERY || eDeactType == PHMWIF_DEACTIVATE_TYPE_IDLE");
        gx_status = NFA_Deactivate(FALSE);
    }
    PH_ON_ERROR_RETURN(NFA_STATUS_OK,gx_status, "MwIf> Error Could not Deactivate");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,NFA_DEACTIVATED_EVT,5000, "MwIf> Error in NfcDeactivate (SEM) !! \n",&(mwIfHdl->sLastQueueData));

    ALOGD("MwIf>%s:Exit",__FUNCTION__);
    return MWIFSTATUS_SUCCESS;
}

void phMwIfi_SendNxpNciCommand(void *mwIfHandle,
                                     uint8_t cmd_params_len,
                                     uint8_t *p_cmd_params,
                                     tNFA_NXP_NCI_RSP_CBACK *p_cback)
{
    phMwIf_sHandle_t *mwIfHdl = mwIfHandle;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    gx_status = NFA_SendNxpNciCommand (cmd_params_len, p_cmd_params, p_cback);
    PH_ON_ERROR_RETURN(NFA_STATUS_OK, gx_status, "MwIf> Error Could not send NxpNciCommand");
    PH_WAIT_FOR_CBACK_EVT(mwIfHdl->pvQueueHdl,(NFA_NXPCFG_EVT_OFFSET+NCI_RSP_EVT),5000, "MwIf> Error in SendNxpNciCommand (SEM) !! \n",&(mwIfHdl->sLastQueueData));
    ALOGD("MwIf>%s:Exit",__FUNCTION__);
}

void phMwIfi_NfaNxpNciPropCommRspCallback(UINT8 event, UINT16 param_len, UINT8 *p_param)
{
    phMwIf_sHandle_t *mwIfHdl = &g_mwIfHandle;
    phMwIf_sQueueData_t *psQueueData = NULL;
    ALOGD("MwIf>%s:Enter",__FUNCTION__);
    ALOGD("DEBUG> Data length = %d event = 0x%x\n",param_len, event);
    if(p_param[3] == 0x00)
    {
        ALOGD("NFA_STATUS_OK\n");
    }
    else
    {
        ALOGD("NFA_STATUS_FAILED\n");
    }

    psQueueData = (phMwIf_sQueueData_t*)malloc(sizeof(phMwIf_sQueueData_t));
    memset(psQueueData,0,sizeof(phMwIf_sQueueData_t));
    psQueueData->dwEvtType = NFA_NXPCFG_EVT_OFFSET + event;
    if(p_param)
    {
        if(param_len > MAX_NXPCFG_RSP_DATA)
        {
            ALOGE("MwIf>RSP Data too high!! Length=0x%x",param_len);
        }
        psQueueData->uEvtData.sNxpCfgEvtData.dwSize = param_len;
        memcpy(psQueueData->uEvtData.sNxpCfgEvtData.abCfgRspData,p_param,param_len);
    }
    if (phOsal_QueuePush(mwIfHdl->pvQueueHdl,psQueueData,0) != NFA_STATUS_OK)
    {
        ALOGE("MwIf>%s Error Could not Push to Queue\n",__FUNCTION__);
        gx_status = NFA_STATUS_FAILED;
    }

    ALOGD("MwIf>%s:Exit",__FUNCTION__);
}

/*Wrapper for MW EE Tech Routing API*/
tNFA_STATUS phMwIfi_EeSetDefaultTechRouting (tNFA_HANDLE          ee_handle,
                                             tNFA_TECHNOLOGY_MASK technologies_switch_on,
                                             tNFA_TECHNOLOGY_MASK technologies_switch_off,
                                             tNFA_TECHNOLOGY_MASK technologies_battery_off)
{
#if  L_OSP_ENABLE
     return  NFA_EeSetDefaultTechRouting (ee_handle,technologies_switch_on,
                                          technologies_switch_off,technologies_battery_off,
                                          0x0,0x0);
#else
     return  NFA_EeSetDefaultTechRouting (ee_handle,technologies_switch_on,
                                          technologies_switch_off,technologies_battery_off);
#endif
}

/*Wrapper for MW EE Proto Routing API*/
tNFA_STATUS phMwIfi_EeSetDefaultProtoRouting (tNFA_HANDLE         ee_handle,
                                              tNFA_PROTOCOL_MASK  protocols_switch_on,
                                              tNFA_PROTOCOL_MASK  protocols_switch_off,
                                              tNFA_PROTOCOL_MASK  protocols_battery_off)
{
#if  L_OSP_ENABLE
    return NFA_EeSetDefaultProtoRouting (ee_handle,protocols_switch_on,
                                         protocols_switch_off,protocols_battery_off,
                                         0x0,0x0);
#else
    return NFA_EeSetDefaultProtoRouting (ee_handle,protocols_switch_on,
                                         protocols_switch_off,protocols_battery_off);
#endif
}

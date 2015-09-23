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

#ifndef __PH_DTATST_H__
#define __PH_DTATST_H__

#include "phDTAOSAL.h"

#define DTA_VERSION "NFCDTA_06.02"
#define PHDTALIB_PATTERN_NUM_ANALOG_TEST 0x1000

/**< Error Comparison with Return and Print */
#define PH_ON_ERROR_RETURN(status,retstatus,message) \
    if(retstatus != status)\
    {\
        phOsal_LogError(message);\
        return retstatus;\
    }
#define LOG_FUNCTION_ENTRY phOsal_LogDebugString((const uint8_t*)"DtaLib>:Entered:",(const uint8_t*)__FUNCTION__)
#define LOG_FUNCTION_EXIT  phOsal_LogDebugString((const uint8_t*)"DtaLib>:Exit:",(const uint8_t*)__FUNCTION__)

/** \ingroup grp_dta_lib
    List of LLCP variables */
static const char gs_abInboundCoService[]     = "urn:nfc:sn:dta-co-echo-in";
#define PHDTALIB_SAP_LT_CO_IN_SRC             ((uint8_t) 0x20) /* Conn Oriented Client on LT*/
#define PHDTALIB_SAP_IUT_CO_IN_DEST           ((uint8_t) 0x14) /* Conn Oriented Server on IUT*/
static const char gs_abOutboundCoService[]    = "urn:nfc:sn:dta-co-echo-out";
#define PHDTALIB_SAP_LT_CO_OUT_DEST           ((uint8_t) 0x12) /* Conn Oriented Server on LT*/
#define PHDTALIB_SAP_IUT_CO_OUT_SRC           ((uint8_t) 0x22) /* Conn Oriented Client on IUT*/
static const char gs_abInboundClService[]     = "urn:nfc:sn:dta-cl-echo-in";
#define PHDTALIB_SAP_LT_CL_IN_SRC             ((uint8_t) 0x21) /* Conn Less Client on LT*/
#define PHDTALIB_SAP_IUT_CL_IN_DEST           ((uint8_t) 0x13) /* Conn Less Server on IUT*/
static const char gs_abOutboundClService[]    = "urn:nfc:sn:dta-cl-echo-out";
#define PHDTALIB_SAP_LT_CL_OUT_DEST           ((uint8_t) 0x11) /* Conn Less Server on LT*/
#define PHDTALIB_SAP_IUT_CL_OUT_SRC           ((uint8_t) 0x23) /* Conn Less Client on IUT*/

/** \ingroup grp_dta_lib
    LLCP related constants */
/**< LLCP Connection Oriented Functionality setting up by SAP or  Connection-less Functionality */
#define    PHDTALIB_LLCP_CO_SET_SAP_OR_CL   0x1200
/**< LLCP Connection Oriented Functionality setting-up by name or  Connection-less Functionality */
#define    PHDTALIB_LLCP_CO_SET_NAME_OR_CL  0x1240
/**< LLCP Connection Oriented Functionality setting up by using SNL or  Connection-less Functionality */
#define    PHDTALIB_LLCP_CO_SET_SNL_OR_CL   0x1280
/*Delay Time before sending the loopback data for connectionless service.As per DTA spec 3000ms */
#define    TDELAY_CL_IN_MS                  3000

typedef struct phDtaLib_sHandle {
    void*                           mwIfHdl;
    void*                           dtaThreadHdl;
    void*                           queueHdl;
    phDtaLib_sTestProfile_t         sTestProfile;
    phMwIf_sDiscCfgPrms_t           sPrevdiscCfgParams;
    phDtaLib_eCeDevType_t           CeDevType;
    BOOLEAN                         bEnableCE;
    phDtaLib_eLlcpSnepType_t        p2pType;
    BOOLEAN                         bEnableP2P;
    phMWIf_sActivatedEvtInfo_t      sT3TActivationParams;
    void*                           pvCORemoteServerConnHandle; /**<Connection Oriented Remote Server Connection*/
    void*                           pvCORemoteClientConnHandle; /**<Connection Oriented Remote Client Connection*/
    void*                           pvCOServerHandle;           /**<Connection Oriented Server*/
    void*                           pvCOClientHandle;           /**<Connection Oriented Server*/
    void*                           pvCLServerHandle;           /**<ConnectionLess Server*/
    void*                           pvCLRemoteClientConnLessHandle; /**<Connection Less Remote Client Handle*/
    uint8_t                         bRemoteSapClEchoOut;
    BOOLEAN                         bDtaInitialized;            /**Flag to indicate DTA Initialization is  Done*/
} phDtaLib_sHandle_t;

typedef union phDtaLib_eEvtType
{
    phMWIf_eEvtType_t eDpEvtType;/**<  Digital Protocol related events */
    phMWIf_eLlcpEvtType_t eLlcpEvtType;/**< LLCP events*/
}phDtaLib_uEvtType_t;

/*The events from Middleware interface. The event values shall be unique for each group*/
typedef union phDtaLib_eEvtInfo
{
    phMWIf_uEvtInfo_t      uDpEvtInfo;/**<  Digital Protocol related events */
    phMwIf_uLlcpEvtInfo_t  uLlcpEvtInfo;/**< LLCP events*/
}phDtaLib_uEvtInfo_t;

/*This data is sent from Callback function to queue which will be handled by the
 * Message handler thread*/
typedef struct phDtaLib_sQueueData
{
    phDtaLib_uEvtType_t uEvtType;
    phDtaLib_uEvtInfo_t uEvtInfo;
}phDtaLib_sQueueData_t;

extern uint8_t gx_status;

/**
 * \ingroup grp_dta_lib internal function
 * \brief Checks whether Tag is NDEF compliant or not
 *
 * This function Checks whether Tag is NDEF compliant or not.
 *
 * \param[in] psNdefDetectParams        parameters returned during NDEF tag detection
 *
 * \retval #DTASTATUS_SUCCESS           Ndef TAG detect successfully
 * \retval #DTASTATUS_FAILED            Ndef TAG detect unsuccessful.
 *
 */
extern DTASTATUS phDtaLibi_CheckNDEF(phMwIf_sNdefDetectParams_t* psNdefDetectParams);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Read NDEF data from tag
 *
 * This function Read NDEF data from NDEF compliant tag.
 *
 * \param[in] psTagOpsParams            data parameters returned/sent during tag operations
 *
 * \retval #DTASTATUS_SUCCESS           NDEF Read from tag is successful
 * \retval #DTASTATUS_FAILED            NDEF Read from tag is not successful.
 *
 */
extern DTASTATUS phDtaLibi_ReadNDEF(phMwIf_uTagOpsParams_t* psTagOpsParams);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Write NDEF data to tag
 *
 * This function Writes the NDEF data to NDEF compliant tag.
 *
 * \param[in] psTagOpsParams            data parameters returned/sent during tag operations
 *
 * \retval #DTASTATUS_SUCCESS           NDEF Write to tag is successful
 * \retval #DTASTATUS_FAILED            NDEF Write to tag is not successful.
 *
 */
extern DTASTATUS phDtaLibi_WriteNDEF(phMwIf_uTagOpsParams_t* psTagOpsParams);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Set the Tag as read only
 *
 * This function Set the Tag as read only.
 *
 * \param[in] blReadOnly                Flag to set the tag read only
 * \param[in] psTagOpsParams            data parameters returned/sent during tag operations
 *
 * \retval #DTASTATUS_SUCCESS           NDEF Write to tag is successful
 * \retval #DTASTATUS_FAILED            NDEF Write to tag is not successful.
 *
 */
extern DTASTATUS phDtaLibi_SetTagReadOnly(phMwIf_uTagOpsParams_t* psTagOpsParams);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to print the string or array or group of data
 *
 * This Function to print the string or array or group of data
 *
 * \param[in] pubuf                     8bit Buffer to be printed
 * \param[in] uilen                     length of the Buffer to be printed
 * \param[in] pcsmessage                Message to be printed before printing the Buffer
 *
 * \retval                              None
 *
 */
extern void phDtaLibi_PrintBuffer(uint8_t *pubuf,uint16_t uilen,const char *pcsmessage);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function To handle NFC_DEP listen mode test cases means LT is polling and is initiator
 *
 * Function To handle NFC_DEP listen mode test cases means LT is polling and is initiator
 *
 * \param[in]                           None
 *
 * \retval #DTASTATUS_SUCCESS           handling of NFC_DEP listen mode test cases is successful
 * \retval #DTASTATUS_FAILED            handling of NFC_DEP listen mode test cases is Unsuccessful.
 *
 */
extern DTASTATUS phDtaLibi_NfcdepInitiatorOperations();

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function To handle NFC_DEP poll mode test cases means LT is listening and is polling
 *
 * Function To handle NFC_DEP poll mode test cases means LT is listening and is polling
 *
 * \param[in] bStartDiscReqd            Flag to start the discovery
 * \param[in] bStopDiscReqd             Flag to stop the discovery
 *
 * \retval #DTASTATUS_SUCCESS           handling of NFC_DEP listen mode test cases is successful
 * \retval #DTASTATUS_FAILED            handling of NFC_DEP listen mode test cases is Unsuccessful.
 *
 */
extern DTASTATUS phDtaLibi_NfcdepTargetOperations(BOOLEAN* bStartDiscReqd,
        BOOLEAN* bStopDiscReqd);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function To Handle Host card emulation related operations
 *
 * Function To handle Host card emulation related operations
 *
 * \param[in]                           None
 *
 * \retval #DTASTATUS_SUCCESS           Handling of Host card emulation related operations is successful
 * \retval #DTASTATUS_FAILED            Handling of Host card emulation related operations is Unsuccessful.
 *
 */
extern DTASTATUS phDtaLibi_HceOperations();

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function To Handle UICC card emulation related operations
 *
 * Function To handle UICC card emulation related operations
 *
 * \param[in]                           None
 *
 * \retval #DTASTATUS_SUCCESS           Handling of Host card emulation related operations is successful
 * \retval #DTASTATUS_FAILED            Handling of Host card emulation related operations is Unsuccessful.
 *
 */
extern DTASTATUS phDtaLibi_UICCOperations();

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Handle T1T Tag operations(Read/Write)
 *
 * Function To Handle T1T Tag operations(Read/Write) based on the pattern number configured
 *
 * \param[in] sTestProfile              Tag testing required params
 *
 * \retval #DTASTATUS_SUCCESS           T1T Tag read/write operations is successful
 * \retval #DTASTATUS_FAILED            T1T Tag read/write operations is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_T1TOperations(phDtaLib_sTestProfile_t sTestProfile);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Handle T2T Tag operations(Read/Write)
 *
 * Function To Handle T2T Tag operations(Read/Write) based on the pattern number configured
 *
 * \param[in] sTestProfile              Tag testing required params
 *
 * \retval #DTASTATUS_SUCCESS           T2T Tag read/write operations is successful
 * \retval #DTASTATUS_FAILED            T2T Tag read/write operations is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_T2TOperations(phDtaLib_sTestProfile_t sTestProfile);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Handle T3T Tag operations(Read/Write)
 *
 * Function To Handle T3T Tag operations(Read/Write) based on the pattern number configured
 *
 * \param[in] sTestProfile              Tag testing required params
 * \param[in] bStartDiscReqd            Flag to start the discovery
 * \param[in] bStopDiscReqd             Flag to stop the discovery
 * \param[in] psT3TActivationParams     T3T Activation params
 *
 * \retval #DTASTATUS_SUCCESS           T3T Tag read/write operations is successful
 * \retval #DTASTATUS_FAILED            T3T Tag read/write operations is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_T3TOperations(phDtaLib_sTestProfile_t sTestProfile,
                                         phMWIf_sActivatedEvtInfo_t* psT3TActivationParams,BOOLEAN* bStartDiscReqd,
                                         BOOLEAN* bStopDiscReqd);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Handle T4T Tag operations(Read/Write)
 *
 * Function To Handle T4T Tag operations(Read/Write) based on the pattern number configured
 *
 * \param[in] sTestProfile              Tag testing required params
 * \param[in] bStartDiscReqd            Flag to start the discovery
 * \param[in] bStopDiscReqd             Flag to stop the discovery
 *
 * \retval #DTASTATUS_SUCCESS           T4T Tag read/write operations is successful
 * \retval #DTASTATUS_FAILED            T4T Tag read/write operations is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_T4TOperations(phDtaLib_sTestProfile_t sTestProfile,
                                         BOOLEAN* blStartDiscReqd,
                                         BOOLEAN* blStopDiscReqd);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to handle operations related to LLCP starts below
 *
 * Function to handle operations related to LLCP starts below
 *
 * \param[in] psTestProfile              LLCP testing required params
 * \param[in] phMWIf_eLlcpEvtType_t      LLCP events sent to user of MwIf
 * \param[in] phMwIf_uLlcpEvtInfo_t      union of all the LLCP callback parameters
 *
 * \retval #DTASTATUS_SUCCESS           handle operations related to LLCP is successful
 * \retval #DTASTATUS_FAILED            handle operations related to LLCP is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_LlcpOperations(phDtaLib_sTestProfile_t* psTestProfile,
                                          phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                          phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to handle operations related to Connection less LLCP starts below
 *
 * Function to handle operations related to Connection less LLCP starts below
 *
 * \param[in] psTestProfile              LLCP testing required params
 * \param[in] phMWIf_eLlcpEvtType_t      LLCP events sent to user of MwIf
 * \param[in] phMwIf_uLlcpEvtInfo_t      union of all the LLCP callback parameters
 *
 * \retval #DTASTATUS_SUCCESS           handle connection less operations related to LLCP is successful
 * \retval #DTASTATUS_FAILED            handle connection less operations related to LLCP is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_LlcpConnLessOperations(phDtaLib_sTestProfile_t* psTestProfile,
                                                  phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                                  phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo);

/**
 * \ingroup grp_dta_lib internal function
 * \brief Function to Operations to be done after remote client disconnects from local connection oriented server
 *
 * Function to handle Operations to be done after remote client disconnects from local connection oriented server
 *
 * \param[in] psTestProfile              LLCP testing required params
 * \param[in] phMWIf_eLlcpEvtType_t      LLCP events sent to user of MwIf
 * \param[in] phMwIf_uLlcpEvtInfo_t      union of all the LLCP callback parameters
 *
 * \retval #DTASTATUS_SUCCESS           handle connection oriented operations related to LLCP is successful
 * \retval #DTASTATUS_FAILED            handle connection oriented operations related to LLCP is Unsuccessful
 *
 */
extern DTASTATUS phDtaLibi_LlcpHandleCoClientDisconnect(phDtaLib_sTestProfile_t* psTestProfile,
                                                        phMWIf_eLlcpEvtType_t    eLlcpEvtType,
                                                        phMwIf_uLlcpEvtInfo_t*   puLlcpEvtInfo);

/**
 * \ingroup grp_dta_lib internal function
 * \brief DTA Library function to handle the call back events
 *
 * Function to handle the middleware call back events
 *
 * \param[in] mwIfHandle              middlware handler
 * \param[in] applHdl                 application handler
 * \param[in] phMWIf_eEvtType_t       Data events sent to user of MwIf
 * \param[in] phMWIf_uEvtInfo_t       Data sent to user of MwIf for callback events
 *
 * \retval                            None
 *
 */
extern void phDtaLibi_EvtCb(void* mwIfHandle,
                            void* applHdl,
                            phMWIf_eEvtType_t evtType,
                            phMWIf_uEvtInfo_t* psEvtData);

/**
 * \ingroup grp_dta_lib internal function
 * \brief DTA Library function to handle the call back events
 *
 * Function to handle the llcp middleware call back events
 *
 * \param[in] mwIfHandle              middlware handler
 * \param[in] applHdl                 application handler
 * \param[in] phMWIf_eLlcpEvtType_t   LLCP events sent to user of MwIf
 * \param[in] phMwIf_uLlcpEvtInfo_t   union of all the LLCP callback parameters
 *
 * \retval                            None
 *
 */
extern void phDtaLibi_LlcpEvtCb (void*                   pvMwIfHandle,
                                 void*                   pvApplHdl,
                                 phMWIf_eLlcpEvtType_t   eLlcpEvtType,
                                 phMwIf_uLlcpEvtInfo_t*  puLlcpEvtInfo);

#endif /*#ifndef __PH_DTATST_H__*/

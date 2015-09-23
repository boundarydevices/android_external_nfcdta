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
 * \file phDTALib.c
 *
 * Project: NFC DTA
 *
 */

/*
 ************************* Header Files ****************************************
 */

#include <utils/Log.h>
#include "data_types.h"
#include "phMwIf.h"
#include "phDTALibJNI.h"
#include "phDTALib.h"
#include "phDTAOSAL.h"
#ifdef __cplusplus
extern "C" {
#endif

#define LOG_FUNC_ENTRY phOsal_LogDebugString((const uint8_t*)"DTAJni>:Enter:",(const uint8_t*)__FUNCTION__)
#define LOG_FUNC_EXIT  phOsal_LogDebugString((const uint8_t*)"DTAJni>:Exit:",(const uint8_t*)__FUNCTION__)

/*JNI handle contains DTALib Initialization. */
static dtaJniHandle dtalibState=0x0;

/**
 * JNI Test. This will be useful to know if JNI is working or not. This is the first JNI call from UI.
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_TestJNI(
        JNIEnv *env, jobject obj, jint inter) {
    LOG_FUNC_ENTRY;

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Received Interval Value:",inter);

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Initialize DTA Lib
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibInitJNI(
        JNIEnv *env, jobject obj) {
    DTASTATUS dwDtaStatus;
    LOG_FUNC_ENTRY;
    jint  dtaJniRetVal=0;

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling DTALib Init\n");
    if(dtalibState == DTALIB_STATE_INITIALIZED)
    {
        phOsal_LogDebug((const uint8_t*)"DTAJni>DTA Already initialized\n");
        dtaJniRetVal = DTAJNISTATUS_ALREADY_INITIALIZED;
    }
    else
    {
        dwDtaStatus = phDtaLib_Init();
        if(dwDtaStatus == DTASTATUS_SUCCESS)
        {
            phOsal_LogDebug((const uint8_t*)"DTAJni>DTA initilization successfull\n");
            dtalibState = DTALIB_STATE_INITIALIZED;
            dtaJniRetVal = DTAJNISTATUS_SUCCESS;
        }
        else
        {
            phOsal_LogDebug((const uint8_t*)"Error in DTA initilization\n");
            dtaJniRetVal = DTAJNISTATUS_FAILED;
        }
    }

    LOG_FUNC_EXIT;
    return dtaJniRetVal;
}

/**
 * JNI De-Initialize DTA Lib
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibDeInitJNI(
        JNIEnv *env, jobject obj) {
    DTASTATUS dwDtaStatus;
    LOG_FUNC_ENTRY;
    jint  dtaJniRetVal=0;

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling DTALib De-Init\n");
    if(dtalibState == DTALIB_STATE_DEINITIALIZED )
    {
        phOsal_LogDebug((const uint8_t*)"DTAJni>DTA Already Deinitialized\n");
        dtaJniRetVal = DTAJNISTATUS_ALREADY_DEINITIALIZED;
    }
    else
    {
        dwDtaStatus = phDtaLib_DeInit();
        if(dwDtaStatus == DTASTATUS_SUCCESS)
        {
            phOsal_LogDebug((const uint8_t*)"DTAJni>DTA Deinitilization successfull\n");
            dtalibState = DTALIB_STATE_DEINITIALIZED;
            dtaJniRetVal = DTAJNISTATUS_SUCCESS;
        }
        else
        {
            phOsal_LogDebug((const uint8_t*)"Error in DTA Deinitilization\n");
            dtaJniRetVal = DTAJNISTATUS_FAILED;
        }
    }

    LOG_FUNC_EXIT;
    return dtaJniRetVal;
}

/**
 * JNI Enable Discovery
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibEnableDiscoveryJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibsDiscParamst) {
    phDtaLib_sDiscParams_t discParams;

    LOG_FUNC_ENTRY;
    jclass phdtaclass = env->GetObjectClass(phDtaLibsDiscParamst);

    jfieldID poll_A = env->GetFieldID(phdtaclass, "pollA", "I");
    jint pTechPol_A = env->GetIntField(phDtaLibsDiscParamst, poll_A);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Poll Tech A = :",pTechPol_A);

    jfieldID poll_B = env->GetFieldID(phdtaclass, "pollB", "I");
    jint pTechPol_B = env->GetIntField(phDtaLibsDiscParamst, poll_B);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Poll Tech B = ",pTechPol_B);

    jfieldID poll_F = env->GetFieldID(phdtaclass, "pollF", "I");
    jint pTechPol_F = env->GetIntField(phDtaLibsDiscParamst, poll_F);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Poll Tech F = ",pTechPol_F);

    jfieldID listen_A = env->GetFieldID(phdtaclass, "listenA", "I");
    jint pTechLis_A = env->GetIntField(phDtaLibsDiscParamst, listen_A);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Listen Tech A = ",pTechLis_A);

    jfieldID listen_B = env->GetFieldID(phdtaclass, "listenB", "I");
    jint pTechLis_B = env->GetIntField(phDtaLibsDiscParamst, listen_B);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Listen Tech B = ",pTechLis_B);

    jfieldID listen_F = env->GetFieldID(phdtaclass, "listenF", "I");
    jint pTechLis_F = env->GetIntField(phDtaLibsDiscParamst, listen_F);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Listen Tech F = ",pTechLis_F);

    discParams.Poll_A = pTechPol_A;
    discParams.Poll_B = pTechPol_B;
    discParams.Poll_F = pTechPol_F;

    discParams.Listen_A = pTechLis_A;
    discParams.Listen_B = pTechLis_B;
    discParams.Listen_F = pTechLis_F;

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Poll_A = ",discParams.Poll_A);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Poll_B = ",discParams.Poll_B);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Poll_F = ",discParams.Poll_F);

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Listen_A = ",discParams.Listen_A);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Listen_B = ",discParams.Listen_B);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>discParams.Listen_F = ",discParams.Listen_F);

    phOsal_LogDebug((const uint8_t*)"DTAJni> Calling Enable Discovery\n");
    phDtaLib_EnableDiscovery(&discParams);
    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Disable Discovery
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibDisableDiscoveryJNI(
        JNIEnv *env, jobject obj) {
    LOG_FUNC_ENTRY;

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling DTALib Disable Discovery\n");
    phDtaLib_DisableDiscovery();

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Initilaise NFCEE
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibInitNfceeJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibeNfceeDevTypet) {
    phDtaLib_eNfceeDevType_t devType;

    /* phDtaLibeNfceeDevTypet values should be mapped later (Check) */

    LOG_FUNC_ENTRY;
    phDtaLib_EeInit(devType);
    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Apply Required configuration needed to work with CE
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibConfigureCeJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibeCeDevTypet, uint8_t Flag) {
    jint ceType;

    LOG_FUNC_ENTRY;
    jclass phdtaclass = env->GetObjectClass(phDtaLibeCeDevTypet);

    jfieldID UICCCe = env->GetFieldID(phdtaclass, "phDtaLibUICCCe", "I");
    jint UICC_Ce = env->GetIntField(phDtaLibeCeDevTypet, UICCCe);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>UICC CE Selection Status = ",UICC_Ce);

    jfieldID SECe = env->GetFieldID(phdtaclass, "phDtaLibSECe", "I");
    jint SE_Ce = env->GetIntField(phDtaLibeCeDevTypet, SECe);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>SE CE Selection Status = ",SE_Ce);

    jfieldID eHBECe = env->GetFieldID(phdtaclass, "phDtaLibeHBECe", "I");
    jint eHBE_Ce = env->GetIntField(phDtaLibeCeDevTypet, eHBECe);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>eHBE / HCE CE Selection Status = ",eHBE_Ce);

    ceType = (UICC_Ce * PHDTALIB_UICC_CE) + (SE_Ce * PHDTALIB_SE_CE) +
             (eHBE_Ce * PHDTALIB_HOST_CE);

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>ceType.phDtaLib_UICC_Ce = ",ceType);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Device Type Flag = ",Flag);

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling DTALib Configure CE\n");
    phDtaLib_ConfigureCe((phDtaLib_eCeDevType_t)ceType, Flag);

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Apply Required configuration needed to work with LLCP & SNEP in P2P with flag
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibConfigureP2PJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibeLlcpSnepTypet, uint8_t p2pFlag) {
    jint p2pType;

    LOG_FUNC_ENTRY;
    jclass phdtaclass = env->GetObjectClass(phDtaLibeLlcpSnepTypet);

    jfieldID LLCPP2p = env->GetFieldID(phdtaclass, "phDtaLibLLCPP2p", "I");
    jint LLCP_P2p = env->GetIntField(phDtaLibeLlcpSnepTypet, LLCPP2p);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>LLCP P2P Selection Status = ",LLCP_P2p);

    jfieldID SNEPP2p = env->GetFieldID(phdtaclass, "phDtaLibSNEPP2p", "I");
    jint SNEP_P2p = env->GetIntField(phDtaLibeLlcpSnepTypet, SNEPP2p);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>SNEP P2P Selection Status = ",SNEP_P2p);

    p2pType = (LLCP_P2p * PHDTALIB_LLCP_P2P) + (SNEP_P2p * PHDTALIB_SNEP_P2P);

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>p2pType.phDtaLib_LLCP_P2P:",p2pType);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni> P2P Type Flag = ",p2pFlag);

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling DTALib Configure P2P LLCP & SENP \n");
    phDtaLib_ConfigureP2p((phDtaLib_eLlcpSnepType_t)p2pType, p2pFlag);

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Get MW, FW and DTA LIB version
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibGetIUTInfoJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibsIUTInfot) {
    phDtaLib_sIUTInfo_t IUTInfo;

    jfieldID DtaLib_VerMajor, DtaLib_VerMinor;
    jfieldID Mw_VerMajor, Mw_VerMinor;
    jfieldID Fw_VerMajor, Fw_VerMinor;

    LOG_FUNC_ENTRY;

    jclass phDtaclass = env->GetObjectClass(phDtaLibsIUTInfot);
    phOsal_LogDebug((const uint8_t*)"DTAJni>After the class  Entered:");

    phOsal_LogDebug((const uint8_t*)"DTAJni>Calling Version Info:");
    phDtaLib_GetIUTInfo(&IUTInfo);

    DtaLib_VerMajor = env->GetFieldID(phDtaclass, "DtaLib_VerMajor", "I");
    IUTInfo.DtaLib_VerMajor = env->GetIntField(phDtaLibsIUTInfot,
            DtaLib_VerMajor);
    IUTInfo.DtaLib_VerMajor = DTALIBVERSION_MAJOR;
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>DtaLib_VerMajor = ",IUTInfo.DtaLib_VerMajor);
    env->SetIntField(phDtaLibsIUTInfot, DtaLib_VerMajor,
            IUTInfo.DtaLib_VerMajor);

    DtaLib_VerMinor = env->GetFieldID(phDtaclass, "DtaLib_VerMinor", "I");
    IUTInfo.DtaLib_VerMinor = env->GetIntField(phDtaLibsIUTInfot,
            DtaLib_VerMinor);
    IUTInfo.DtaLib_VerMinor = DTALIBVERSION_MINOR;
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>DtaLib_VerMinor = ",IUTInfo.DtaLib_VerMinor);
    env->SetIntField(phDtaLibsIUTInfot, DtaLib_VerMinor,
            IUTInfo.DtaLib_VerMinor);

    Mw_VerMajor = env->GetFieldID(phDtaclass, "Mw_VerMajor", "I");

    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Mw_VerMajor = ",IUTInfo.Mw_VerMajor);
    env->SetIntField(phDtaLibsIUTInfot, Mw_VerMajor,
            IUTInfo.Mw_VerMajor);

    Mw_VerMinor = env->GetFieldID(phDtaclass, "Mw_VerMinor", "I");
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Mw_VerMinor = ",IUTInfo.Mw_VerMinor);
    env->SetIntField(phDtaLibsIUTInfot, Mw_VerMinor,
            IUTInfo.Mw_VerMinor);

    Fw_VerMajor = env->GetFieldID(phDtaclass, "Fw_VerMajor", "I");
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Fw_VerMajor = ",IUTInfo.Fw_VerMajor);
    env->SetIntField(phDtaLibsIUTInfot, Fw_VerMajor, IUTInfo.Fw_VerMajor);

    Fw_VerMinor = env->GetFieldID(phDtaclass, "Fw_VerMinor", "I");
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Fw_VerMinor = ",IUTInfo.Fw_VerMinor);
    env->SetIntField(phDtaLibsIUTInfot, Fw_VerMinor, IUTInfo.Fw_VerMinor);

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

/**
 * JNI Set Test Profile. Like pattern number, test case ID....
 */
JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibSetTestProfileJNI(
        JNIEnv *env, jobject obj, jobject phDtaLibsTestProfilet) {
    phDtaLib_sTestProfile_t TestProfile;
    LOG_FUNC_ENTRY;

    jclass phdtaclass = env->GetObjectClass(phDtaLibsTestProfilet);

    jfieldID patternNumberId = env->GetFieldID(phdtaclass, "patternnumber",
            "I");
    if (patternNumberId == NULL)
    {
        phOsal_LogDebug((const uint8_t*)"PatternNumber ID is null so RETURN");
        return 0;
    }
    jint pNumberId = env->GetIntField(phDtaLibsTestProfilet, patternNumberId);
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni>Pattern Number = ",pNumberId);

    TestProfile.Pattern_Number = pNumberId;
    phOsal_LogDebugU32h((const uint8_t*)"DTAJni> TestProfile Pattern Number = ",TestProfile.Pattern_Number);

    phOsal_LogDebug((const uint8_t*)"Calling DTALib Set Test Profile\n");
    phDtaLib_SetTestProfile(TestProfile);

    LOG_FUNC_EXIT;
    return DTASTATUS_SUCCESS;
}

#ifdef __cplusplus
}
#endif

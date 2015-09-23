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
 *
 *
 * \file  phDtaLibJNI.h
 * \brief DTA LIB Interface header file.
 *
 * Project:  NFC DTA LIB
 */
#include <jni.h>

extern "C"
{
#define DTALIBVERSION_MAJOR 6
#define DTALIBVERSION_MINOR 2

/** \ingroup grp_dta_lib
    Indicates DTA initialization status. */
#define DTAJNISTATUS_SUCCESS                  0x00
#define DTAJNISTATUS_FAILED                   0xFF
#define DTAJNISTATUS_ALREADY_INITIALIZED      100
#define DTAJNISTATUS_ALREADY_DEINITIALIZED    200
/*State of DTA Lib */
#define DTALIB_STATE_INITIALIZED   1
#define DTALIB_STATE_DEINITIALIZED 0

typedef jint dtaJniHandle;

/* Configuration for extending symbols for different platform */
#ifndef DTAJNI_LIB_EXTEND
#define DTAJNI_LIB_EXTEND extern
#endif

/**
 * \ingroup grp_dta_lib
 * \brief Initializes the DTA and NFC library
 *
 * This function initializes DTA library and NFC including underlying layers.
 * A session with NFC hardware will be established.
 *
 * \note Before successful initialization other NFC functionalities are not enabled.
 * this function doesn't initialise NFCEE.
 * \param[in]                           None
 * \retval #DTASTATUS_SUCCESS           DTA LIB Initialised successfully
 * \retval #DTASTATUS_FAILED            DTA LIB failed to initialise.
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibInitJNI(JNIEnv* e, jobject obj);

/**
 * \ingroup grp_dta_lib
 * \brief DeInitializes the DTA and NFC library
 *
 * This function DeInitializes DTA library and NFC including underlying layers.
 * A session with NFC hardware will be closed.
 *
 * \param[in]                           None
 * \retval #DTASTATUS_SUCCESS           DTA LIB De-Initialised successfully
 * \retval #DTASTATUS_FAILED            DTA LIB failed to De-initialise.
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibDeInitJNI(JNIEnv* e, jobject obj);

/**
 * \ingroup grp_dta_lib
 * \brief Initializes the DTA and NFC library
 *
 * This function initializes NFCEE(UICC or eSE) needed for DTA.
 *
 * \param[in] DevType                   NFCEE type #phDtaLib_NfceeDevType_t
 * \retval #DTASTATUS_SUCCESS           NFCEE initialised successfully
 * \retval #DTASTATUS_FAILED            DTA LIB failed to De-initialise.
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibInitNfceeJNI(JNIEnv *env, jobject obj, jobject phDtaLibeNfceeDevTypet);

/**
 * \ingroup grp_dta_lib
 * \brief Configure parameter for card emulation.
 *
 * This function prepares and configures all req parameter for card emulation.
 * This includes RF configuration and routing table configuration.
 *
 * \param[in] CeType                    Card Emulation type #phDtaLib_eCeDevType_t
 * \param[in] Flag                      Bit wise operation, one or more option can be combined
                                        DTA_DISABLE_CE          Disable Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_A    Enable Type A Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_B    Enable Type B Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_F    Enable Type F Card Emulation
 *
 * \retval #DTASTATUS_SUCCESS           CE configured successfully
 * \retval #DTASTATUS_FAILED            CE configuration failed
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibConfigureCeJNI(JNIEnv *env, jobject obj, jobject phDtaLibeCeDevTypet, uint8_t Flag);

/**
 * \ingroup grp_dta_lib
 * \brief Enable discovery(Poll + Listen)
 *
 * This function configures enables NFCC to run discovery wheel.
 *
 * \param[in] DiscMask                  Technology types for poll and listen modes #phDtaLib_DiscMask_t
                                        DiscMask = 0 (Disable Discovery)
 * \retval #DTASTATUS_SUCCESS           Discovery enabled successfully
 * \retval #DTASTATUS_FAILED            Failed to enable discovery
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibEnableDiscoveryJNI(JNIEnv *env, jobject obj, jobject phDtaLibsDiscParamst);

/**
 * \ingroup grp_dta_lib
 * \brief Disable discovery(Poll + Listen)
 *
 * This function Disables NFCC to run discovery wheel.
 *
 * \param[in] DiscMask                  Technology types for poll and listen modes #phDtaLib_DiscMask_t
                                        DiscMask = 0 (Disable Discovery)
 * \retval #DTASTATUS_SUCCESS           Discovery disabled successfully
 * \retval #DTASTATUS_FAILED            Failed to disable discovery
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibDisableDiscoveryJNI(JNIEnv *env, jobject obj);

/**
 * \ingroup grp_dta_lib
 * \brief Get IUT Information.
 *
 * This function Retrieves Version information of DTA LIB, MW and FW.
 *
 * \param[in] IUTInfo                  Pointer to #phDtaLib_IUTInfo_t, Memory shall be allocated by caller.
 * \retval #DTASTATUS_SUCCESS           IUT information retrieved successfully.
 * \retval #DTASTATUS_FAILED            Failed to get IUT information
 * \retval #DTASTATUS_INVALID_PARAM     Invalid parameter to function
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibGetIUTInfoJNI(JNIEnv *env, jobject obj, jobject phDtaLibsIUTInfot);

/**
 * \ingroup grp_dta_lib
 * \brief Set Test Profile
 *
 * This function sets the test profile to configure DTA for execution.
 *
 * \param[in] IUTInfo                  Pointer to #phDtaLib_IUTInfo_t, Memory shall be allocated by caller.
 * \retval #DTASTATUS_SUCCESS           IUT information retrieved successfully.
 * \retval #DTASTATUS_FAILED            Failed to get IUT information
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibSetTestProfileJNI(JNIEnv *env, jobject obj, jobject phDtaLibsTestProfilet);

/* P2P LLCP and SNEP*/

/**
 * \ingroup grp_dta_lib
 * \brief Configure parameter for LLCP & SENP in P2P.
 *
 * This function prepares and configures all req parameter for P2P LLCP & SNEP.
 * This includes RF configuration and routing table configuration.
 *
 * \param[in] p2pType                   P2P type #phDtaLib_eP2pDevType_t
 * \param[in] p2pFlag                   Bit wise operation, one or more option can be combined
 *
 * \retval #DTASTATUS_SUCCESS           P2P LLCP & SNEP configured successfully
 * \retval #DTASTATUS_FAILED            P2P LLCP & SNEP configuration failed
 *
 *
 */
DTAJNI_LIB_EXTEND JNIEXPORT jint JNICALL Java_com_phdtaui_helper_PhNXPJniHelper_phDtaLibConfigureP2PJNI(JNIEnv *env, jobject obj, jobject phDtaLibeLlcpSnepTypet, uint8_t p2pFlag);

};

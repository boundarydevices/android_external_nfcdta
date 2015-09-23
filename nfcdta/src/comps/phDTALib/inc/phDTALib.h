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
 * \file  phDtaLib.h
 * \brief DTA LIB Interface header file.
 *
 * Project:  NFC DTA LIB
 */

/* Preprocessor includes for different platform */
#ifdef WIN32
#include "phNfcTypes.h"
#else
#include "data_types.h"
#include <jni.h>
#endif


/* Configuration for extending symbols for different platform */
#ifndef DTALIB_LIB_EXTEND
#define DTALIB_LIB_EXTEND extern
#endif

/*Macro definition*/

/** \ingroup grp_dta_lib
    Indicates successful completion. */
#define DTASTATUS_SUCCESS               0x00
/** \ingroup grp_dta_lib
    Invalid parameter */
#define DTASTATUS_INVALID_PARAM         0x01
/** \ingroup grp_dta_lib
    Status code for failure*/
#define DTASTATUS_FAILED                0xFF


#define DTA_DISABLE_CE                  0x00
#define DTA_ENABLE_CE_TYPE_A            0x01
#define DTA_ENABLE_CE_TYPE_B            0x02
#define DTA_ENABLE_CE_TYPE_F            0x04

typedef uint32_t DTASTATUS;

/** \ingroup grp_dta_lib
    List of secure elements*/
typedef enum phDtaLib_eNfceeDevType
{
    PHDTALIB_NFCEE_UICC    = 0x00,     /**< NFCEE device type UICC*/
    PHDTALIB_NFCEE_ESE     = 0x01     /**< NFCEE device type eSE*/
}phDtaLib_eNfceeDevType_t;


/** \ingroup grp_dta_lib
    Device type where Card Emulation is present*/
typedef enum phDtaLib_eCeDevType
{
     PHDTALIB_UICC_CE  = 0x01,     /**< Card emulation at UICC*/
     PHDTALIB_SE_CE    = 0x02,     /**< Card Emulation at eSE*/
     PHDTALIB_HOST_CE  = 0x04     /**< Card Emulation at Host*/
}phDtaLib_eCeDevType_t;

/** \ingroup grp_dta_lib
     LLCP & SNEP selection in P2P*/
typedef enum phDtaLib_eLlcpSnepType
{
     PHDTALIB_LLCP_P2P  = 0x01,     /**< P2P LLCP*/
     PHDTALIB_SNEP_P2P  = 0x02      /**< P2P SNEP*/
}phDtaLib_eLlcpSnepType_t;

/** \ingroup grp_dta_lib
    RF Discovery Technology and Mode*/
typedef struct phDtaLib_sDiscParams
{
    BOOLEAN Poll_A;      /**< Flag to Enable Tech A poll*/
    BOOLEAN Poll_B;      /**< Flag to Enable Tech B poll*/
    BOOLEAN Poll_F;      /**< Flag to Enable Tech F poll*/
    BOOLEAN Listen_A;    /**< Flag to Enable Tech A Listen*/
    BOOLEAN Listen_B;    /**< Flag to Enable Tech B Listen*/
    BOOLEAN Listen_F;    /**< Flag to Enable Tech F Listen*/
    /*RFU*/
}phDtaLib_sDiscParams_t;

typedef struct phDtaLib_sIUTInfo
{
    uint8_t DtaLib_VerMajor;    /**< Major version of DTA Lib*/
    uint8_t DtaLib_VerMinor;    /**< Minor version of DTA Lib*/
    uint8_t Mw_VerMajor;        /**< Major version of NFC MW library*/
    uint8_t Mw_VerMinor;        /**< Minor version of NFC MW library*/
    uint8_t Fw_VerMajor;        /**< Major version of NFC FW*/
    uint8_t Fw_VerMinor;        /**< Minor version of NFC FW*/
}phDtaLib_sIUTInfo_t, *pphDtaLib_sIUTInfo_t;

typedef struct phDtaLib_sTestProfile
{
    uint32_t Pattern_Number;    /**< Pattern Number*/
    /*RFU*/         /**< Test Case ID and other parameters in Auto Mode*/
}phDtaLib_sTestProfile_t;

#ifdef __cplusplus
extern "C" {
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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_Init();

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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_DeInit();

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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_EeInit(phDtaLib_eNfceeDevType_t DevType);

/**
 * \ingroup grp_dta_lib
 * \brief Configure parameter for card emulation.
 *
 * This function prepares and configures all req parameter for card emulation.
 * This includes RF configuration and routing table configuration.
 *
 * \param[in] CeTpye                    Card Emulation type #phDtaLib_eCeDevType_t
 * \param[in] Flag                      Bit wise operation, one or more option can be combined
                                        DTA_DISABLE_CE          Disable Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_A    Enable Type A Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_B    Enable Type B Card Emulation
 *                                      DTA_ENABLE_CE_TYPE_F    Enable Type F Card Emulation
 *
 * \retval #DTASTATUS_SUCCESS           CE configured successfully
 * \retval #DTASTATUS_FAILED            CE configuration failed
 *
 *
 */
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_ConfigureCe(phDtaLib_eCeDevType_t CeDevType, uint8_t Flag);

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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_EnableDiscovery(phDtaLib_sDiscParams_t* DiscParams);

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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_DisableDiscovery();

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
 *
 */
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_GetIUTInfo(phDtaLib_sIUTInfo_t* psIUTInfo);

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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_SetTestProfile(phDtaLib_sTestProfile_t TestProfile);



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
DTALIB_LIB_EXTEND DTASTATUS phDtaLib_ConfigureP2p(phDtaLib_eLlcpSnepType_t p2pType, uint8_t p2pFlag);



#ifdef __cplusplus
}
#endif

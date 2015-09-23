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

package com.phdtaui.structure;

public class PhDtaLibStructure {

    public  int DtaLib_VerMajor, DtaLib_VerMinor;
    public  int Mw_VerMajor,Mw_VerMinor;
    public  int Fw_VerMajor,Fw_VerMinor;
    /**
     * UICC, HCE and SE CE dev type
     */
    public int phDtaLibUICCCe;
    public int phDtaLibSECe;
    public int phDtaLibeHBECe;

    /**
     * P2P in LLCP & SNEP selecions
     */
    public int phDtaLibLLCPP2p;
    public int phDtaLibSNEPP2p;

    /**
     * pattern number
     */
    public int patternnumber;
    /**
     * manual mode pool
     */
    public int pollA;
    public int pollB;
    public int pollF;

    /**
     * Listen mode pool
     */
    public int listenA;
    public int listenB;
    public int listenF;
    public int getDtaLib_VerMajor() {
        return DtaLib_VerMajor;
    }
    public void setDtaLib_VerMajor(int dtaLib_VerMajor) {
        DtaLib_VerMajor = dtaLib_VerMajor;
    }
    public int getDtaLib_VerMinor() {
        return DtaLib_VerMinor;
    }
    public void setDtaLib_VerMinor(int dtaLib_VerMinor) {
        DtaLib_VerMinor = dtaLib_VerMinor;
    }
    public int getLibNfc_VerMajor() {
        return Mw_VerMajor;
    }
    public void setLibNfc_VerMajor(int libNfc_VerMajor) {
        Mw_VerMajor = libNfc_VerMajor;
    }
    public int getLibNfc_VerMinor() {
        return Mw_VerMinor;
    }
    public void setLibNfc_VerMinor(int libNfc_VerMinor) {
    Mw_VerMinor = libNfc_VerMinor;
    }
    public int getFw_VerMajor() {
        return Fw_VerMajor;
    }
    public void setFw_VerMajor(int fw_VerMajor) {
        Fw_VerMajor = fw_VerMajor;
    }
    public int getFw_VerMinor() {
        return Fw_VerMinor;
    }
    public void setFw_VerMinor(int fw_VerMinor) {
        Fw_VerMinor = fw_VerMinor;
    }
}

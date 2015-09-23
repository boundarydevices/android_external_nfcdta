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

package com.phdtaui.helper;

import android.util.Log;
import android.os.DeadObjectException;
import com.phdtaui.mainactivity.PhDTAUIMainActivity;
import com.phdtaui.structure.PhDtaLibStructure;
import com.phdtaui.utils.PhUtilities;

public class PhNXPJniHelper {

    public native int TestJNI(int param);
    public native int phDtaLibInitJNI();
    public native int phDtaLibDisableDiscoveryJNI();

    public native int phDtaLibDeInitJNI();

    public native int phDtaLibEnableDiscoveryJNI(
        PhDtaLibStructure phDtaLibStructure);

    public native int phDtaLibSetTestProfileJNI(
        PhDtaLibStructure phDtaLibStructure);

    public native int phDtaLibConfigureP2PJNI(
        PhDtaLibStructure phDtaLibStructure, int enableDisableFlag);

    public native int phDtaLibConfigureCeJNI(
        PhDtaLibStructure phDtaLibStructure, int enableDisableFlag);

    public native int phDtaLibGetIUTInfoJNI(PhDtaLibStructure phDtaLibStructure);


    static {
        try {
            if (PhUtilities.NDK_IMPLEMENTATION) {
                System.loadLibrary("dta_jni");

                Log.d(PhUtilities.UI_TAG, "Initializing System Load Library");
            } else {
                Log.d(PhUtilities.UI_TAG, "NDK is Disabled");
            }
        } catch (UnsatisfiedLinkError ule) {
            /**
             * UnstaisfiedLinkErrorhandled here
             */
            PhUtilities.NDK_IMPLEMENTATION = false;
            Log.d(PhUtilities.UI_TAG,
                    " Could not load native library: " + ule.getMessage());
        } catch (NoClassDefFoundError error) {
            /**
             * NoClassDefFoundError Path mismatch error
             */

            PhUtilities.NDK_IMPLEMENTATION = false;
            Log.d(PhUtilities.UI_TAG, error.getMessage());
        } catch (NoSuchMethodError e) {
            /**
             * NoSuchMethodError Library not loaded correctly
             */
            Log.d(PhUtilities.UI_TAG, e.getMessage());
        }
    }
    public void nativeiInitEnableDeviceInfo(int enableCEFlag ,int enableP2PFlag,int mPatternNumber) {

        /**
         * Enable Discovery set Test Profile
         */
        PhDtaLibStructure phDtaLibStructure = new PhDtaLibStructure();
        phDtaLibStructure.patternnumber = mPatternNumber;

        Log.d(PhUtilities.UI_TAG, "patternnumber "
                + phDtaLibStructure.patternnumber);

        phDtaLibSetTestProfileJNI(phDtaLibStructure);

        /**
         * Check Card Emulation Device type
         */
        phDtaLibStructure=PhDTAUIMainActivity.settingDeviceType();

        Log.d(PhUtilities.UI_TAG, "UICC " + phDtaLibStructure.phDtaLibUICCCe);
        Log.d(PhUtilities.UI_TAG, "SECE" + phDtaLibStructure.phDtaLibSECe);
        Log.d(PhUtilities.UI_TAG, "HBE/HCE "
                + phDtaLibStructure.phDtaLibeHBECe);

        phDtaLibConfigureCeJNI(phDtaLibStructure, enableCEFlag);

        /**
         * Check P2P type
         */
        phDtaLibStructure=PhDTAUIMainActivity.settingP2PFlag();

        Log.d(PhUtilities.UI_TAG, "LLCP " + phDtaLibStructure.phDtaLibLLCPP2p);
        Log.d(PhUtilities.UI_TAG, "SNEP " + phDtaLibStructure.phDtaLibSNEPP2p);
        phDtaLibConfigureP2PJNI(phDtaLibStructure, enableP2PFlag);
        /**
         * Enable Discovery structure
         */
        phDtaLibStructure=PhDTAUIMainActivity.settingPollListen();

        Log.d(PhUtilities.UI_TAG, "Poll_A " + phDtaLibStructure.pollA);
        Log.d(PhUtilities.UI_TAG, "Poll_B " + phDtaLibStructure.pollB);
        Log.d(PhUtilities.UI_TAG, "Poll_F " + phDtaLibStructure.pollF);
        Log.d(PhUtilities.UI_TAG, "listen_A " + phDtaLibStructure.listenA);
        Log.d(PhUtilities.UI_TAG, "listen_B " + phDtaLibStructure.listenB);
        Log.d(PhUtilities.UI_TAG, "listen_F " + phDtaLibStructure.listenF);

        phDtaLibEnableDiscoveryJNI(phDtaLibStructure);
    }

    public PhDtaLibStructure versionLib(){
        PhDtaLibStructure phDtaLibStructure=new PhDtaLibStructure();
        phDtaLibGetIUTInfoJNI(phDtaLibStructure);
        phDtaLibStructure.setDtaLib_VerMajor(phDtaLibStructure.DtaLib_VerMajor);
        phDtaLibStructure.setDtaLib_VerMinor(phDtaLibStructure.DtaLib_VerMinor);
        phDtaLibStructure.setLibNfc_VerMajor(phDtaLibStructure.Mw_VerMajor);
        phDtaLibStructure.setLibNfc_VerMinor(phDtaLibStructure.Mw_VerMinor);
        phDtaLibStructure.setFw_VerMajor(phDtaLibStructure.Fw_VerMajor);
        phDtaLibStructure.setFw_VerMinor(phDtaLibStructure.Fw_VerMinor);
    return phDtaLibStructure;
    }

    public int phDtaLibDeInit(){
        return phDtaLibDeInitJNI();
    }
    public int phDtaLibDisableDiscovery(){
        return phDtaLibDisableDiscoveryJNI();
    }

    public int startPhDtaLibInit() {
        return phDtaLibInitJNI();
    }
    public int startTest() {
        return TestJNI(10);
    }
}

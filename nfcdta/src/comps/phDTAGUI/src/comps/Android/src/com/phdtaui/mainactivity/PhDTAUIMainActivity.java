/**
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

package com.phdtaui.mainactivity;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.TimeUnit;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.nfc.NfcAdapter;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.text.Editable;
import android.text.Html;
import android.text.Selection;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import com.phdtaui.asynctask.PhNioClientTask;
import com.phdtaui.asynctask.PhNioServerTask;
import com.phdtaui.customdialogs.PhCustomExitDialog;
import com.phdtaui.customdialogs.PhCustomIPDialog;
import com.phdtaui.customdialogs.PhCustomLibDialog;
import com.phdtaui.customdialogs.PhCustomMSGDialog;
import com.phdtaui.customdialogs.PhCustomPopValidation;
import com.phdtaui.customdialogs.PhCustomSNEPRTD;
import com.phdtaui.customdialogs.PhShowMSGDialog;
import com.phdtaui.helper.PhNXPHelperMainActivity;
import com.phdtaui.helper.PhNXPJniHelper;
import com.phdtaui.structure.PhDtaLibStructure;
import com.phdtaui.utils.PhUtilities;
import android.view.WindowManager;

public class PhDTAUIMainActivity extends Activity implements OnClickListener,
        android.widget.RadioGroup.OnCheckedChangeListener,OnCheckedChangeListener,
        OnItemSelectedListener {

    private PhCustomPopValidation customPopValidation = null;
    private Spinner multitextSpinner, deviceSelectorSpinner;
    private Button runBtn, stopBtn, exitBtn;
    public static volatile TextView tcpIPTextView;
    private TextView statusTextView, versionNumber, copyRightsTextView,
            vPollTextView, vListenTextView,
            logMessageTextView, deviceTextView, deviceSelectionTextView,
            firmWareVersionTextView,p2pTextView;
    private boolean onClickColoringRunning = false, checkNfcServiceDialog = false , isRunClickedAtlLeastOnce = false;
    private EditText logToFileEditText, mPatternNumberEditText;
    public static CheckBox radioButtonPollA, radioButtonPollB,
            radioButtonPollF, radioButtonPollV, radioButtonListenA,
            radioButtonListenB, radioButtonListenF, radioButtonListenV, clearP2PCheckBox;
    public static RadioButton autoRadioButton, manualRadioButton,
            uiccCheckBox, hceCheckBox, llcpCheckBox, snepCheckBox,
            depiCheckBox, deptCheckBox, eseCheckBox;
    private CheckBox logcatCheckBox, versionnumberCheckBox, customMessage,
            showMessage, logToFileCheckBox, cardEmulationTextView;
    private boolean cePatternEnable;
    private String fileName;

    private boolean checkedButtonPollA = true, checkedButtonPollB = true, checkedButtonPollF = true ,
                    checkedButtonPollV = true, checkedButtonListenA = true , checkedButtonListenB = true ,
                    checkedButtonListenF = true , checkedButtonListenV = true;

    public PhDtaLibStructure phDtaLibStructure;
    private String currentStatusStopped = "Current Status: <font color='red'> Stopped</font>" ,
                   currentStatusRunning = "Current Status: <font color='#006600'> Running</font>" ,
                   currentStatusInitializing = "Current Status: <font color='#006600'> Initializing</font>";

    private PhNXPHelperMainActivity nxpHelperMainActivity = new PhNXPHelperMainActivity();

    private NfcAdapter mNfcAdapter;

    public int enableCEFlag, enableP2PFlag , mPatternNumber;

    public String sPatternNumber , versionName;

    public static String errorMsg;

    private static boolean errorPopUP;

    private RadioGroup autoManualRadioGroup,cardEmulationRadioGroup, llsnepradiogroup;

    /**
     * Inet Address and port number for Wireless connection between DUT and LT
     */
    public static InetAddress inetAddress;

    public static int portNumber;

    /**
     * TO know whether Run button or Stop button is clicked
     */
    PhNXPJniHelper phNXPJniHelper = new PhNXPJniHelper();
    ProgressDialog clientConnectionProgress;
    /**
     * Load Library
     */

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
            errorPopUP = true;
            errorMsg = "NO_LIBRARY_FILE";
            PhUtilities.NDK_IMPLEMENTATION = false;
            Log.d(PhUtilities.UI_TAG,
                    " Could not load native library: " + ule.getMessage());
        } catch (NoClassDefFoundError error) {
            /**
             * NoClassDefFoundError Path mismatch error
             */

            errorMsg = "PATH_MISMATCH";
            errorPopUP = true;
            PhUtilities.NDK_IMPLEMENTATION = false;
            Log.d(PhUtilities.UI_TAG, error.getMessage());
        } catch (NoSuchMethodError e) {
            /**
             * NoSuchMethodError Library not loaded correctly
             */
            Log.d(PhUtilities.UI_TAG, e.getMessage());
        }
    }

    /**
     * Loading the .so file from library
     */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /**
         * Loading the layout for displaying the user interface
         */
        setContentView(R.layout.ph_nxpdta_layout);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setFindViewByID();
        /**
         * Error Pop up start /
         */
        mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
        if (errorPopUP) {
            PhCustomLibDialog customLibDialog = new PhCustomLibDialog(
                    PhDTAUIMainActivity.this, errorMsg);
            customLibDialog.show();
        }

        PhUtilities.exitValue = "exit not clicked";
        // START - get version number from manifest file
        try {
            versionName = getPackageManager().getPackageInfo(getPackageName(),
                    0).versionName;
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
     //   versionNumber.setText("UI Version: " + "05.05");
        // END - get version number from manifest file

        // START - enabling text box on check of file to log
        nfccheck();
        if (PhUtilities.NDK_IMPLEMENTATION && !(mNfcAdapter.isEnabled())) {
            clientConnectionProgress = ProgressDialog.show(PhDTAUIMainActivity.this, "", "Loading Please Wait..", true, true);
            clientConnectionProgress.setCancelable(false);
            new Thread(){
                public void run() {
                    super.run();
                    try{
                       if(phNXPJniHelper.startPhDtaLibInit()==0){
                       Log.e(PhUtilities.UI_TAG, "DTA is Initializing");
                    }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                        clientConnectionProgress.dismiss();
                };
            }.start();
        }
        statusTextView.setText(Html.fromHtml(currentStatusStopped),
                TextView.BufferType.SPANNABLE);

        /**
         * TCP Connection status
         */
        tcpIPTextView.setText(PhUtilities.TCPIPConnectionStateText);
        /**
         * Auto Manual Radio Group Check Change Listener
         */
        autoManualRadioGroup.setOnCheckedChangeListener(this);
        cardEmulationTextView.setOnCheckedChangeListener(this);

        /**
         * LLCP and SNEP Radio Button Group - onCheckedChangeListener
         */
        llsnepradiogroup.setOnCheckedChangeListener(this);

        /**
         * SNEP on click listner for RTD pop up
         */
        snepCheckBox.setOnClickListener(this);

        firmWareVersionTextView.setText(getResources().getString(
                R.string.firmare_version));

        /**
         * Enabling and Setting all the show message and custom message checked
         * is false in manual mode
         */
        radioButtonPollV.setEnabled(false);
        radioButtonListenV.setEnabled(false);
        selectionMode(false);
        customMessage.setChecked(false);
        showMessage.setChecked(false);
        logToFileEditText.setEnabled(true);

        /**
         * On Click listener for all the widget
         */
        onClickEventForAllTheViews();
        /**
         * check weather logToFileCheckBox is check or not checked
         */

        logToFileCheckBox.setOnCheckedChangeListener(this);
        // END - enabling text box on check of file to log

        clearP2PCheckBox.setOnCheckedChangeListener(this);
        // START - Display Copy Rights symbol
        String copyRightsSymbol = "Copyright NXP Semiconductors";
        copyRightsTextView.setText(copyRightsSymbol);
        // END - Display Copy Rights symbol

        logcatCheckBox.setChecked(false);
        // START - change intent to show Log Cat or Console

        /** check weather logcatCheckBox is check or not checked */

        logcatCheckBox.setOnCheckedChangeListener(this);

        // END - change intent to show Log Cat or Console

        mPatternNumberEditText.setEnabled(true);
        multitextSpinner.setEnabled(true);
        multitextSpinner.setOnItemSelectedListener(PhDTAUIMainActivity.this);

        /**
         * Set the adapter for the device spinner
         */

        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                PhDTAUIMainActivity.this, R.array.device_selection_array,
                android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        deviceSelectorSpinner.setAdapter(adapter);
        // deviceSelectorSpinner
        // .setOnItemSelectedListener(PhDTAUIMainActivity.this);

//        new UITask().execute();
        runBtn.setOnClickListener(this);
        stopBtn.setOnClickListener(this);
        exitBtn.setOnClickListener(this);

        /**
         * Get pattern number from user
         */
        mPatternNumberEditText.addTextChangedListener(new MaxLengthWatcher(4));
        /**
         *
         * APPEAR DISAPPEAR AND FADDING
         */
        /**
         * Poll and listen mode selection defaultly
         */
        radioButtonListenA.setChecked(true);
        radioButtonListenB.setChecked(true);
        radioButtonListenF.setChecked(true);
        radioButtonPollA.setChecked(true);
        radioButtonPollB.setChecked(true);
        radioButtonPollF.setChecked(true);

        /** Appear disappear for Custom */
        appearDissappearOFViews();

    }

    @Override
    public void onResume() {
        super.onResume();
        if(!mNfcAdapter.isEnabled()){
            try {
                PhCustomSNEPRTD.SERVER_STATE=false;
                PhCustomSNEPRTD.CLIENT_STATE=false;
                PhCustomSNEPRTD.runServerButton.setVisibility(View.VISIBLE);
                PhCustomSNEPRTD.stopServerButton.setVisibility(View.INVISIBLE);
                PhCustomSNEPRTD.runClientButton.setVisibility(View.VISIBLE);
                PhCustomSNEPRTD.stopClientButton.setVisibility(View.INVISIBLE);
                PhCustomSNEPRTD.testCasesSpinner.setSelection(0);
                PhCustomSNEPRTD.runClientButton.setEnabled(false);
            } catch (NullPointerException e) {
                e.printStackTrace();
            }
        }
        logToFileEditText.setFocusable(false);
        if (PhUtilities.exitValue.equals("exit clicked")) {
            Toast.makeText(getApplicationContext(), PhUtilities.exitValue,
                    Toast.LENGTH_SHORT).show();
            finish();
        }

    }

    /** Onclick actions for all the widget */

    @Override
    public void onClick(View v) {

        switch (v.getId()) {
        /** Run button onclick actions for all the widget */

        case R.id.run:
            mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
            boolean technologySelectionerror = radioButtonListenA.isChecked() || radioButtonListenB.isChecked() || radioButtonListenF.isChecked()
                                      || radioButtonPollA.isChecked() || radioButtonPollB.isChecked() || radioButtonPollF.isChecked();
            if(!mNfcAdapter.isEnabled() && technologySelectionerror){
                runBtn.setClickable(false);
                statusTextView.setText(Html.fromHtml(currentStatusInitializing),TextView.BufferType.SPANNABLE);
                runBtn.setBackgroundColor(Color.parseColor(getResources().getString(R.color.grey)));
                statusTextView.setText(Html.fromHtml(currentStatusInitializing), TextView.BufferType.SPANNABLE);
                Log.e(PhUtilities.UI_TAG, "runButton clicked");
            /*
             * statusTextView.setText(Html.fromHtml(currentStatusRunning),
             * TextView.BufferType.SPANNABLE);
             */
//            statusTextView.post(new Runnable() {
//                public void run() {
                    if(!PhUtilities.isServerRunning){
                        /*AlertDialog.Builder alertDialog = new AlertDialog.Builder(PhDTAUIMainActivity.this);
                        alertDialog.setTitle("Client Message");
                        alertDialog.setMessage("Server is not running");
                        alertDialog.setPositiveButton("Retry", new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                Log.d(PhUtilities.TCPSRV_TAG, "trying to reconnect");

                            }
                        });
                        alertDialog.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.cancel();

                            }
                        });
                        alertDialog.show(); */
                    }
                    clientImplementation();
//                }
//            });
            autoRadioButton.setEnabled(false);
            manualRadioButton.setEnabled(false);
            cardEmulationTextView.setEnabled(false);

            /**
             * Console check weather it is checked
             */
            if (logcatCheckBox.isChecked()) {
                Intent intent = new Intent(getBaseContext(),
                        PhLogCatActivity.class);
                startActivity(intent);
            }
            if (logToFileCheckBox.isChecked()) {
                fileName = logToFileEditText.getText().toString();
                try {
                    Log.d(PhUtilities.UI_TAG,
                            "Inside Run, Creating file and writing log to it .... ");
                    nxpHelperMainActivity.createFileWithGivenNameOnExternalStorageAndWriteLog(fileName
                                    + ".txt");
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            /**
             * Creating files based on conditions checked
             */
            if (PhUtilities.LOG_DUT_MESSAGES) {
                try {
                    if (!PhUtilities.FILE_NAME_DUT.equals("")) {
                        Log.e(PhUtilities.UI_TAG, "Creating DUT file");
                        nxpHelperMainActivity
                                .createFileWithGivenNameOnsdCard(PhUtilities.FILE_NAME_DUT);
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (PhUtilities.LOG_LT_MESSAGES) {
                try {
                    if (!PhUtilities.FILE_NAME_LT.equals("")) {
                        Log.e(PhUtilities.UI_TAG, "Crating LT file");
                        nxpHelperMainActivity.createFileWithGivenNameOnsdCard(PhUtilities.FILE_NAME_LT);
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            /** Need To add once so file is changed */
            /**
             * NativeUtilities nativeUtilities = new
             * NativeUtilities();nativeUtilities.ndkImplementation();
             */

            /*** Need To remove once so file is changed */

            /*** NDK implementation */
            // loadJNILibrary();

            isRunClickedAtlLeastOnce = true;

            if (PhUtilities.NDK_IMPLEMENTATION) {
                // SOM
                // 4. Creating background Async Task for start dta
                // dtaOperationStartTask = new DtaOperationStart();
                // 5. Creating watchdog timer object
                /** check auto mode or manual mode */
                if (manualRadioButton.isChecked()) {
                    Log.d(PhUtilities.UI_TAG, "Looping into Manual Mode");
                    // Get pattern number from EditText
                    String getPatternNoEditText = mPatternNumberEditText
                            .getText().toString().trim();
                    if (!mPatternNumberEditText.getText().toString().isEmpty()) {
                        sPatternNumber = getPatternNoEditText;
                    }
                    // Get pattern number from Spinner
                    else {
                        sPatternNumber = nxpHelperMainActivity
                                .getsPatternnumber();
                        Log.d(PhUtilities.UI_TAG,
                                "Get pattern number from spinner"
                                        + sPatternNumber);

                    }

                    // Check for CE Enable
                    if (uiccCheckBox.isChecked() || hceCheckBox.isChecked() || eseCheckBox.isChecked()) {
                        cePatternEnable = true;

                    } else {
                        cePatternEnable = false;
                    }

                } else if (autoRadioButton.isChecked()) {
                    automodeOnRunButtonPressed();
                    }
            }

                // Common Code for both Auto & Manual Modes

                try {
                    // Converting String Pattern Number to Hex
                    Log.d(PhUtilities.UI_TAG, "Entered Pattern Number: "
                            + sPatternNumber);
                    mPatternNumber = Integer.parseInt(sPatternNumber, 16);
                    // mPatternNumber = Integer.parseInt(sPatternNumber, 16);
                    // String getHexValue = Integer.toHexString(mPatternNumber);

                    // Log.d(PhUtilities.UI_TAG, getHexValue);
                    boolean patternNumberValidation=((mPatternNumber == 0x0000)|| (mPatternNumber == 0x0001)
                            || (mPatternNumber == 0x0002)|| (mPatternNumber == 0x0003)|| (mPatternNumber == 0x0004)
                            || (mPatternNumber == 0x0005)|| (mPatternNumber == 0x0006)|| (mPatternNumber == 0x0007)
                            || (mPatternNumber == 0x0008)|| (mPatternNumber == 0x0009)|| (mPatternNumber == 0x000A)
                            || (mPatternNumber == 0x000B)|| (mPatternNumber == 0x000C)|| (mPatternNumber == 0x000D)
                            || (mPatternNumber == 0x000E)|| (mPatternNumber == 0x000F) || (mPatternNumber == 0x1000))&& (!cePatternEnable);
                    if (patternNumberValidation) {
                        Log.d(PhUtilities.UI_TAG,
                                "After Converted String to Integer "
                                        + mPatternNumber);
                        Log.d(PhUtilities.UI_TAG,
                                "After Converted String to Integer Correct Pattern Number without Card Emulation");
                    } else if (((mPatternNumber == 0x0000) || (mPatternNumber == 0x1000))
                            && (cePatternEnable)) {
                        Log.d(PhUtilities.UI_TAG,
                                "After Converted String to Integer"
                                        + mPatternNumber);
                        Log.d(PhUtilities.UI_TAG,
                                "After Converted String to Integer Correct Pattern Number with Card Emulation");
                    }
                } catch (NumberFormatException e) {

                    Log.d(PhUtilities.UI_TAG, "Number format exception"
                            + mPatternNumberEditText.getText().toString());
                    Log.d(PhUtilities.UI_TAG, "Ce Pattern Enable boolean value"
                            + cePatternEnable);
                }
            runBtn.setClickable(false);
            try {
                enableDiscovery();
            } catch (Exception e) {
                e.printStackTrace();
            }

            onClickColoringRunning = true;
            disableAllOtherBtns();
            tcpIPTextView.setText(PhUtilities.TCPIPConnectionStateText);
            stopBtn.setBackgroundColor(Color.parseColor(getResources()
                    .getString(R.color.red)));
            exitBtn.setBackgroundColor(Color.parseColor(getResources()
                    .getString(R.color.grey)));
            }else{
                  technologyError(getResources().getString(R.string.technology_error));
            }
                nfccheck();

            break;
        /** Stop button onclick actions for all the widget */
        case R.id.stop:
            runBtn.setClickable(true);
            Log.e(PhUtilities.UI_TAG, "stop clicked");
            statusTextView.setText(Html.fromHtml(currentStatusStopped),
                    TextView.BufferType.SPANNABLE);

            /**
             * Mode check done here
             */
            if (autoRadioButton.isChecked()) {
                autoModeisON();
            } else if (manualRadioButton.isChecked()) {
                manualModeisON();
            }

            if(llcpCheckBox.isChecked()){
                mPatternNumberEditText.setEnabled(false);
                cardEmulationTextView.setChecked(false);
            }else if(snepCheckBox.isChecked()){
            mPatternNumberEditText.setEnabled(false);
                cardEmulationTextView.setChecked(false);
                multitextSpinner.setEnabled(false);
            }

            /**
             * Change button background colour to user actions
             */
            if (onClickColoringRunning) {
                stopBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.grey)));
                runBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.green)));
                runBtn.setEnabled(true);
                exitBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.orange)));
                onClickColoringRunning = false;
                multitextSpinner.setSelection(0);
                reLaunchMainActivity();

            } else {
                runBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.green)));
                runBtn.setEnabled(true);
                stopBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.grey)));
                exitBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.orange)));
                Toast.makeText(getApplicationContext(), "Run before Stop",
                        Toast.LENGTH_LONG).show();
            }

            // This should be done only for auto mode// SOM
            PhUtilities.SIMULATED_SERVER_SELECTED = false;
            PhUtilities.CLIENT_SELECTED = false;
            PhUtilities.SERVER_SELECTED = false;
            PhUtilities.SIMULATED_CLIENT_SELECTED = false;

            break;
        /** Exit button onclick actions for all the widget */
        case R.id.exit_btn:
            if (onClickColoringRunning) {
                exitBtn.setBackgroundColor(Color.parseColor(getResources()
                        .getString(R.color.grey)));// Can be removed
                runBtn.setClickable(false);
                Toast.makeText(getApplicationContext(),
                        "Stop the application then exit", Toast.LENGTH_LONG)
                        .show();
            } else {
                /**
                 * Exit pop up will open need to close the application or not
                 */
                PhCustomExitDialog customExitDialog = new PhCustomExitDialog(
                        this,"Confirm Exit?",false);
                runBtn.setClickable(true);
                customExitDialog.show();

            }

            break;

        /***
         * Exit Check Box checked or not actions for all the Check box starts
         * here
         */
        case R.id.a_poll_btn:

            if (!checkedButtonPollA) {
                radioButtonPollA.setChecked(true);

                checkedButtonPollA = true;
            } else if (checkedButtonPollA) {
                checkedButtonPollA = false;
                radioButtonPollA.setChecked(false);
            }
            break;
        case R.id.b_poll_btn:
            if (!checkedButtonPollB) {
                radioButtonPollB.setChecked(true);
                checkedButtonPollB = true;
            } else if (checkedButtonPollB) {
                checkedButtonPollB = false;
                radioButtonPollB.setChecked(false);
            }
            break;
        case R.id.f_poll_btn:
            if (!checkedButtonPollF) {
                radioButtonPollF.setChecked(true);
                checkedButtonPollF = true;
            } else if (checkedButtonPollF) {
                checkedButtonPollF = false;
                radioButtonPollF.setChecked(false);
            }
            break;
        case R.id.v_poll_btn:
            if (!checkedButtonPollV) {
                radioButtonPollV.setChecked(true);
                checkedButtonPollV = true;
            } else if (checkedButtonPollV) {
                checkedButtonPollV = false;
                radioButtonPollV.setChecked(false);
            }
            break;
        case R.id.a_listen_btn:
            if (!checkedButtonListenA) {
                radioButtonListenA.setChecked(true);
                checkedButtonListenA = true;
            } else if (checkedButtonListenA) {
                checkedButtonListenA = false;
                radioButtonListenA.setChecked(false);
            }
            break;
        case R.id.b_listen_btn:
            if (!checkedButtonListenB) {
                radioButtonListenB.setChecked(true);
                checkedButtonListenB = true;
            } else if (checkedButtonListenB) {
                checkedButtonListenB = false;
                radioButtonListenB.setChecked(false);
            }
            break;
        case R.id.f_listen_btn:
            if (!checkedButtonListenF) {
                radioButtonListenF.setChecked(true);
                checkedButtonListenF = true;
            } else if (checkedButtonListenF) {
                checkedButtonListenF = false;
                radioButtonListenF.setChecked(false);
            }
            break;
        case R.id.v_listen_btn:
            if (!checkedButtonListenV) {
                radioButtonListenV.setChecked(true);
                checkedButtonListenV = true;
            } else if (checkedButtonListenV) {
                checkedButtonListenV = false;
                radioButtonListenV.setChecked(false);
            }
            break;
        /***
         * Exit Check Box checked or not actions for all the Check box ends here
         */

        /** Create the log file with user entered name function here */
        case R.id.logtofiletext:

            logToFileEditText.setFocusable(true);
            logToFileEditText.setFocusableInTouchMode(true);
            logToFileEditText.setEnabled(true);

            /**
             * Log File Edit Text Validation should be done here if Edit Text
             * empty check box is disabled if not it will be enable
             */
            logToFileEditText.addTextChangedListener(new TextWatcher() {
                @Override
                public void afterTextChanged(Editable editText) {
                }

                @Override
                public void beforeTextChanged(CharSequence arg0, int arg1,
                        int arg2, int arg3) {
                }

                @Override
                public void onTextChanged(CharSequence textInEditBox, int arg1,
                        int arg2, int count) {
                    logToFileCheckBox = (CheckBox) findViewById(R.id.logtofile);
                    logToFileEditText = (EditText) findViewById(R.id.logtofiletext);
                    if (textInEditBox.length() < 1) {
                        Toast.makeText(getBaseContext(),
                                "Enter File Name To Store Log", 3).show();

                        logToFileCheckBox.setEnabled(false);
                        logToFileCheckBox.setFocusable(false);
                        logToFileCheckBox.setChecked(false);
                    } else {
                        logToFileCheckBox.setEnabled(true);
                    }
                }
            });
            break;

        case R.id.logtofile:
            break;

        /** Show the Custom alert dialog for Custom Message */
        case R.id.custom_msg:

            /**
             * Open the Custom Message Pop Up
             */
            if (customMessage.isChecked()) {
                PhCustomMSGDialog customMSGDialog = new PhCustomMSGDialog(this);
                customMSGDialog.show();
            }

            break;
        /** Show the Custom alert dialog for Show Message */
        case R.id.show_msg:
            /**
             * Open the Custom Message Pop Up
             */
            if (showMessage.isChecked()) {
                PhShowMSGDialog showMSGDialog = new PhShowMSGDialog(this);
                showMSGDialog.show();
            }

            break;
        /*** version number update if info check box is enabled */
        case R.id.versionnumber:
            /**
             * Set Version Number from DTA Library
             */
            if (versionnumberCheckBox.isChecked()) {
                if (PhUtilities.NDK_IMPLEMENTATION) {
                    phDtaLibStructure = phNXPJniHelper.versionLib();

                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getDtaLib_VerMajor());
                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getDtaLib_VerMinor());
                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getLibNfc_VerMajor());
                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getLibNfc_VerMinor());
                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getFw_VerMajor());
                    Log.d(PhUtilities.UI_TAG,
                            "" + phDtaLibStructure.getFw_VerMinor());

                    firmWareVersionTextView.setText("UI Version: "+ "06.02"+
                            "\nFirmWare Version: "
                            + Integer.toHexString(phDtaLibStructure.getFw_VerMajor()) + "."
                            + Integer.toHexString(phDtaLibStructure.getFw_VerMinor())
                            + "\nMiddleWare Version:"
                            + Integer.toHexString(phDtaLibStructure.getLibNfc_VerMajor()) + "."
                            + Integer.toHexString(phDtaLibStructure.getLibNfc_VerMinor())
                            + " \nDTA Lib Version: "
                            + Integer.toHexString(phDtaLibStructure.getDtaLib_VerMajor()) + "."
                            + Integer.toHexString(phDtaLibStructure.getDtaLib_VerMinor()));

                } else {
                    /**
                     * Handling the error if problem in loading the library
                     */
                    mPatternNumberEditText.setFocusable(false);
                    PhCustomLibDialog customLibDialog = new PhCustomLibDialog(
                            PhDTAUIMainActivity.this, "VERSION_INFO");
                    customLibDialog.show();
                }
            } else {

                firmWareVersionTextView.setText(getResources().getString(
                        R.string.firmare_version));
            }
            break;
        case R.id.card_emulation_txt:
            Log.e(PhUtilities.UI_TAG, ""+cardEmulationTextView.isChecked());
            if(cardEmulationTextView.isChecked()){
                uiccCheckBox.setEnabled(true);
                hceCheckBox.setEnabled(true);
                eseCheckBox.setEnabled(true);
                cardEmulationTextView.setClickable(true);
                uiccCheckBox.setChecked(true);
                hceCheckBox.setChecked(false);
                eseCheckBox.setChecked(false);
                llcpCheckBox.setChecked(false);
                snepCheckBox.setChecked(false);
                multitextSpinner.setEnabled(true);
                llsnepradiogroup.clearCheck();
                clearP2PCheckBox.setChecked(false);
                clearP2PCheckBox.setClickable(true);

                Log.e(PhUtilities.UI_TAG, ""+uiccCheckBox.isChecked());
              }else if(!cardEmulationTextView.isChecked()){
                uiccCheckBox.setChecked(false);
                hceCheckBox.setChecked(false);
                eseCheckBox.setChecked(false);
                clearP2PCheckBox.setClickable(true);
              }
            break;
        case R.id.llcp_check_box:
            if(clearP2PCheckBox.isChecked()){
                llcpCheckBox.setChecked(true);
                ArrayAdapter<CharSequence> adapter;
                Log.e(PhUtilities.UI_TAG, "llcp is selected");
                multitextSpinner.setEnabled(true);
                adapter = ArrayAdapter
                    .createFromResource(PhDTAUIMainActivity.this,
                            R.array.pattern_number_llcp,
                            android.R.layout.simple_spinner_item);
                mPatternNumberEditText.setText("");
                cardEmulationTextView.setClickable(true);
                mPatternNumberEditText.setEnabled(false);
                uiccCheckBox.setChecked(false);
                hceCheckBox.setChecked(false);
                eseCheckBox.setChecked(false);
            }else{
                llcpCheckBox.setChecked(false);
            }
            break;
        case R.id.clearp2p:
            /**
             * Checkbox is the selection for P2P mode
             */
            if(clearP2PCheckBox.isChecked()){
            llcpCheckBox.setChecked(true);
            }else{
                llcpCheckBox.setChecked(false);
            }
            ArrayAdapter<CharSequence> adapter;
            if(llcpCheckBox.isChecked()){
                multitextSpinner.setEnabled(true);
                adapter = ArrayAdapter
                    .createFromResource(PhDTAUIMainActivity.this,
                            R.array.pattern_number_llcp,
                            android.R.layout.simple_spinner_item);
                mPatternNumberEditText.setText("");
                cardEmulationTextView.setClickable(true);
                mPatternNumberEditText.setEnabled(false);
                uiccCheckBox.setChecked(false);
                hceCheckBox.setChecked(false);
                eseCheckBox.setChecked(false);
            }else{
                adapter = ArrayAdapter
                        .createFromResource(PhDTAUIMainActivity.this,
                                R.array.multi_text_array,
                                android.R.layout.simple_spinner_item);
                mPatternNumberEditText.setEnabled(true);
            }
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            multitextSpinner.setAdapter(adapter);

            if(clearP2PCheckBox.isChecked()){
                Log.e(PhUtilities.UI_TAG, "P2P is checked");
                cardEmulationRadioGroup.clearCheck();
                uiccCheckBox.setChecked(false);
                uiccCheckBox.setSelected(false);
                eseCheckBox.setChecked(false);
                eseCheckBox.setSelected(false);
                llcpCheckBox.setEnabled(true);
                snepCheckBox.setEnabled(true);
                llcpCheckBox.setClickable(true);
                llcpCheckBox.setChecked(true);
                snepCheckBox.setChecked(false);
                hceCheckBox.setChecked(false);
                snepCheckBox.setClickable(true);
                multitextSpinner.setEnabled(true);
                mPatternNumberEditText.setEnabled(false);
                cardEmulationTextView.setChecked(false);
            }else{
                llcpCheckBox.setEnabled(false);
                snepCheckBox.setEnabled(false);
                llcpCheckBox.setClickable(false);
                llcpCheckBox.setChecked(false);
                snepCheckBox.setChecked(false);
                snepCheckBox.setClickable(false);
                clearP2PCheckBox.setClickable(true);
                multitextSpinner.setEnabled(true);
                mPatternNumberEditText.setEnabled(true);
            }
            break;
        case R.id.snep_check_box:
            /**
             * SNEP check box if checked throw custom dialog for displaying RTD data
             */
            if(clearP2PCheckBox.isChecked()){
                snepCheckBox.setChecked(true);
                Log.e(PhUtilities.UI_TAG, "snep is selected");
            }else{
                snepCheckBox.setChecked(false);
            }
            if (snepCheckBox.isChecked()) {
                try {
                    adapter = ArrayAdapter
                            .createFromResource(PhDTAUIMainActivity.this,
                                    R.array.multi_text_array,
                                    android.R.layout.simple_spinner_item);
                    mPatternNumberEditText.setEnabled(true);
                    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                    multitextSpinner.setAdapter(adapter);
                    PhCustomSNEPRTD phCustomSNEPRTD = new PhCustomSNEPRTD(this);
                    phCustomSNEPRTD.show();
                } catch (Exception e) {
                }
            }
            break;

        }
    }

    private void technologyError(String message) {
        AlertDialog.Builder alertDialogBuilder=new AlertDialog.Builder(this);
        alertDialogBuilder.setMessage(message);
        alertDialogBuilder.setPositiveButton(getResources().getString(R.string.ok),new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface arg0, int arg1) {
            }
        });
        alertDialogBuilder.show();
    }
    private void uiccErrorHandling(){
        if(mPatternNumberEditText.getText().toString().equals("1000")){
            cardEmulationTextView.setChecked(true);
            hceCheckBox.setChecked(true);
        }else if(!mPatternNumberEditText.getText().toString().equals("1000")){
            cardEmulationTextView.setChecked(false);
            hceCheckBox.setChecked(false);
        }
    }

    private void onClickEventForAllTheViews() {
        customMessage.setOnClickListener(this);
        showMessage.setOnClickListener(this);
        llcpCheckBox.setOnClickListener(this);
        snepCheckBox.setOnClickListener(this);
        versionnumberCheckBox.setOnClickListener(this);
        radioButtonPollA.setOnClickListener(this);
        radioButtonPollB.setOnClickListener(this);
        radioButtonPollF.setOnClickListener(this);
        radioButtonPollV.setOnClickListener(this);
        radioButtonListenA.setOnClickListener(this);
        radioButtonListenB.setOnClickListener(this);
        radioButtonListenF.setOnClickListener(this);
        radioButtonListenV.setOnClickListener(this);
        logToFileCheckBox.setOnClickListener(this);
        logToFileEditText.setOnClickListener(this);
        cardEmulationTextView.setOnClickListener(this);
        clearP2PCheckBox.setOnClickListener(this);
    }

    private void enableDiscovery() {
        statusTextView.setText(Html.fromHtml(currentStatusInitializing),TextView.BufferType.SPANNABLE);
        if (PhUtilities.NDK_IMPLEMENTATION && !mNfcAdapter.isEnabled()) {
            // 9. Start Execution of dta start task
            if((clientConnectionProgress==null) || !(clientConnectionProgress.isShowing())){
            clientConnectionProgress = ProgressDialog.show(PhDTAUIMainActivity.this, "", "Loading Please Wait..", true, true);
            clientConnectionProgress.setCancelable(false);
            }
            new Thread(){
                @Override
                public void run() {
                    super.run();
                    try {
                        settingPollListen();
                        settingDeviceType();
                        if(!mNfcAdapter.isEnabled()){
                            if(cardEmulationTextView.isChecked()){
                                enableCEFlag=1;
                                enableP2PFlag=0;
                                if(phNXPJniHelper.startPhDtaLibInit()==0){
                                Log.e(PhUtilities.UI_TAG, "DTA is Initializing");
                                }else{
                                Log.e(PhUtilities.UI_TAG, "Already DTA is Initialized");
                                }
                                phNXPJniHelper.nativeiInitEnableDeviceInfo(enableCEFlag,enableP2PFlag, mPatternNumber);
                            }else {
                                enableCEFlag=0;
                                enableP2PFlag=0;
                                phNXPJniHelper.startTest();
                                if(phNXPJniHelper.startPhDtaLibInit()==0){
                                    Log.e(PhUtilities.UI_TAG, "DTA is Initializing");
                                    }else{
                                    Log.e(PhUtilities.UI_TAG, "Already DTA is Initialized");
                                    }
                                if(llcpCheckBox.isChecked()){
                                    enableP2PFlag=1;
                                    phNXPJniHelper.nativeiInitEnableDeviceInfo(enableCEFlag , enableP2PFlag , mPatternNumber);
                                }else{
                                    phNXPJniHelper.nativeiInitEnableDeviceInfo(enableCEFlag , enableP2PFlag , mPatternNumber);
                                }
                            }
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    clientConnectionProgress.dismiss();
                }

            }.start();
            statusTextView.setText(Html.fromHtml(currentStatusRunning),TextView.BufferType.SPANNABLE);
        }
    }

    private void automodeOnRunButtonPressed() {
        Log.d(PhUtilities.UI_TAG, "Looping into Auto Mode");
        mPatternNumberEditText.setHint("Custom");
        mPatternNumberEditText.setEnabled(false);
        multitextSpinner.setEnabled(false);
        if (PhUtilities.CLIENT_SELECTED) {
            if (PhUtilities.SIMULATED_SERVER_SELECTED) {
                Log.d(PhUtilities.TCPSRV_TAG,
                        "Starting TCP/IP Server Simulator. On RUN button click.");

                new PhNioServerTask(portNumber)
                        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                // Utilities.SIMULATED_SERVER_SELECTED = false;
            } else {
                Log.d(PhUtilities.TCPCNT_TAG,
                        "Starting TCP/IP Client. On RUN button click.");
                if(inetAddress != null){
                    Log.d(PhUtilities.TCPCNT_TAG, "IPaddress captured from UI : " + inetAddress.getHostAddress());
                    Log.d(PhUtilities.TCPCNT_TAG, "Port Number captured from UI : " + portNumber);
                }
                new PhNioClientTask(inetAddress, portNumber)
                        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                // Utilities.CLIENT_SELECTED = false;
            }

        } else if (PhUtilities.SERVER_SELECTED) {
             Log.d(PhUtilities.TCPSRV_TAG,
                     "Starting TCP/IP Server. On RUN button click.");

             new PhNioServerTask(portNumber)
                     .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }

        try {
            TimeUnit.MILLISECONDS.sleep(500);
        } catch (InterruptedException e1) {
            e1.printStackTrace();
        }

        sPatternNumber = PhUtilities.patternNUmberBuff == null ? "0" : PhUtilities.patternNUmberBuff;
        try {
            /**
             * Initialized cardEmulationModeBuff to zero by default
             */
            cePatternEnable = Integer
                    .parseInt(PhUtilities.cardEmulationModeBuff) == 0 ? false
                    : true;
        } catch (NumberFormatException e) {
            Log.e(PhUtilities.UI_TAG, "cardEmulationModeBuff : "
                    + PhUtilities.cardEmulationModeBuff);
        }

    }

    /** Enable all the buttons */

    public void manualModeisON() {
        checkedButtonPollA = true;
        checkedButtonPollB = true;
        checkedButtonPollF = true;
        checkedButtonPollV = true;
        checkedButtonListenA = true;
        checkedButtonListenB = true;
        checkedButtonListenF = true;
        checkedButtonListenV = true;
        uiccCheckBox.setEnabled(true);
        hceCheckBox.setEnabled(true);
        eseCheckBox.setEnabled(true);
        cardEmulationTextView.setEnabled(true);
        radioButtonPollA.setEnabled(true);
        radioButtonPollB.setEnabled(true);
        radioButtonPollF.setEnabled(true);
        radioButtonPollV.setEnabled(false);
        radioButtonListenA.setEnabled(true);
        radioButtonListenB.setEnabled(true);
        radioButtonListenF.setEnabled(true);
        radioButtonListenV.setEnabled(false);
        radioButtonPollA.setChecked(true);
        radioButtonPollB.setChecked(true);
        radioButtonPollF.setChecked(true);
        radioButtonListenA.setChecked(true);
        radioButtonListenB.setChecked(true);
        radioButtonListenF.setChecked(true);
        radioButtonListenV.setEnabled(false);
        uiccCheckBox.setEnabled(true);
        hceCheckBox.setEnabled(true);
        llcpCheckBox.setEnabled(true);
        snepCheckBox.setEnabled(true);
        depiCheckBox.setEnabled(false);
        deptCheckBox.setEnabled(false);
        mPatternNumberEditText.setEnabled(true);
        multitextSpinner.setEnabled(true);
        /** enable mode */
        autoRadioButton.setEnabled(true);
        manualRadioButton.setEnabled(true);
        logToFileEditText.setEnabled(true);
        logcatCheckBox.setEnabled(true);
        versionnumberCheckBox.setEnabled(true);
        multitextSpinner.setEnabled(true);
        clearP2PCheckBox.setEnabled(true);

    }

    /** Disable all the buttons */
    public void autoModeisON() {
        checkedButtonPollA = true;
        checkedButtonPollB = true;
        checkedButtonPollF = true;
        checkedButtonPollV = true;
        checkedButtonListenA = true;
        checkedButtonListenB = true;
        checkedButtonListenF = true;
        checkedButtonListenV = true;
        radioButtonPollA.setEnabled(true);
        radioButtonPollB.setEnabled(true);
        radioButtonPollF.setEnabled(true);
        radioButtonPollV.setEnabled(false);
        radioButtonListenA.setEnabled(true);
        radioButtonListenB.setEnabled(true);
        radioButtonListenF.setEnabled(true);
        radioButtonListenV.setEnabled(false);
        uiccCheckBox.setEnabled(false);
        hceCheckBox.setEnabled(false);
        eseCheckBox.setEnabled(false);
        llcpCheckBox.setEnabled(false);
        snepCheckBox.setEnabled(false);
        depiCheckBox.setEnabled(false);
        deptCheckBox.setEnabled(false);
        radioButtonPollA.setChecked(true);
        radioButtonPollB.setChecked(true);
        radioButtonPollF.setChecked(true);
        radioButtonPollV.setChecked(false);
        radioButtonListenA.setChecked(true);
        radioButtonListenB.setChecked(true);
        radioButtonListenF.setChecked(true);
        radioButtonListenV.setChecked(false);
        uiccCheckBox.setChecked(false);
        hceCheckBox.setChecked(false);
        llcpCheckBox.setChecked(false);
        snepCheckBox.setChecked(false);
        depiCheckBox.setChecked(false);
        deptCheckBox.setChecked(false);
        logToFileCheckBox.setEnabled(false);
        multitextSpinner.setEnabled(false);
        uiccCheckBox.setEnabled(false);
        hceCheckBox.setEnabled(false);
        eseCheckBox.setEnabled(false);
        cardEmulationTextView.setEnabled(false);
             /** enable mode */
        autoRadioButton.setEnabled(true);
        manualRadioButton.setEnabled(true);
        logToFileEditText.setEnabled(true);
        logcatCheckBox.setEnabled(true);
        versionnumberCheckBox.setEnabled(true);
        autoRadioButton.setEnabled(true);
        manualRadioButton.setEnabled(true);
        clearP2PCheckBox.setChecked(false);
        clearP2PCheckBox.setEnabled(false);
        mPatternNumberEditText.setEnabled(false);
    }

    /** Disable all the buttons */

    public void disableAllOtherBtns() {
        radioButtonPollA.setEnabled(false);
        radioButtonPollB.setEnabled(false);
        radioButtonPollF.setEnabled(false);
        radioButtonPollV.setEnabled(false);
        radioButtonListenA.setEnabled(false);
        radioButtonListenB.setEnabled(false);
        radioButtonListenF.setEnabled(false);
        radioButtonListenV.setEnabled(false);
        uiccCheckBox.setEnabled(false);
        hceCheckBox.setEnabled(false);
        eseCheckBox.setEnabled(false);
        llcpCheckBox.setEnabled(false);
        snepCheckBox.setEnabled(false);
        depiCheckBox.setEnabled(false);
        deptCheckBox.setEnabled(false);
        deviceSelectorSpinner.setEnabled(false);
        mPatternNumberEditText.setEnabled(false);
        logcatCheckBox.setEnabled(false);
        logcatCheckBox.setFocusable(false);
        versionnumberCheckBox.setEnabled(false);
        multitextSpinner.setEnabled(false);
        autoRadioButton.setEnabled(false);
        manualRadioButton.setEnabled(false);
        logToFileEditText.setEnabled(false);
        logToFileCheckBox.setEnabled(false);
        clearP2PCheckBox.setEnabled(false);
        cardEmulationTextView.setEnabled(false);

    }

    /** Enable the Custom Message and Show Message from user actions */
    public void selectionMode(boolean selectionMode) {
        customMessage.setEnabled(selectionMode);
        showMessage.setEnabled(selectionMode);
        tcpIPTextView.setEnabled(selectionMode);

    }

    /**
     * On Hard ware Back Key is Pressed handled here
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            PhCustomExitDialog customExitDialog = new PhCustomExitDialog(this,"Are you sure you want to exit?",true);
            customExitDialog.show();
        }

        return super.onKeyDown(keyCode, event);
    }

    public static int DTA_STARTED = 1;
    public static int DTA_STOP = 2;

    /**
     * When user press the Stop Button and Closing all the process
     */
    private void reLaunchMainActivity() {
        try {
            if(PhUtilities.NDK_IMPLEMENTATION){
            Log.v(PhUtilities.UI_TAG, "phDtaLibDisableDiscovery");
            phNXPJniHelper.phDtaLibDisableDiscovery();
            }
        } catch (NullPointerException e) {
            e.printStackTrace();
        } catch (ActivityNotFoundException e) {
        }

    }

    /**
     * Checking device contains NFC Tag and it is enabled or not
     */
    private void nfccheck() {
        boolean nfcEnable = false;
        nfcEnable = mNfcAdapter.isEnabled();
        // Pop AlertDialog to Reminder Start NFC
        if(PhCustomSNEPRTD.CHECKSNEPUIENABLED && nfcEnable){
            Log.v(PhUtilities.UI_TAG,"Nothing to do");
        }else{
        final Context ctx = this;
        AlertDialog.Builder builder = new AlertDialog.Builder(ctx);
        if (nfcEnable == true) {
            Log.d(PhUtilities.UI_TAG, "NFC ON");
            builder.setMessage(" Turn OFF NFC Service in DTA mode \n Wait for 5 seconds and Run DTA")
            // .setNegativeButton("Cancel", null) Anil 24/04/3:35
                    .setPositiveButton("Enter",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int id) {
                                    try {
                                        dialog.cancel();
                                        ctx.startActivity(new Intent(Settings.ACTION_NFC_SETTINGS));
                                        checkNfcServiceDialog = true;
                                    } catch (Exception e) {
                                        e.printStackTrace();
                                    }
                                }
                            });
            builder.setCancelable(false);
            builder.show();
        }
        }
    }

    /**
     * Checking Auto Mode or Manual Mode is enabled
     */
    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {

        switch (group.getCheckedRadioButtonId()) {
        case R.id.auto_radio_button:
            mPatternNumberEditText.setEnabled(false);
            mPatternNumberEditText.setText("");
            cardEmulationTextView.setChecked(false);
            autoModeisON();
            /**
             * Auto Mode is Enable Opening the Client Server Communication
             */
            if (autoRadioButton.isChecked()) {

                PhCustomIPDialog customIPDialog = new PhCustomIPDialog(this);
                customIPDialog.show();
                autoRadioButton.setEnabled(false);
                manualRadioButton.setEnabled(true);
                multitextSpinner.setEnabled(false);
                selectionMode(true);
            }
            break;

        case R.id.manual_radio_button:
            manualModeisON();
            /**
             * Manual Mode is Enable
             */
            if (manualRadioButton.isChecked()) {
                manualRadioButton.setEnabled(false);
                autoRadioButton.setEnabled(true);
                selectionMode(false);
                customMessage.setChecked(false);
                showMessage.setChecked(false);
            }
            break;
        case R.id.uicc_check_box:
            if(!cardEmulationTextView.isChecked()){
                uiccCheckBox.setChecked(false);
            }else{
                uiccCheckBox.setChecked(true);
                Log.e(PhUtilities.UI_TAG, "uicc is selected");
            }
            break;
        case R.id.cefh_check_box:
            Log.e(PhUtilities.UI_TAG, "hce  is selected");
            if(!cardEmulationTextView.isChecked()){
                hceCheckBox.setChecked(false);
            }else{
                hceCheckBox.setChecked(true);
            }
            break;
        case R.id.ese_Check_box:
            Log.e(PhUtilities.UI_TAG, "ESE is selected");
            if(!cardEmulationTextView.isChecked()){
                eseCheckBox.setChecked(false);
            }else{
                eseCheckBox.setChecked(true);
            }

        case R.id.llcp_snep_rg:
            llcpSnepSpinnerSelection();
            break;
    }
}

    /**
     * Edit Text validation For Custom Pattern Number
     */

    public void llcpSnepSpinnerSelection(){
        ArrayAdapter<CharSequence> adapter;
        if(llcpCheckBox.isChecked() || snepCheckBox.isChecked()){
             adapter = ArrayAdapter
                         .createFromResource(PhDTAUIMainActivity.this,
                                          R.array.pattern_number_llcp,
                                             android.R.layout.simple_spinner_item);
              mPatternNumberEditText.setEnabled(false);
              mPatternNumberEditText.setText("");
              uiccCheckBox.setChecked(false);
              hceCheckBox.setChecked(false);
        }else{
                adapter = ArrayAdapter
                   .createFromResource(PhDTAUIMainActivity.this,
                        R.array.multi_text_array,
                          android.R.layout.simple_spinner_item);
                       mPatternNumberEditText.setEnabled(true);
                       //cardEmulationTextView.setClickable(true);
                       uiccCheckBox.setClickable(true);
                       hceCheckBox.setClickable(true);
                       eseCheckBox.setClickable(true);
                       llcpCheckBox.setClickable(true);
                       snepCheckBox.setClickable(true);
        }
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        multitextSpinner.setAdapter(adapter);
    }
    public class MaxLengthWatcher implements TextWatcher {

        private int maxLen = 0;

        public MaxLengthWatcher(int maxLen) {
            this.maxLen = maxLen;
        }

        public void afterTextChanged(Editable arg0) {

        }

        public void beforeTextChanged(CharSequence arg0, int arg1, int arg2,
                int arg3) {

        }

        public void onTextChanged(CharSequence chars, int start, int before,
                int count) {
            Editable editable = mPatternNumberEditText.getText();
            int len = editable.length();
            String checkValidite = null;
            checkValidite = chars.toString();
            Log.d(PhUtilities.UI_TAG, "Get pattern Number checkValidite "
                    + checkValidite);

            Log.d(PhUtilities.UI_TAG, "Get pattern Number length "
                    + checkValidite.length());
            if ((checkValidite.length() > 4)
                    && (checkValidite.matches(("[a-fA-F0-9]+")))) {
                Log.d(PhUtilities.UI_TAG,
                        "" + (checkValidite.matches(("[a-fA-F0-9]+"))));
                customPopValidation = new PhCustomPopValidation(
                        PhDTAUIMainActivity.this, getResources().getString(
                                R.string.warning));
                customPopValidation.show();
                Log.d(PhUtilities.UI_TAG,
                        "Entered the correct pattern number but it excedes 4 characters");
            } else if ((checkValidite.matches(("[a-fA-F0-9]+")))) {
                Log.d(PhUtilities.UI_TAG,
                        "" + (checkValidite.matches(("[a-fA-F0-9]+"))));

                Log.d(PhUtilities.UI_TAG, "Entered the correct pattern number");
            } else if ((checkValidite.length() != 0)) {
                customPopValidation = new PhCustomPopValidation(
                        PhDTAUIMainActivity.this, getResources().getString(
                                R.string.warning_for_wrong_key));
                Log.d(PhUtilities.UI_TAG,
                        "extractText"
                                + checkValidite.substring(0,
                                        checkValidite.length() - 1));
                mPatternNumberEditText.setText(checkValidite.substring(0,
                        checkValidite.length() - 1));
                customPopValidation.show();
                Log.d(PhUtilities.UI_TAG, "Entered the wrong pattern number");
            } else {
                Log.d(PhUtilities.UI_TAG,
                        "Entered the else wrong pattern number");
            }

            uiccErrorHandling();
            /**
             *
             * Spinner implementation
             */
            if (mPatternNumberEditText.getText().toString().length() > 0) {
                Log.d(PhUtilities.UI_TAG,
                        "Show pop user enter more than 4 characters");
                multitextSpinner.setEnabled(false);
                ArrayAdapter<CharSequence> adapter = ArrayAdapter
                        .createFromResource(PhDTAUIMainActivity.this,
                                R.array.empty_Array,
                                android.R.layout.simple_spinner_item);
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                multitextSpinner.setAdapter(adapter);

            } else {
                multitextSpinner.setEnabled(true);
                Log.d(PhUtilities.UI_TAG, "Spinner is enabled");
                ArrayAdapter<CharSequence> adapter;
                if(llcpCheckBox.isChecked()){
                    adapter = ArrayAdapter
                        .createFromResource(PhDTAUIMainActivity.this,
                                R.array.pattern_number_llcp,
                                android.R.layout.simple_spinner_item);
                    mPatternNumberEditText.setEnabled(false);

                }else if(snepCheckBox.isChecked()){
                    adapter = ArrayAdapter
                            .createFromResource(PhDTAUIMainActivity.this,
                                    R.array.empty_Array,
                                    android.R.layout.simple_spinner_item);
                        mPatternNumberEditText.setEnabled(false);
                }else{
                    adapter = ArrayAdapter
                            .createFromResource(PhDTAUIMainActivity.this,
                                    R.array.multi_text_array,
                                    android.R.layout.simple_spinner_item);
                    mPatternNumberEditText.setEnabled(true);
                }
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                multitextSpinner.setAdapter(adapter);

            }

            /*** Restricting edit text to maximum 4 characters */
            if (len > maxLen) {
                int selEndIndex = Selection.getSelectionEnd(editable);
                String str = editable.toString();
                String newStr = str.substring(0, maxLen);
                mPatternNumberEditText.setText(newStr);
                editable = mPatternNumberEditText.getText();
                int newLen = editable.length();
                if (selEndIndex > newLen) {
                    selEndIndex = editable.length();
                }
                Selection.setSelection(editable, selEndIndex);
            }
        }
    }

    /*** Spinner on selection */

    @Override
    public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
            long arg3) {
        Log.d(PhUtilities.UI_TAG,
                "get pattern number" + multitextSpinner.getSelectedItem().toString());

        nxpHelperMainActivity.setsPatternnumber(multitextSpinner.getSelectedItem().toString());

    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {

    }

    /**
     * Enable and disable button a/c to auto or manual mode
     */

    @Override
    protected void onDestroy() {
        if (!mNfcAdapter.isEnabled() && PhUtilities.NDK_IMPLEMENTATION
                && !PhCustomExitDialog.deInitInCustomExitDialog) {
            Log.d(PhUtilities.UI_TAG, "Stop the Deinitialize the DTA");
            phNXPJniHelper.phDtaLibDisableDiscovery();
            android.os.Process.killProcess(android.os.Process.myPid());
        }else{
            android.os.Process.killProcess(android.os.Process.myPid());
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        runBtn.setEnabled(true);
        mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
        nfccheck();
        if(!(mNfcAdapter.isEnabled())){
            if(PhUtilities.NDK_IMPLEMENTATION==true && phNXPJniHelper.startPhDtaLibInit()==0){
                Log.e(PhUtilities.UI_TAG, "DTA is Initializing");
            }else{
                Log.e(PhUtilities.UI_TAG, "Already DTA is Initialized");
            }
        }
    }
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    public static PhDtaLibStructure settingPollListen() {

    PhDtaLibStructure phDtaLibStructure = new PhDtaLibStructure();

        /**
         * Poll Technology
         */
        if (radioButtonPollA.isChecked()) {
            phDtaLibStructure.pollA = 1;
        } else {
            phDtaLibStructure.pollA = 0;
        }
        if (radioButtonPollB.isChecked()) {
            phDtaLibStructure.pollB = 1;
        } else {
            phDtaLibStructure.pollB = 0;
        }
        if (radioButtonPollF.isChecked()) {
            phDtaLibStructure.pollF = 1;
        } else {
            phDtaLibStructure.pollF = 0;
        }
        /**
         * Listen Technology
         */
        if (radioButtonListenA.isChecked()) {
            phDtaLibStructure.listenA = 1;
        } else {
            phDtaLibStructure.listenA = 0;
        }
        if (radioButtonListenB.isChecked()) {
            phDtaLibStructure.listenB = 1;
        } else {
            phDtaLibStructure.listenB = 0;
        }
        if (radioButtonListenF.isChecked()) {
            phDtaLibStructure.listenF = 1;
        } else {
            phDtaLibStructure.listenF = 0;
        }
        return phDtaLibStructure;

    }

    public static PhDtaLibStructure settingDeviceType() {

        PhDtaLibStructure phDtaLibStructure = new PhDtaLibStructure();
        /**
         * Uicc check box is enabled or disabled
         */
        if (uiccCheckBox.isChecked()) {
            phDtaLibStructure.phDtaLibUICCCe = 1;
        } else {
            phDtaLibStructure.phDtaLibUICCCe = 0;
        }
        /**
         * @authoranil give actual value now hard coded to hce check box only
         *             SECE check box is enabled or disabled
         */
        /*
         * if (hceCheckBox.isChecked()) { phDtaLibCeDevTypeT.phDtaLibSECe = 1; }
         * else {
         */
        if(eseCheckBox.isChecked()) {
            phDtaLibStructure.phDtaLibSECe = 1;
        } else {
            phDtaLibStructure.phDtaLibSECe = 0;
        }
        // }
        /**
         * HBECE check box is enabled or disabled
         */
        if (hceCheckBox.isChecked()) {
            phDtaLibStructure.phDtaLibeHBECe = 1;
        } else {
            phDtaLibStructure.phDtaLibeHBECe = 0;
        }

        return phDtaLibStructure;
    }

    public static PhDtaLibStructure settingP2PFlag(){
        PhDtaLibStructure phDtaLibStructure = new PhDtaLibStructure();

        /**
         * LLCP flag sets to 1 if checked else sets to 0
         */
        if(llcpCheckBox.isChecked()) {
            phDtaLibStructure.phDtaLibLLCPP2p = 1;
        }else{
            phDtaLibStructure.phDtaLibLLCPP2p = 0;
        }

        /**
         * SNEP flag seing set to 1 if checked else sets to 0
         */
        if(snepCheckBox.isChecked()) {
            phDtaLibStructure.phDtaLibSNEPP2p = 1;
        }else{
            phDtaLibStructure.phDtaLibSNEPP2p = 0;
        }

        return phDtaLibStructure;
    }

    public void setIpAdressAndPortNumber(String ipAddress, String portNumberString) throws UnknownHostException{
        inetAddress = InetAddress.getByName(ipAddress);
        portNumber = Integer.parseInt(portNumberString);
    }
    private void clientImplementation() {
        if(PhUtilities.CLIENT_SELECTED){
            clientConnectionProgress = ProgressDialog.show(PhDTAUIMainActivity.this, "ClientConnection", "Client trying to Connect to Server", true, true);
            new Thread(new Runnable() {
                @Override
                public void run() {
                    while(PhUtilities.countOfRetry <= PhUtilities.maxNumberOfRetries){
                        Log.d(PhUtilities.UI_TAG, " "+PhUtilities.countOfRetry);
                       try{
                           Thread.sleep(100);
                       } catch(Exception e){

                       }
                       if(PhUtilities.isClientConnected){
                           clientConnectionProgress.dismiss();
                           break;
                       }
                    }
                    if(!PhUtilities.isClientConnected){
                        clientConnectionProgress.dismiss();
                        final Builder alertDialog = new AlertDialog.Builder(PhDTAUIMainActivity.this);
                        alertDialog.setTitle("Client Message");
                        alertDialog.setMessage("Retried maximum number of times...No Luck");
                        alertDialog.setNeutralButton("Ok", new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        });
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                alertDialog.show();
                            }
                        });
                    }
                }
            }).start();
        }
    }
    public void setFindViewByID(){
       // versionNumber = (TextView) findViewById(R.id.version_number);
        logToFileEditText = (EditText) findViewById(R.id.logtofiletext);
        mPatternNumberEditText = (EditText) findViewById(R.id.custom);
        /**
         * Calling all the Check Box id's
         */
        logToFileCheckBox = (CheckBox) findViewById(R.id.logtofile);
        radioButtonPollA = (CheckBox) findViewById(R.id.a_poll_btn);
        radioButtonPollB = (CheckBox) findViewById(R.id.b_poll_btn);
        radioButtonPollF = (CheckBox) findViewById(R.id.f_poll_btn);
        radioButtonPollV = (CheckBox) findViewById(R.id.v_poll_btn);
        radioButtonListenA = (CheckBox) findViewById(R.id.a_listen_btn);
        radioButtonListenB = (CheckBox) findViewById(R.id.b_listen_btn);
        radioButtonListenF = (CheckBox) findViewById(R.id.f_listen_btn);
        radioButtonListenV = (CheckBox) findViewById(R.id.v_listen_btn);
        logcatCheckBox = (CheckBox) findViewById(R.id.logcat);
        versionnumberCheckBox = (CheckBox) findViewById(R.id.versionnumber);
        customMessage = (CheckBox) findViewById(R.id.custom_msg);
        showMessage = (CheckBox) findViewById(R.id.show_msg);
        /**
         * CheckBox for clearing selections under P2P category
         */
        clearP2PCheckBox = (CheckBox) findViewById(R.id.clearp2p);
        clearP2PCheckBox.setOnClickListener(this);
        /**
         * Calling all the Radio Group id's
         */
        autoManualRadioGroup = (RadioGroup) findViewById(R.id.auto_manual_rb);
        llsnepradiogroup = (RadioGroup) findViewById(R.id.llcp_snep_rg);
        cardEmulationRadioGroup = (RadioGroup) findViewById(R.id.card_emulation_radio_group);
        cardEmulationRadioGroup.setOnCheckedChangeListener(this);
        /**
         * Calling all the Radio Button id's
         */
        autoRadioButton = (RadioButton) findViewById(R.id.auto_radio_button);
        manualRadioButton = (RadioButton) findViewById(R.id.manual_radio_button);
        uiccCheckBox = (RadioButton) findViewById(R.id.uicc_check_box);
        hceCheckBox = (RadioButton) findViewById(R.id.cefh_check_box);
        eseCheckBox = (RadioButton) findViewById(R.id.ese_Check_box);
        llcpCheckBox = (RadioButton) findViewById(R.id.llcp_check_box);
        snepCheckBox = (RadioButton) findViewById(R.id.snep_check_box);
        depiCheckBox = (RadioButton) findViewById(R.id.depinitiator_check_box);
        deptCheckBox = (RadioButton) findViewById(R.id.deptarget_check_box);
        /**
         * Calling all the Text View id's
         */
        copyRightsTextView = (TextView) findViewById(R.id.copyrights);
        tcpIPTextView = (TextView) findViewById(R.id.tcp_ip_txt);
        vPollTextView = (TextView) findViewById(R.id.v_poll_text);
        vListenTextView = (TextView) findViewById(R.id.v_listen_text);
        cardEmulationTextView = (CheckBox) findViewById(R.id.card_emulation_txt);
        p2pTextView = (TextView) findViewById(R.id.p2p_txt);
        logMessageTextView = (TextView) findViewById(R.id.log_msg);
        deviceTextView = (TextView) findViewById(R.id.device_msg);
        deviceSelectionTextView = (TextView) findViewById(R.id.deviceselection);
        firmWareVersionTextView = (TextView) findViewById(R.id.firmwareversion_text_view);
        statusTextView = (TextView) findViewById(R.id.current_status);
        /**
         * Calling all the Spinner id's
         */
        multitextSpinner = (Spinner) findViewById(R.id.spinner_text);
        deviceSelectorSpinner = (Spinner) findViewById(R.id.device_selector);
        /**
         * Calling all the Button id's
         */
        runBtn = (Button) findViewById(R.id.run);
        stopBtn = (Button) findViewById(R.id.stop);
        exitBtn = (Button) findViewById(R.id.exit_btn);
    }

    @Override
    public void onCheckedChanged(CompoundButton view, boolean arg1) {
        switch (view.getId()) {
        case R.id.card_emulation_txt:
            if(!cardEmulationTextView.isChecked()){
                uiccCheckBox.setChecked(false);
                hceCheckBox.setChecked(false);
            }else{
                ArrayAdapter<CharSequence> adapter;
                mPatternNumberEditText.setEnabled(true);
                adapter = ArrayAdapter.createFromResource(PhDTAUIMainActivity.this,
                        R.array.multi_text_array,android.R.layout.simple_spinner_item);
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                multitextSpinner.setAdapter(adapter);
            }
            break;

        case R.id.logtofile:
            if (logToFileCheckBox.isChecked()) {
                logToFileEditText.setFocusable(true);
                logToFileEditText.setFocusableInTouchMode(true);
            } else {
                logToFileEditText.setFocusable(false);
                logToFileEditText.setFocusableInTouchMode(false);
                logToFileEditText.setText("");
            }
            break;

        case R.id.clearp2p:
            llcpCheckBox.setChecked(false);
            snepCheckBox.setChecked(false);
            depiCheckBox.setChecked(false);
            deptCheckBox.setChecked(false);
            llsnepradiogroup.clearCheck();
            break;
        case R.id.logcat:
             if (logcatCheckBox.isChecked()) {}
        break;
        }
    }
    private void appearDissappearOFViews() {
        if (PhUtilities.APPEAR_CUSTOM) {
            mPatternNumberEditText.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_CUSTOM) {
            mPatternNumberEditText.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_CUSTOM) {
            mPatternNumberEditText.setEnabled(false);
        }

        /** Appear disappear for auto_mode */
        if (PhUtilities.APPEAR_AUTO_MODE) {
            autoRadioButton.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_AUTO_MODE) {
            autoRadioButton.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_AUTO_MODE) {
            autoRadioButton.setEnabled(false);
        }
        /** Appear disappear for show message */
        if (PhUtilities.APPEAR_SHOW_MESSAGE) {
            showMessage.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_SHOW_MESSAGE) {
            showMessage.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_SHOW_MESSAGE) {
            showMessage.setEnabled(false);
        }
        /** Appear disappear for custom message */
        if (PhUtilities.APPEAR_CUSTOM_MESSAGE) {
            customMessage.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_CUSTOM_MESSAGE) {
            customMessage.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_CUSTOM_MESSAGE) {
            customMessage.setEnabled(false);
        }
        /** Appear disappear for TCP/IP Text message */
        if (PhUtilities.APPEAR_TCP_IP_STATUS) {
            tcpIPTextView.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_TCP_IP_STATUS) {
            tcpIPTextView.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_TCP_IP_STATUS) {
            tcpIPTextView.setEnabled(false);
        }
        /** Appear disappear for V_RF_Technology Text message */
        if (PhUtilities.APPEAR_V_RF_POLL_TECHNOLOGY) {
            radioButtonPollV.setVisibility(View.VISIBLE);
            vPollTextView.setVisibility(View.VISIBLE);

        } else if (PhUtilities.DISAPPER_V_RF_POLL_TECHNOLOGY) {
            radioButtonPollV.setVisibility(View.INVISIBLE);
            vPollTextView.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_V_RF_POLL_TECHNOLOGY) {
            radioButtonPollV.setEnabled(false);
            vPollTextView.setEnabled(false);
        }
        /** Appear disappear for V_RF_Technology Text message */
        if (PhUtilities.APPEAR_V_RF_LISTEN_TECHNOLOGY) {
            radioButtonListenV.setVisibility(View.VISIBLE);
            vListenTextView.setVisibility(View.VISIBLE);
        } else if (PhUtilities.DISAPPER_V_RF_LISTEN_TECHNOLOGY) {
            radioButtonListenV.setVisibility(View.INVISIBLE);
            vListenTextView.setVisibility(View.INVISIBLE);
        } else if (PhUtilities.FADING_V_RF_LISTEN_TECHNOLOGY) {
            radioButtonListenV.setEnabled(false);
            vListenTextView.setEnabled(false);
        }

        /** Appear disappear for card emulation */
        if (PhUtilities.APPEAR_CARD_EMULATION) {
            uiccCheckBox.setVisibility(View.VISIBLE);
            cardEmulationTextView.setVisibility(View.VISIBLE);
            hceCheckBox.setVisibility(View.VISIBLE);
            eseCheckBox.setVisibility(View.VISIBLE);

        } else if (PhUtilities.DISAPPER_CARD_EMULATION) {
            uiccCheckBox.setVisibility(View.INVISIBLE);
            cardEmulationTextView.setVisibility(View.INVISIBLE);
            hceCheckBox.setVisibility(View.INVISIBLE);
            eseCheckBox.setVisibility(View.INVISIBLE);

        } else if (PhUtilities.FADING_CARD_EMULATION) {
            uiccCheckBox.setEnabled(false);
            cardEmulationTextView.setEnabled(false);
            hceCheckBox.setEnabled(false);
            eseCheckBox.setEnabled(false);
        }
        /** Appear disappear for p2p */
        if (PhUtilities.APPEAR_CARD_EMULATION) {
            p2pTextView.setVisibility(View.VISIBLE);
            llcpCheckBox.setVisibility(View.VISIBLE);
            snepCheckBox.setVisibility(View.VISIBLE);
            depiCheckBox.setVisibility(View.INVISIBLE);
            deptCheckBox.setVisibility(View.INVISIBLE);

        }/** Appear disappear for card emulation */
        else if (PhUtilities.DISAPPER_CARD_EMULATION) {
            p2pTextView.setVisibility(View.INVISIBLE);
            llcpCheckBox.setVisibility(View.INVISIBLE);
            snepCheckBox.setVisibility(View.INVISIBLE);
            depiCheckBox.setVisibility(View.INVISIBLE);
            deptCheckBox.setVisibility(View.INVISIBLE);

        } else if (PhUtilities.FADING_CARD_EMULATION) {
            p2pTextView.setEnabled(false);
            llcpCheckBox.setEnabled(false);
            snepCheckBox.setEnabled(false);
            depiCheckBox.setEnabled(false);
            deptCheckBox.setEnabled(false);
        }

        /** Appear disappear for Log Message */
        if (PhUtilities.APPEAR_LOG_MESSAGE) {
            cardEmulationTextView.setVisibility(View.VISIBLE);
            logToFileEditText.setVisibility(View.VISIBLE);
            logToFileCheckBox.setVisibility(View.VISIBLE);
            logcatCheckBox.setVisibility(View.VISIBLE);

        } else if (PhUtilities.DISAPPER_LOG_MESSAGE) {
            logMessageTextView.setVisibility(View.INVISIBLE);
            logToFileEditText.setVisibility(View.INVISIBLE);
            logToFileCheckBox.setVisibility(View.INVISIBLE);
            logcatCheckBox.setVisibility(View.INVISIBLE);

        } else if (PhUtilities.FADING_LOG_MESSAGE) {
            logToFileEditText.setEnabled(false);
            logMessageTextView.setEnabled(false);
            logToFileCheckBox.setEnabled(false);
            logcatCheckBox.setEnabled(false);
        }

        /** Appear disappear for Device selection and spinner */
        if (PhUtilities.APPEAR_DEVICE) {
            deviceTextView.setVisibility(View.VISIBLE);
            deviceSelectionTextView.setVisibility(View.VISIBLE);
            deviceSelectorSpinner.setVisibility(View.VISIBLE);

        } else if (PhUtilities.DISAPPER_DEVICE) {
            deviceTextView.setVisibility(View.INVISIBLE);
            deviceSelectionTextView.setVisibility(View.INVISIBLE);
            deviceSelectorSpinner.setVisibility(View.INVISIBLE);

        } else if (PhUtilities.FADING_DEVICE) {
            deviceTextView.setEnabled(false);
            deviceSelectionTextView.setEnabled(false);
            deviceSelectorSpinner.setEnabled(false);
        }

        /** Appear disappear for Device selection and spinner */
        if (PhUtilities.APPEAR_DEVICE) {
            deviceTextView.setVisibility(View.VISIBLE);
            firmWareVersionTextView.setVisibility(View.VISIBLE);
            versionnumberCheckBox.setVisibility(View.VISIBLE);

        } else if (PhUtilities.DISAPPER_DEVICE) {
            deviceTextView.setVisibility(View.INVISIBLE);
            firmWareVersionTextView.setVisibility(View.INVISIBLE);
            versionnumberCheckBox.setVisibility(View.INVISIBLE);

        } else if (PhUtilities.FADING_DEVICE) {
            deviceTextView.setEnabled(false);
            firmWareVersionTextView.setEnabled(false);
            versionnumberCheckBox.setEnabled(false);
        }

        if (PhUtilities.FADING_DEPT_P2P) {
            depiCheckBox.setEnabled(false);
            deptCheckBox.setEnabled(false);
        }
    }

}

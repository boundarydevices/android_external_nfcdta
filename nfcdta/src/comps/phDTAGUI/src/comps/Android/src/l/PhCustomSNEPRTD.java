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

package com.phdtaui.customdialogs;

import java.nio.charset.Charset;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.CreateNdefMessageCallback;
import android.nfc.NfcAdapter.OnNdefPushCompleteCallback;
import android.nfc.NfcEvent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import com.phdtaui.mainactivity.PhDTAUIMainActivity;
import com.phdtaui.mainactivity.R;
import com.phdtaui.messenger.PhMessengerService;
import com.phdtaui.helper.PhNXPJniHelper;
import com.phdtaui.utils.PhUtilities;

public class PhCustomSNEPRTD extends Dialog implements
        android.view.View.OnClickListener, CreateNdefMessageCallback,
        OnNdefPushCompleteCallback, OnItemSelectedListener {
    private static final String TAG = "SnepExtendedDTAServer";
    private Context mContext;
    public  static  Button runServerButton , stopServerButton , runClientButton , stopClientButton , backBtn,clientClearMsgBtn,serverClearMsgBtn;
    public  static Spinner testCasesSpinner;
    private NfcAdapter mNfcAdapter;
    private static final int MESSAGE = 1 , MESSAGE_RECEIVE = 2 , MESSAGE_LLCP_ACTIVATED = 7 , MESSAGE_LLCP_DEACTIVATED = 8;
    private StringBuffer strBuffer;
    private boolean DTA_STATE,RUN_CLIENT;
    public  static boolean BACK_BUTTON_CLICKED,CHECKSNEPUIENABLED;
    private ProgressDialog loadingProgress;
    private int testCaseID;
    private CheckBox mIsShortRecordLayout;
    public static TextView clientMsg,serverMessage;
    PhMessengerService phMessengerService=new PhMessengerService();
    public static boolean SERVER_STATE;
    public static boolean CLIENT_STATE;

    public PhCustomSNEPRTD(Context context) {
        super(context);
        if (context instanceof PhDTAUIMainActivity) {
            this.mContext = context;
            setOwnerActivity((PhDTAUIMainActivity) context);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /**
         * No Title for the PopUp
         */
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        /**
         * Setting the layout for Custom Exit Dialog
         */
        CHECKSNEPUIENABLED=true;
        BACK_BUTTON_CLICKED = false;
        setContentView(R.layout.ph_custom_snep_rtd);
        DTA_STATE = true;
        if (DTA_STATE) {
            DTA_STATE=false;
            new DeInitDTA().execute();
        }
        setCanceledOnTouchOutside(false);
        runServerButton = (Button) findViewById(R.id.run_server);
        runServerButton.setOnClickListener(this);

        stopServerButton = (Button) findViewById(R.id.stop_server);
        stopServerButton.setOnClickListener(this);

        runClientButton = (Button) findViewById(R.id.run_client);
        runClientButton.setOnClickListener(this);
        runClientButton.setEnabled(false);

        stopClientButton = (Button) findViewById(R.id.stop_client);
        stopClientButton.setOnClickListener(this);

        backBtn = (Button) findViewById(R.id.back_btn);
        backBtn.setOnClickListener(this);

        testCasesSpinner = (Spinner) findViewById(R.id.spinner1);

        testCasesSpinner.setOnItemSelectedListener(this);

        clientMsg=(TextView)findViewById(R.id.client_ndef_message);
        serverMessage=(TextView)findViewById(R.id.server_ndef_message);

        mIsShortRecordLayout=(CheckBox)findViewById(R.id.short_record_layout);
        mIsShortRecordLayout.setChecked(false);

        clientClearMsgBtn=(Button)findViewById(R.id.client_clear_msg_button);
        clientClearMsgBtn.setOnClickListener(this);

        serverClearMsgBtn=(Button)findViewById(R.id.server_clear_msg_button);
        serverClearMsgBtn.setOnClickListener(this);

        strBuffer = new StringBuffer();
        mNfcAdapter   = NfcAdapter.getDefaultAdapter(this.mContext);
    }

    @Override
    public void onClick(View v) {
        boolean nfcEnable=mNfcAdapter.isEnabled();
        switch (v.getId()) {
        case R.id.run_server:
            if(!CLIENT_STATE){
            SERVER_STATE=true;
            mNfcAdapter = NfcAdapter.getDefaultAdapter(this.mContext);
            stopServerButton.setVisibility(View.VISIBLE);
            runServerButton.setVisibility(View.INVISIBLE);
            clientMsg.setText("");
            serverMessage.setText("");
            if(!nfcEnable){
            loadingProgress = ProgressDialog.show(mContext, "","Enabling Please Wait...");
            new Thread() {
                public void run() {
                    try {
                        mNfcAdapter.enable();
                        threadStart(5000);
                        snepDtaCmd("enabledta", null, 0, 0, 0, 0);
                        threadStart(500);
                        snepDtaCmd("enableserver", "urn:nfc:sn:sneptest", 13, 128, 1, 0);
                    } catch (Exception e) {
                    }
                    loadingProgress.dismiss();
                }
            }.start();
            }
            else{
                threadStart(500);
                snepDtaCmd("enableserver", "urn:nfc:sn:sneptest", 13, 128, 1, 0);
            }
            }else{
                Toast.makeText(mContext, "Please Stop Snep Client", Toast.LENGTH_SHORT).show();
            }
            break;

        case R.id.stop_server:
            SERVER_STATE=false;
            mNfcAdapter = NfcAdapter.getDefaultAdapter(this.mContext);
            stopServerButton.setVisibility(View.INVISIBLE);
            runServerButton.setVisibility(View.VISIBLE);
            serverMessage.setText("");
            clientMsg.setText("");
            stopServer(mNfcAdapter.isEnabled());
            break;

        case R.id.run_client:
            if(!SERVER_STATE){
                CLIENT_STATE=true;
            if(mIsShortRecordLayout.isChecked()){
                testCaseID=testCaseID+20;
            }
            clientMsg.setText("");
            serverMessage.setText("");
            mIsShortRecordLayout.setEnabled(false);
            RUN_CLIENT=true;
            mNfcAdapter = NfcAdapter.getDefaultAdapter(this.mContext);
            Log.e("mNfcAdapter.isEnabled()", ""+mNfcAdapter.isEnabled());
            runClientButton.setVisibility(View.INVISIBLE);
            stopClientButton.setVisibility(View.VISIBLE);
            if(!nfcEnable){
                loadingProgress = ProgressDialog.show(mContext, "","Enabling Please Wait...");
                new Thread() {
                    public void run() {
                        try {
                            mNfcAdapter.enable();
                            threadStart(5000);
                            snepDtaCmd("enabledta", null, 0, 0, 0, 0);
                            threadStart(500);
                            Log.d("testCasesID", ""+testCaseID);
                            if(testCaseID>0 && testCaseID <=6){
                                snepDtaCmd("enableclient", "urn:nfc:sn:snep", 13, 128, 1, testCaseID);
                            }else {
                                snepDtaCmd("enableclient", "urn:nfc:sn:sneptest", 13, 128, 1, testCaseID);
                            }
                        } catch (NoSuchMethodError e) {
                            e.printStackTrace();
                        }catch (NoClassDefFoundError e) {
                            e.printStackTrace();
                        }
                        loadingProgress.dismiss();
                    }
                }.start();
                }else if(!testCasesSpinner.getSelectedItem().toString().equals("Select")){
                Log.d("testCasesID", ""+testCaseID);
                threadStart(500);
                if(testCaseID>0 && testCaseID <=6){
                    snepDtaCmd("enableclient", "urn:nfc:sn:snep", 13, 128, 1, testCaseID);
                }else {
                    snepDtaCmd("enableclient", "urn:nfc:sn:sneptest", 13, 128, 1, testCaseID);
                }
            }
            }else{
                Toast.makeText(mContext, "Please Stop the Snep Server", Toast.LENGTH_SHORT).show();
            }
            break;

        case R.id.stop_client:
            CLIENT_STATE=false;
            testCaseID=testCasesSpinner.getSelectedItemPosition();
            mIsShortRecordLayout.setEnabled(true);
            RUN_CLIENT=false;
            clientMsg.setText("");
            serverMessage.setText("");
            runClientButton.setVisibility(View.VISIBLE);
            stopClientButton.setVisibility(View.INVISIBLE);
            snepDtaCmd("disableclient", null, 0, 0, 0, 0);
            stopServer(mNfcAdapter.isEnabled());
            break;

        case R.id.back_btn:
             exitDialog();
                break;
        case R.id.server_clear_msg_button:
             serverMessage.setText("");
            break;
        case R.id.client_clear_msg_button:
             clientMsg.setText("");
            break;
            }
    }
    private void stopServer(boolean nfcEnable) {
         if(nfcEnable){
             loadingProgress = ProgressDialog.show(mContext, "","Disabling Please Wait...");
             new Thread() {
                 public void run() {
                     try {
                         mNfcAdapter.disable();
                         threadStart(2000);
                         snepDtaCmd("disableserver", null, 0, 0, 0, 0);
                         threadStart(500);
                     } catch (NoSuchMethodError e) {
                         e.printStackTrace();
                     }catch (NoClassDefFoundError e) {
                         e.printStackTrace();
                     }
                     loadingProgress.dismiss();
                 }
             }.start();
             }
    }
    AlertDialog.Builder alertBuilder;
    public void exitDialog(){
        alertBuilder=new AlertDialog.Builder(mContext);
        alertBuilder.setMessage("Do you want to exit from SNEP testing and \n go back to main menu?");
        alertBuilder.setPositiveButton("Yes", new OnClickListener() {
        @Override
        public void onClick(DialogInterface arg0, int arg1) {
                dismissTheDialog();
            }
        });
        alertBuilder.setNegativeButton("No", new OnClickListener() {
        @Override
        public void onClick(DialogInterface arg0, int arg1) {
        }
        });
        alertBuilder.create();
        alertBuilder.show();
    }

    public void dismissTheDialog(){
        mNfcAdapter = NfcAdapter.getDefaultAdapter(this.mContext);
        BACK_BUTTON_CLICKED = true;
        PhDTAUIMainActivity.clearP2PCheckBox.setChecked(false);
        PhDTAUIMainActivity.snepCheckBox.setChecked(false);
        PhDTAUIMainActivity.llcpCheckBox.setChecked(false);
        Log.d("mNfcAdapter.isEnabled()", ""+mNfcAdapter.isEnabled());
        if(mNfcAdapter.isEnabled()){
            try {
                mNfcAdapter.disable();
            } catch (NoSuchMethodError e) {
                e.printStackTrace();
            }catch (NoClassDefFoundError e) {
                e.printStackTrace();
            }
        }
        if (!DTA_STATE) {
            loadingProgress = ProgressDialog.show(mContext, "","Please Wait...");
            new Thread() {
                public void run() {
                    try {
                        dismiss();
                    } catch (Exception e) {
                        Log.e("tag", e.getMessage());
                    }
                    loadingProgress.dismiss();
                }
            }.start();
    }
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(event.KEYCODE_BACK==keyCode){
            exitDialog();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onNdefPushComplete(NfcEvent event) {
        mHandler.obtainMessage(MESSAGE).sendToTarget();
        Log.w(TAG, "sending message "
                + mHandler.obtainMessage(MESSAGE).toString());
    }

    @Override
    public NdefMessage createNdefMessage(NfcEvent arg0) {
        String text = ("Beam me up!\n\n" + "Beam Time: ");
        NdefMessage msg = new NdefMessage(new NdefRecord[] { createMimeRecord(
                "application/com.example.android.beam", text.getBytes()) });
        return msg;
    }

    public NdefRecord createMimeRecord(String mimeType, byte[] payload) {
        byte[] mimeBytes = mimeType.getBytes(Charset.forName("US-ASCII"));
        NdefRecord mimeRecord = new NdefRecord(NdefRecord.TNF_MIME_MEDIA,
                mimeBytes, new byte[0], payload);
        return mimeRecord;
    }

    /** This handler receives a message from onNdefPushComplete */
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE: {
                Toast.makeText(mContext, "Message sent!", Toast.LENGTH_SHORT)
                        .show();
                break;
            }
            case MESSAGE_RECEIVE: {
                Toast.makeText(mContext, "Message Received!",
                        Toast.LENGTH_SHORT).show();
                // only one message sent during the beam
                NdefMessage ndefMsg = (NdefMessage) msg.obj;
                // record 0 contains the MIME type, record 1 is the AAR, if
                // present
                byte[] b_msg = ndefMsg.getRecords()[0].getPayload();
                strBuffer = new StringBuffer();
                strBuffer.append("Receive: ");
                strBuffer.append("legnth = " + b_msg.length);
                strBuffer.append('\n');
                strBuffer.append(new String(b_msg));
                strBuffer.append('\n');
                strBuffer.append("[" + toHexString(b_msg, 0, b_msg.length)
                        + "]");
                strBuffer.append('\n');
                break;
            }
            case MESSAGE_LLCP_ACTIVATED: {
                Toast.makeText(mContext, "LLCP activated!", Toast.LENGTH_SHORT)
                        .show();
                break;
            }
            case MESSAGE_LLCP_DEACTIVATED: {
                Toast.makeText(mContext, "LLCP deactivated!",
                        Toast.LENGTH_SHORT).show();
                break;
            }
            }
        }
    };
    private String toHexString(byte[] buffer, int offset, int length) {
        final char[] HEX_CHARS = "0123456789abcdef".toCharArray();
        char[] chars = new char[3 * length];
        for (int j = offset; j < offset + length; ++j) {
            chars[3 * j] = HEX_CHARS[(buffer[j] & 0xF0) >>> 4];
            chars[3 * j + 1] = HEX_CHARS[buffer[j] & 0x0F];
            chars[3 * j + 2] = ' ';
        }
        return new String(chars);
    }

    public class DeInitDTA extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            return null;
        }
        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            try {
                if(PhUtilities.NDK_IMPLEMENTATION==true){
                    PhNXPJniHelper phNXPJniHelper = new PhNXPJniHelper();
                    phNXPJniHelper.phDtaLibDeInit();
                }
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
            } catch (NoClassDefFoundError e) {
                e.printStackTrace();
            } catch (NoSuchMethodError e) {
                e.printStackTrace();
            }

        }
        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
        }
    }
    @Override
    public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
            long arg3) {
        testCaseID=testCasesSpinner.getSelectedItemPosition();
        Log.d("testCasesSpinner.getSelectedItem().toString()", ""+testCasesSpinner.getSelectedItem().toString());
        if(testCasesSpinner.getSelectedItem().equals(getContext().getResources().getString(R.string.select_client_test_case))){
            runClientButton.setEnabled(false);
        }
        else if (testCaseID > 0 && testCaseID < 7) {
            if(RUN_CLIENT){
                RUN_CLIENT=false;
                snepDtaCmd("disableclient", null, 0, 0, 0, 0);
                stopServer(mNfcAdapter.isEnabled());
            }
            testCasesSpinner.getSelectedItemPosition();
            stopClientButton.setVisibility(View.INVISIBLE);
            runClientButton.setVisibility(View.VISIBLE);
            runClientButton.setEnabled(true);
            Log.d(PhUtilities.UI_TAG, "<7" + testCaseID);
        } else if (testCaseID > 6 && testCaseID <= 9) {
            if(RUN_CLIENT){
                RUN_CLIENT=false;
                snepDtaCmd("disableclient", null, 0, 0, 0, 0);
                stopServer(mNfcAdapter.isEnabled());
            }
            Log.d(PhUtilities.UI_TAG, ">7" + testCaseID);
            runClientButton.setEnabled(true);
            stopClientButton.setVisibility(View.INVISIBLE);
            runClientButton.setVisibility(View.VISIBLE);
        }
    }

    public void snepDtaCmd(String enableclient, String serviceName, int miu,
            int rwSize, int serialNumber, int testCaseId) {
        try{
            /*mNfcAdapter.snepDtaCmd(enableclient, serviceName, miu,rwSize, serialNumber, testCaseID);*/
            com.nxp.nfc.NxpNfcAdapter mNxpNfcAdapter= com.nxp.nfc.NxpNfcAdapter.getNxpNfcAdapter(mNfcAdapter);
            com.nxp.nfc.NfcDta mNfcDta = mNxpNfcAdapter.createNfcDta();
            mNfcDta.snepDtaCmd(enableclient, serviceName, miu,rwSize, serialNumber, testCaseID);
        }catch (NoSuchMethodError e) {
           e.printStackTrace();
        }catch (NoClassDefFoundError e) {
           e.printStackTrace();
        }
    }
    @Override
    public void onNothingSelected(AdapterView<?> arg0) {

    }
    public void threadStart(long sleep) {
        try {
            Thread.sleep(sleep);
        } catch (InterruptedException e) {
        }
    }
}
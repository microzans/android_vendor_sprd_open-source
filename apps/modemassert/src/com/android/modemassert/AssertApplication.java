package com.android.modemassert;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.text.TextUtils;
import android.util.Log;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;

public class AssertApplication extends Application {
    private final String MTAG = "AssertApplication";

    //Action for assert broadcast
    private static final String MODEM_STAT_CHANGE = "com.android.modemassert.MODEM_STAT_CHANGE";
    //extra data key in intent for assert broadcast
    private static final String MODEM_STAT = "modem_stat";
    //SPRD: add modem assert reason for PhoneInfo feature
    private static final String MODEM_INFO= "modem_info";
    //values of extra data in intent.
    private static final String MODEM_ALIVE = "modem_alive";
    private static final String MODEM_ASSERT = "modem_assert";
    //name of socket to listen to modem state
    private static final String MODEM_SOCKET_NAME = "modemd";

    //notification id to cancel
    private static final int MODEM_ASSERT_ID = 1;
    private static final int WCND_ASSERT_ID = 2;
    private static final int MODEM_BLOCK_ID = 3;

    private static final int BUF_SIZE = 128;

    private HandlerThread mAssertThread;
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(MTAG, "onCreate()...");
        mAssertThread = new HandlerThread("assertHandlerThread");
        mAssertThread.start();
        /* a receiver to receive wcnd assert info */
        IntentFilter filter = new IntentFilter();
        filter.addAction(WCND_CP2_STATE_CHANGED_ACTION);
        filter.addAction(SLOG_DUMP_START_ACTION);
        filter.addAction(SLOG_DUMP_END_ACTION);
        registerReceiver(mReceiver, filter);
    }
    /**
     * Broadcast intent action indicating that CP2 status has been assert,
     * alive,
     */
    private static final String WCND_CP2_STATE_CHANGED_ACTION = "com.android.server.wcn.WCND_CP2_STATE_CHANGED";
    private static final String SLOG_DUMP_START_ACTION = "slogui.intent.action.DUMP_START";
    private static final String SLOG_DUMP_END_ACTION = "slogui.intent.action.DUMP_END";

    /**
     * The lookup key for an {@code boolean} giving if CP2 is OK.
     */
    private static final String EXTRA_IS_CP2_OK = "isCP2OK";
    /**
     * The lookup key for an {@code String} giving the CP2 assert info.
     */
    public static final String EXTRA_CP2_ASSERT_INFO = "CP2AssertInfo";
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (WCND_CP2_STATE_CHANGED_ACTION.equals(action)) {
                boolean cp2ok = (boolean) intent.getBooleanExtra(EXTRA_IS_CP2_OK, false);
                String assertInfo = intent.getStringExtra(EXTRA_CP2_ASSERT_INFO);
                Log.d(MTAG, "CP2 status: " + cp2ok + ", info: " + assertInfo);
                if (!cp2ok) {
                    Log.d(MTAG, "show assert notification for wcnd.");
                    showNotification(WCND_ASSERT_ID, "wcnd assert", assertInfo);
                } else if (cp2ok) {
                    hideNotification(WCND_ASSERT_ID);
                }
            } else if (SLOG_DUMP_START_ACTION.equals(action)) {
                showProgressDialog();
            } else if (SLOG_DUMP_END_ACTION.equals(action)) {
                Log.d(MTAG, "SLOG_DUMP_END_ACTION");
                Intent closeIntent = new Intent(context, SystemInfoDumpingActivity.class);
                closeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                closeIntent.putExtra("closeFlag", true);
                startActivity(closeIntent);
            }
        }
    };

    private void showNotification(int notificationId, String title, String info) {
        Log.v(MTAG, "show assert Notefication.");
        int icon = R.drawable.modem_assert;
        NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        long when = System.currentTimeMillis();
        Notification notification = new Notification(icon, info, when);

        Context context = getApplicationContext();
        CharSequence contentTitle = title;
        CharSequence contentText = info;
        /** modify 145779 add show assertinfo page **/
        Intent notificationIntent = new Intent(this, AssertInfoActivity.class);
        notificationIntent.putExtra("assertInfo", info);
        PendingIntent contentIntent =  PendingIntent.getActivity(context, 0, notificationIntent, 0);

        //notification.defaults |= Notification.DEFAULT_VIBRATE;
        long[] vibrate = {0, 10000};
        notification.vibrate = vibrate;
        notification.flags |= Notification.FLAG_NO_CLEAR;// no clear
        /** modify 145779 add show assertinfo page **/
        notification.defaults |= Notification.DEFAULT_SOUND;
        //notification.sound = Uri.parse("file:///sdcard/assert.mp3");

        notification.setLatestEventInfo(context, contentTitle, contentText, contentIntent);
        manager.notify(notificationId, notification);
    }

    private void hideNotification(int notificationId) {
        Log.v(MTAG, "hideNotification");
        NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        manager.cancel(notificationId);
    }

    private void sendModemStatBroadcast(String modemStat,String info) {
        Log.d(MTAG, "sendModemStatBroadcast : " + modemStat);
        Intent intent = new Intent(MODEM_STAT_CHANGE);
        intent.putExtra(MODEM_STAT, modemStat);
        //SPRD: add modem assert reason for PhoneInfo feature
        intent.putExtra(MODEM_INFO, info);
        sendBroadcast(intent);
    }

    private void showProgressDialog() {
       Intent intent = new Intent(this, SystemInfoDumpingActivity.class);
       intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
       intent.putExtra("closeFlag", false);
       startActivity(intent);
    }

}

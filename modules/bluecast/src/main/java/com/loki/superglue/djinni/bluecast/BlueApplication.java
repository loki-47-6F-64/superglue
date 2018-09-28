package com.loki.superglue.djinni.bluecast;

import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;

public class BlueApplication extends Application {
    public final static String CHANNEL_ID = "BlueApplication";
    public final static int NOTIFICATION_ID = 321;

    private BroadcastReceiver receiver;

    @Override
    public void onCreate() {
        super.onCreate();

        createNotificationChannel();
        SuperGlueBlueCast.init(this);

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int newState = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.STATE_OFF);

                if(newState == BluetoothAdapter.STATE_TURNING_OFF || newState == BluetoothAdapter.STATE_TURNING_ON) {
                    return;
                }

                SuperGlueBlueCast.getBluetooth().getBluetoothCallback().onBluePowerStateChange(Bluetooth.fromPowerState(newState));
            }
        };

        registerReceiver(receiver, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
    }

    @Override
    public void onTerminate() {
        unregisterReceiver(receiver);
        super.onTerminate();
    }

    private void createNotificationChannel() {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.channel_name);
            String description = getString(R.string.channel_description);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
            channel.setDescription(description);
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }
}

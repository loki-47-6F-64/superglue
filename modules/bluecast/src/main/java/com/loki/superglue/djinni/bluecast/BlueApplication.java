package com.loki.superglue.djinni.bluecast;

import android.app.Application;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public class BlueApplication extends Application {
    private BroadcastReceiver receiver;

    @Override
    public void onCreate() {
        super.onCreate();

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
}

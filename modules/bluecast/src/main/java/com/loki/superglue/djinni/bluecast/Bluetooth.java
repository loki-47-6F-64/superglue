package com.loki.superglue.djinni.bluecast;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.RemoteException;
import android.util.Log;

import com.loki.superglue.djinni.bluecast.gatt.GattBind;
import com.loki.superglue.djinni.bluecast.gatt.GattCharacteristicBind;
import com.loki.superglue.djinni.jni.BlueBeacon;
import com.loki.superglue.djinni.jni.BlueCallback;
import com.loki.superglue.djinni.jni.BlueCastInterface;
import com.loki.superglue.djinni.jni.BlueController;
import com.loki.superglue.djinni.jni.BlueDevice;
import com.loki.superglue.djinni.jni.BlueGattConnectionState;
import com.loki.superglue.djinni.jni.BluePowerState;
import com.loki.superglue.djinni.jni.BlueScanResult;

import org.altbeacon.beacon.Beacon;
import org.altbeacon.beacon.BeaconConsumer;
import org.altbeacon.beacon.BeaconManager;
import org.altbeacon.beacon.BeaconParser;
import org.altbeacon.beacon.Region;
import org.altbeacon.beacon.service.ArmaRssiFilter;

import java.util.Collection;
import java.util.UUID;

public class Bluetooth extends BluetoothGattCallback {
    private static final String TAG = "bluecast.Bluetooth";

    private static final String IBEACON_LAYOUT = "m:0-3=4c000215,i:4-19,i:20-21,i:22-23,p:24-24";

    private BeaconManager beaconManager;
    private BeaconConsumer beaconConsumer;

    private BluetoothAdapter btAdap;

    private BlueCallback blCall;
    private ScanCallback scanCall;

    private Context context;

    public Bluetooth(Context context) {
        this.context = context;

        btAdap = BluetoothAdapter.getDefaultAdapter();
        scanCall = scanCallback();

        blCall = BlueCastInterface.config(blueController());

        beaconManager = BeaconManager.getInstanceForApplication(context);
        BeaconManager.setRssiFilterImplClass(ArmaRssiFilter.class);
        beaconManager.getBeaconParsers().add(new BeaconParser().setBeaconLayout(IBEACON_LAYOUT));

        beaconConsumer = beaconConsumerImpl();
    }

    private BlueController blueController() {
        return new BlueController() {
            @Override
            public boolean isEnabled() {
                return btAdap.isEnabled();
            }

            @Override
            public void connectGatt(BlueDevice dev) {
                BluetoothDevice device = btAdap.getRemoteDevice(dev.getAddress());
                device.connectGatt(context, false, Bluetooth.this);
            }

            @Override
            public void peripheralScan(boolean enable) {
                BluetoothLeScanner bleScanner = btAdap.getBluetoothLeScanner();

                if(enable) {
                    bleScanner.startScan(scanCall);
                }
                else {
                    bleScanner.stopScan(scanCall);
                }
            }

            @Override
            public void beaconScan(boolean enable) {
                if(enable && !beaconManager.isBound(beaconConsumer)) {
                    beaconManager.bind(beaconConsumer);
                }
                else if(!enable && beaconManager.isBound(beaconConsumer)){
                    beaconManager.unbind(beaconConsumer);
                }
            }
        };
    }

    private BlueScanResult fromScanResult(ScanResult res) {
        BluetoothDevice dev = res.getDevice();

        return new BlueScanResult(
                new BlueDevice(dev.getName(), dev.getAddress()),
                res.getRssi()
        );
    }

    private ScanCallback scanCallback() {
        return new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                super.onScanResult(callbackType, result);

                blCall.onScanResult(fromScanResult(result));
            }
        };
    }

    private static BlueGattConnectionState fromGattState(int newState) {
        switch (newState) {
            case BluetoothGatt.STATE_DISCONNECTING:
                return BlueGattConnectionState.DISCONNECTED;
            case BluetoothGatt.STATE_CONNECTED:
                return BlueGattConnectionState.CONNECTED;
        }

        return BlueGattConnectionState.DISCONNECTED;
    }

    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
        blCall.onGattConnectionStateChange(new GattBind(gatt), fromGattState(newState));
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        blCall.onGattServicesDiscovered(new GattBind(gatt), status == BluetoothGatt.GATT_SUCCESS);
    }

    @Override
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
        blCall.onCharacteristicRead(new GattBind(gatt), new GattCharacteristicBind(characteristic), status == BluetoothGatt.GATT_SUCCESS);
    }


    private BeaconConsumer beaconConsumerImpl() {
        return new BeaconConsumer() {
            @Override
            public void onBeaconServiceConnect() {
                beaconManager.addRangeNotifier((Collection<Beacon> beacons, Region region) -> {
                        for(Beacon beacon : beacons) {
                            UUID uuid = beacon.getId1().toUuid();
                            int major = beacon.getId2().toInt();
                            int minor = beacon.getId3().toInt();

                            BluetoothDevice device = btAdap.getRemoteDevice(beacon.getBluetoothAddress());
                            blCall.onBeaconUpdate(new BlueBeacon(
                                    uuid.toString(),
                                    major,
                                    minor,
                                    beacon.getDistance()
                            ));
                        }
                });
                try {
                    Region region = new Region("ibeacon", null, null, null);
                    beaconManager.startRangingBeaconsInRegion(region);
                } catch (RemoteException e) {
                    Log.d(TAG, e.getMessage());
                    e.printStackTrace();
                }
            }

            @Override
            public Context getApplicationContext() {
                return context.getApplicationContext();
            }

            @Override
            public void unbindService(ServiceConnection serviceConnection) {
                context.unbindService(serviceConnection);
            }

            @Override
            public boolean bindService(Intent intent, ServiceConnection serviceConnection, int i) {
                return context.bindService(intent, serviceConnection, i);
            }
        };
    }


    public BlueCallback getBluetoothCallback() {
        return blCall;
    }

    public static BluePowerState fromPowerState(int state) {
        switch (state) {
            case BluetoothAdapter.STATE_OFF:
                return BluePowerState.OFF;
            case BluetoothAdapter.STATE_ON:
                return BluePowerState.ON;
        }

        return BluePowerState.OFF;
    }
}
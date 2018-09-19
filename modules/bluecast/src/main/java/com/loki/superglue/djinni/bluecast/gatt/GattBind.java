package com.loki.superglue.djinni.bluecast.gatt;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattService;

import com.loki.superglue.djinni.jni.BlueGatt;
import com.loki.superglue.djinni.jni.BlueGattCharacteristic;
import com.loki.superglue.djinni.jni.BlueGattService;

import java.util.ArrayList;
import java.util.List;

public class GattBind extends BlueGatt {
    private BluetoothGatt gatt;

    public GattBind(BluetoothGatt gatt) {
        this.gatt = gatt;
    }

    @Override
    public void disconnect() {
        gatt.disconnect();
    }

    @Override
    public boolean discoverServices() {
        return gatt.discoverServices();
    }

    @Override
    public void readCharacteristic(BlueGattCharacteristic characteristic) {
        gatt.readCharacteristic(((GattCharacteristicBind)characteristic).getCharacteristic());
    }

    @Override
    public void close() {
        gatt.close();
    }

    @Override
    public ArrayList<BlueGattService> services() {
        List<BluetoothGattService> gattServicesOut = gatt.getServices();

        ArrayList<BlueGattService> tempServicesIn = new ArrayList<>(gattServicesOut.size());

        for(BluetoothGattService gatt_service : gattServicesOut) {
            tempServicesIn.add(new GattServiceBind(gatt_service));
        }

        return tempServicesIn;
    }
}

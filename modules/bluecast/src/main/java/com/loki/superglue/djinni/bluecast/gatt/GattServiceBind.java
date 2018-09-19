package com.loki.superglue.djinni.bluecast.gatt;

import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;

import com.loki.superglue.djinni.bluecast.gatt.GattCharacteristicBind;
import com.loki.superglue.djinni.jni.BlueGattCharacteristic;
import com.loki.superglue.djinni.jni.BlueGattService;

import java.util.ArrayList;
import java.util.List;

public class GattServiceBind extends BlueGattService {
    private BluetoothGattService service;

    public GattServiceBind(BluetoothGattService service) {
        this.service = service;
    }

    @Override
    public String uuid() {
        return service.getUuid().toString();
    }

    @Override
    public ArrayList<BlueGattCharacteristic> characteristics() {
        List<BluetoothGattCharacteristic> gattCharacteristicOut = service.getCharacteristics();

        ArrayList<BlueGattCharacteristic> gattCharacteristicIn = new ArrayList<>(gattCharacteristicOut.size());

        for(BluetoothGattCharacteristic gatt_characteristic : gattCharacteristicOut) {
            gattCharacteristicIn.add(new GattCharacteristicBind(gatt_characteristic));
        }

        return gattCharacteristicIn;
    }
}

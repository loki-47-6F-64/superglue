package com.loki.superglue.djinni.bluecast.gatt;

import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;

import com.loki.superglue.djinni.bluecast.gatt.DescriptorBind;
import com.loki.superglue.djinni.jni.BlueGattCharacteristic;
import com.loki.superglue.djinni.jni.BlueGattDescriptor;

import java.util.ArrayList;
import java.util.List;

public class GattCharacteristicBind extends BlueGattCharacteristic {
    private BluetoothGattCharacteristic characteristic;

    public GattCharacteristicBind(BluetoothGattCharacteristic characteristic) {
        this.characteristic = characteristic;
    }

    @Override
    public String uuid() {
        return characteristic.getUuid().toString();
    }

    @Override
    public byte[] getValue() {
        return characteristic.getValue();
    }

    @Override
    public String getStringValue(int offset) {
        return characteristic.getStringValue(offset);
    }

    @Override
    public ArrayList<BlueGattDescriptor> descriptors() {
        List<BluetoothGattDescriptor> descriptorsOut = characteristic.getDescriptors();

        ArrayList<BlueGattDescriptor> descriptorsIn = new ArrayList<>(descriptorsOut.size());

        for(BluetoothGattDescriptor descriptor : descriptorsOut) {
            descriptorsIn.add(new DescriptorBind(descriptor));
        }

        return descriptorsIn;
    }

    public BluetoothGattCharacteristic getCharacteristic() {
        return characteristic;
    }
}

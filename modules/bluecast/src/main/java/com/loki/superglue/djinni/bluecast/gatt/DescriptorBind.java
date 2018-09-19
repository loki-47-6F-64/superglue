package com.loki.superglue.djinni.bluecast.gatt;

import android.bluetooth.BluetoothGattDescriptor;

import com.loki.superglue.djinni.jni.BlueGattDescriptor;

public class DescriptorBind extends BlueGattDescriptor {
    private BluetoothGattDescriptor descriptor;

    public DescriptorBind(BluetoothGattDescriptor descriptor) {
        this.descriptor = descriptor;
    }

    @Override
    public String uuid() {
        return descriptor.getUuid().toString();
    }
}

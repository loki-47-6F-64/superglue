package com.loki.superglue.djinni.bluecast;

import android.content.Context;

import com.loki.superglue.djinni.common.SuperGlueCommon;

public class SuperGlueBlueCast {
    private static boolean loaded = false;

    private static Bluetooth bluetooth;

    public static Bluetooth getBluetooth() {
        return bluetooth;
    }

    public static void init(Context ctx) {
        if(!loaded) {
            loaded = true;

            SuperGlueCommon.init(ctx);

            bluetooth = new Bluetooth(ctx);
        }
    }
}

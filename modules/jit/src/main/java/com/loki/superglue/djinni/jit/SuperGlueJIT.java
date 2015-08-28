package com.loki.superglue.djinni.jit;

import android.content.Context;

import com.loki.superglue.djinni.common.SuperGlueCommon;

/**
 * Created by loki on 25-8-15.
 */
public class SuperGlueJIT {
    private static boolean loaded = false;

    public static void init(final Context ctx) {
        if(!loaded) {
            SuperGlueCommon.init(ctx);

            loaded = true;
        }
    }
}

package com.loki.superglue.djinni.common;

import android.content.Context;
import android.util.Log;

import com.loki.superglue.djinni.jni.CommonInterface;
import com.loki.superglue.djinni.jni.FileInterface;
import com.loki.superglue.djinni.jni.LogInterface;
import com.loki.superglue.djinni.jni.LogSeverity;
import com.loki.superglue.djinni.jni.ThreadCallback;
import com.loki.superglue.djinni.jni.ThreadInterface;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Created by loki on 18-5-15.
 */
public class SuperGlueCommon {
    private static boolean loaded = false;
    static {
        System.loadLibrary("superglue");
    }

    public static void init(final Context ctx) {
        if(!loaded) {
            CommonInterface.init(logImpl(), threadImpl(), fileImpl(ctx));
            loaded = true;
        }
    }

    private static LogInterface logImpl() {
        return new LogInterface() {
            @Override
            public void log(LogSeverity logSeverity, String s) {
                final String superglue = "SuperGlue";
                switch (logSeverity) {
                    case DEBUG:
                        Log.d(superglue, s);
                        break;
                    case INFO:
                        Log.i(superglue, s);
                        break;
                    case WARN:
                        Log.w(superglue, s);
                        break;
                    case ERROR:
                        Log.e(superglue, s);
                }
            }
        };
    }

    private static ThreadInterface threadImpl() {
        return new ThreadInterface() {
            @Override
            public void create(final ThreadCallback threadCallback) {
                new Thread(() -> threadCallback.run()).start();
            }
        };
    }

    private static FileInterface fileImpl(final Context ctx) {
        return new FileInterface() {
            @Override
            public String resourceTxt(String fileName) {
                return new String(resourceBin(fileName));
            }

            @Override
            public byte[] resourceBin(String fileName) {
                InputStream in = null;
                try {
                    in = ctx.getAssets().open(fileName);
                } catch (IOException e) {
                    e.printStackTrace();

                    return new byte[0];
                }

                ByteArrayOutputStream out = new ByteArrayOutputStream();
                try {
                    byte[] buf = new byte[1024];
                    int len;
                    while ((len = in.read(buf)) > 0) {
                        out.write(buf, 0, len);
                    }

                    in.close();
                } catch (IOException e) {
                    e.printStackTrace();

                    return new byte[0];
                }
                return out.toByteArray();
            }

            @Override
            public String homeDir() {
                return ctx.getFilesDir().getAbsolutePath() + "/";
            }
        };
    }
}

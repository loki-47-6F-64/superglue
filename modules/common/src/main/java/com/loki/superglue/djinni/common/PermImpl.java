package com.loki.superglue.djinni.common;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

import com.loki.superglue.djinni.jni.Permission;
import com.loki.superglue.djinni.jni.PermissionCallback;
import com.loki.superglue.djinni.jni.PermissionInterface;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

public class PermImpl extends PermissionInterface {
    // For some reason REQUEST_PERMISSION = Integer.MAX_VALUE doesn't causes a pop-up for requesting permissions
    private static int REQUEST_PERMISSION = 100;

    private Activity activity;

    private ConcurrentMap<String, PermissionCallback> permissionCallbackMap;

    public PermImpl(Activity activity) {
        this.activity = activity;

        permissionCallbackMap = new ConcurrentHashMap<>();
    }

    @Override
    public boolean has(Permission perm) {
        return ContextCompat.checkSelfPermission(activity, fromPermission(perm)) == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    public void request(Permission perm, final PermissionCallback f) {
        if(has(perm)) {
            f.result(perm, true);

            return;
        }

        String permString = fromPermission(perm);
        permissionCallbackMap.put(permString, f);

        ActivityCompat.requestPermissions(activity, new String[] { permString }, REQUEST_PERMISSION);
    }

    @NonNull
    private String fromPermission(Permission perm) {
        switch(perm) {
            case BLUETOOTH:
                return Manifest.permission.BLUETOOTH;
            case BLUETOOTH_ADMIN:
                return Manifest.permission.BLUETOOTH_ADMIN;
            case COARSE_LOCATION:
                return Manifest.permission.ACCESS_COARSE_LOCATION;
            default:
                return Manifest.permission.ACCESS_COARSE_LOCATION;
        }
    }

    private Permission toPermission(String perm) {
        switch(perm) {
            case Manifest.permission.BLUETOOTH:
                return Permission.BLUETOOTH;
            case Manifest.permission.BLUETOOTH_ADMIN:
                return Permission.BLUETOOTH_ADMIN;
            case Manifest.permission.ACCESS_COARSE_LOCATION:
                return Permission.COARSE_LOCATION;
            default:
                return Permission.COARSE_LOCATION;
        }
    }

    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if(requestCode != REQUEST_PERMISSION || !(permissions.length > 0)) {
            return;
        }

        PermissionCallback f = permissionCallbackMap.remove(permissions[0]);
        f.result(toPermission(permissions[0]), grantResults[0] == PackageManager.PERMISSION_GRANTED);
    }
}

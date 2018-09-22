package com.loki.superglue.djinni.common.activity;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;

import com.loki.superglue.djinni.common.PermImpl;
import com.loki.superglue.djinni.jni.PermissionInterface;

public class CommonActivity extends AppCompatActivity {
    PermImpl perm;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        perm = new PermImpl(this);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        perm.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    public PermissionInterface getPermissionInterface() {
        return perm;
    }
}

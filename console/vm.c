#include "vm.h"

/**
Copyright 2015 Tim 'diff' Strazzere <strazz@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

/**
 * COPIED from -- https://github.com/rednaga/native-shim
 */

JavaVM *init_jvm(JavaVM **p_vm, JNIEnv **p_env) {
  //https://android.googlesource.com/platform/frameworks/native/+/ce3a0a5/services/surfaceflinger/DdmConnection.cpp
  JavaVMOption opt[4];
  opt[0].optionString = "-Djava.class.path=/data/local/tmp/target-app.apk";
  opt[1].optionString = "-agentlib:jdwp=transport=dt_android_adb,suspend=n,server=y";
  opt[2].optionString = "-Djava.library.path=/data/local/tmp:/system/lib:/system/lib64";
  opt[3].optionString = "-verbose:jni"; // may want to remove this, it's noisy

  // Add this option if you're hacking stuff and need it, not normally required
  // opt[4].optionString = "-Xno-sig-chain"; // may not be require prior to ART vm, may even cause issues for DVM

  JavaVMInitArgs args;
  args.version = JNI_VERSION_1_6;
  args.options = opt;
  args.nOptions = 4; // Uptick this to 5, it will pass in the no-sig-chain option
  args.ignoreUnrecognized = JNI_FALSE;

  void *libdvm_dso = dlopen("libdvm.so", RTLD_NOW);
  void *libandroid_runtime_dso = dlopen("libandroid_runtime.so", RTLD_NOW);

  if (!libdvm_dso) {
    libdvm_dso = dlopen("libart.so", RTLD_NOW);
  }

  if (!libdvm_dso || !libandroid_runtime_dso) {
    return NULL;
  }

  JNI_CreateJavaVM_t JNI_CreateJavaVM;
  JNI_CreateJavaVM = (JNI_CreateJavaVM_t) dlsym(libdvm_dso, "JNI_CreateJavaVM");
  if (!JNI_CreateJavaVM) {
    return NULL;
  }

  registerNatives_t registerNatives;
  registerNatives = (registerNatives_t) dlsym(libandroid_runtime_dso, "Java_com_android_internal_util_WithFramework_registerNatives");
  if (!registerNatives) {
    // Attempt non-legacy version
    registerNatives = (registerNatives_t) dlsym(libandroid_runtime_dso, "registerFrameworkNatives");
    if(!registerNatives) {
      return NULL;
    }
  }

  if (JNI_CreateJavaVM(&(*p_vm), &(*p_env), &args)) {
    return NULL;
  }

  if (registerNatives(*p_env, 0)) {
    return NULL;
  }

  return *p_vm;
}

// linker flags needed :: -Wl,--export-dynamic
// Include all of the Android's libsigchain symbols
// libsigchain calls abort()
JNIEXPORT void InitializeSignalChain() {}
JNIEXPORT void ClaimSignalChain() {}
JNIEXPORT void UnclaimSignalChain() {}
JNIEXPORT void InvokeUserSignalHandler() {}
JNIEXPORT void EnsureFrontOfChain() {}
JNIEXPORT void AddSpecialSignalHandlerFn() {}
JNIEXPORT void RemoveSpecialSignalHandlerFn() {}

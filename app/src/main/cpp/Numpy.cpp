#include <iostream>
#include <math.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stdlib.h>

#include <jni.h>
#include <android/log.h>

using namespace std;
//
//#define LOGTAG "Numpy.cpp"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOGTAG, __VA_ARGS__)
//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, __VA_ARGS__)
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOGTAG, __VA_ARGS__)

static JavaVM *gVm = NULL;
static jobject gOjb = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
//    LOGD("JNI_OnLoad");
    gVm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
//    LOGD("JNI_OnUnload");
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
}
#include <iostream>
#include <math.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stdlib.h>

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

using namespace std;

#define LOGTAG "ImageGraphProcessing.cpp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOGTAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOGTAG, __VA_ARGS__)

static JavaVM *gVm = NULL;
static jobject gOjb = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnLoad");
    gVm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnUnload");
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
}

static void toGrayScale(AndroidBitmapInfo *info, void *pixels);

static void toBinary(AndroidBitmapInfo *info, void *pixels);

static void setContrast(AndroidBitmapInfo *info, void *pixels, jint value);

static int **applyKernel(JNIEnv *env, AndroidBitmapInfo *info, void *pixels, jobjectArray kernel);

static int **convFull(int **convMatrix, int **kernelMatrix, int n, int m, int kn, int km);

static jobject getVertexes(JNIEnv *env, AndroidBitmapInfo *info, void *pixels);

static jobject connectedComponents(JNIEnv *env, jobject points);

static void dfs(JNIEnv *env, int u, jobject points, bool *freeVector, int n, jobject res);

#ifdef __cplusplus
extern "C" {
#endif

void
Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_convertToGrayScale(JNIEnv *env,
                                                                                    jobject,
                                                                                    jobject srcBitmap) {
    // initialize
    AndroidBitmapInfo info;
    void *pixels;

    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return;
    }

    AndroidBitmap_getInfo(env, srcBitmap, &info);
    LOGD("width: %d, height: %d, format: %d, stride: %d", info.width, info.height,
         info.format, info.stride);
    LOGD("AndroidBitmap_lockPixels");

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888");
        return;
    }

    toGrayScale(&info, pixels);

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return;
    }
}

void Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_convertToBinary(JNIEnv *env,
                                                                                      jobject,
                                                                                      jobject srcBitmap) {
    // initialize
    AndroidBitmapInfo info;
    void *pixels;

    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return;
    }

    AndroidBitmap_getInfo(env, srcBitmap, &info);
    LOGD("width: %d, height: %d, format: %d, stride: %d", info.width, info.height,
         info.format, info.stride);
    LOGD("AndroidBitmap_lockPixels");

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888");
        return;
    }

    toBinary(&info, pixels);

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return;
    }
}

void Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_setContrast(JNIEnv *env,
                                                                                  jobject,
                                                                                  jobject srcBitmap,
                                                                                  jint value) {
    // initialize
    AndroidBitmapInfo info;
    void *pixels;

    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return;
    }

    AndroidBitmap_getInfo(env, srcBitmap, &info);
    LOGD("width: %d, height: %d, format: %d, stride: %d", info.width, info.height,
         info.format, info.stride);
    LOGD("AndroidBitmap_lockPixels");

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888");
        return;
    }

    setContrast(&info, pixels, value);

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return;
    }
}

void Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_applyKernel(JNIEnv *env,
                                                                                  jobject,
                                                                                  jobject srcBitmap,
                                                                                  jobjectArray kernel) {
    // initialize
    AndroidBitmapInfo info;
    void *pixels;
    int **resMatrix;

    // STEP 1
    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return;
    }

    AndroidBitmap_getInfo(env, srcBitmap, &info);
    LOGD("width: %d, height: %d, format: %d, stride: %d", info.width, info.height,
         info.format, info.stride);
    LOGD("AndroidBitmap_lockPixels");

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888");
        return;
    }

    resMatrix = applyKernel(env, &info, pixels, kernel);

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return;
    }

    // STEP 2
    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return;
    }

    int yy;
    for (yy = 0; yy < info.height; yy++) {
        uint32_t *line = (uint32_t *) pixels;
        int xx;
        for (xx = 0; xx < info.width; xx++) {
            uint32_t *pixel = &line[xx];
            int *value = &resMatrix[yy][xx];
            if (*value < 0) {
                *value = 0;
            } else if (*value > 255) {
                *value = 255;
            }
            *pixel = (uint32_t) (*value << 16) + (*value << 8) + *value;
        }
        pixels = (char *) pixels + info.stride;
    }

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return;
    }

    delete resMatrix;
}


jobject Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_getVertexes(JNIEnv *env,
                                                                                     jobject,
                                                                                     jobject srcBitmap) {
    // initialize
    AndroidBitmapInfo info;
    void *pixels;

    // lockPixels
    if (AndroidBitmap_lockPixels(env, srcBitmap, &pixels) < 0) {
        LOGE("Unable to lockPixels");
        return NULL;
    }

    AndroidBitmap_getInfo(env, srcBitmap, &info);
    LOGD("width: %d, height: %d, format: %d, stride: %d", info.width, info.height,
         info.format, info.stride);
    LOGD("AndroidBitmap_lockPixels");

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888");
        return NULL;
    }

    jobject points = getVertexes(env, &info, pixels);

    // unlockPixels
    if (AndroidBitmap_unlockPixels(env, srcBitmap) < 0) {
        LOGE("Unable to unlockPixels");
        return NULL;
    }

    return points;
}

jobject
Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_connectedComponents(JNIEnv *env,
                                                                                     jobject,
                                                                                     jobjectArray points) {
    jobject llPoints = connectedComponents(env, points);
    return llPoints;
}

jobject Java_com_dutn_handwritingdemo_utils_ImageGraphProcessing_getRect(JNIEnv *env,
                                                                                 jobject,
                                                                                 jobject points,
                                                                                 jint n) {
    jclass listCls = env->FindClass("java/util/ArrayList");
    jmethodID listGet = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    jclass pointCls = env->FindClass("com/mht/dutn/android_card_scan/utils/Point");
    jmethodID pointGetX = env->GetMethodID(pointCls, "getX", "()I");
    jmethodID pointGetY = env->GetMethodID(pointCls, "getY", "()I");
    jclass rectCls = env->FindClass("com/mht/dutn/android_card_scan/utils/Rect");
    jmethodID rectConstruct = env->GetMethodID(rectCls, "<init>", "()V");
    jobject rect = env->NewObject(rectCls, rectConstruct);
    jmethodID setLeft = env->GetMethodID(rectCls, "setLeft", "(I)V");
    jmethodID setTop = env->GetMethodID(rectCls, "setTop", "(I)V");
    jmethodID setRight = env->GetMethodID(rectCls, "setRight", "(I)V");
    jmethodID setBottom = env->GetMethodID(rectCls, "setBottom", "(I)V");

    jobject p0 = env->CallObjectMethod(points, listGet, 0);
    jint minX, maxX, minY, maxY;
    minX = env->CallIntMethod(p0, pointGetX);
    maxX = env->CallIntMethod(p0, pointGetX);
    minY = env->CallIntMethod(p0, pointGetY);
    maxY = env->CallIntMethod(p0, pointGetY);
    env->DeleteLocalRef(p0);
    int i;
    for (i = 1; i < n; i++) {
        jobject p = env->CallObjectMethod(points, listGet, i);
        jint px = env->CallIntMethod(p, pointGetX);
        jint py = env->CallIntMethod(p, pointGetY);
        if (px < minX) {
            minX = px;
        }
        if (px > maxX) {
            maxX = px;
        }
        if (py < minY) {
            minY = py;
        }
        if (py > maxY) {
            maxY = py;
        }
        env->DeleteLocalRef(p);
    }
    env->CallVoidMethod(rect, setLeft, minX);
    env->CallVoidMethod(rect, setTop, minY);
    env->CallVoidMethod(rect, setRight, maxX);
    env->CallVoidMethod(rect, setBottom, maxY);
    env->DeleteLocalRef(rectCls);
    env->DeleteLocalRef(listCls);
    env->DeleteLocalRef(pointCls);
    return rect;
}

#ifdef __cplusplus
}
#endif

static void toGrayScale(AndroidBitmapInfo *info, void *pixels) {
    int yy;
    for (yy = 0; yy < info->height; yy++) {
        uint32_t *line = (uint32_t *) pixels;
        int xx;
        for (xx = 0; xx < info->width; xx++) {
            uint32_t *pixel = &line[xx];
            uint32_t r = *pixel >> 16 & 0xff;
            uint32_t g = *pixel >> 8 & 0xff;
            uint32_t b = *pixel & 0xff;
            uint32_t max = r < g ? (g < b ? b : g) : (r < b ? b : r);
            *pixel = (max << 16) + (max << 8) + max;
//            uint32_t min = r > g ? (g > b ? b : g) : (r > b ? b : r);
//            *pixel = (min << 16) + (min << 8) + min;
        }
        pixels = (char *) pixels + info->stride;
    }
}

static void toBinary(AndroidBitmapInfo *info, void *pixels) {
    int yy;
    for (yy = 0; yy < info->height; yy++) {
        uint32_t *line = (uint32_t *) pixels;
        int xx;
        for (xx = 0; xx < info->width; xx++) {
            uint32_t *pixel = &line[xx];
            uint32_t r = *pixel >> 16 & 0xff;
            uint32_t g = *pixel >> 8 & 0xff;
            uint32_t b = *pixel & 0xff;
            if (r < 50 && g < 50 && b < 50) {
                *pixel = 0;
            } else {
                *pixel = (0xffffff);
            }
        }
        pixels = (char *) pixels + info->stride;
    }
}

static void setContrast(AndroidBitmapInfo *info, void *pixels, jint value) {
    double contrast = pow((value + 100) / 100, 2);
    int yy;
    for (yy = 0; yy < info->height; yy++) {
        uint32_t *line = (uint32_t *) pixels;
        int xx;
        for (xx = 0; xx < info->width; xx++) {
            uint32_t *pixel = &line[xx];
            jint a = 255;
            jint r = (*pixel >> 16) & 0xff;
            jint g = (*pixel >> 8) & 0xff;
            jint b = (*pixel) & 0xff;
            r = (uint32_t) (((((r / 255.0) - 0.5) * contrast) + 0.5) * 255.0);
            if (r < 0) r = 0;
            else if (r > 255) r = 255;
            g = (uint32_t) (((((g / 255.0) - 0.5) * contrast) + 0.5) * 255.0);
            if (g < 0) g = 0;
            else if (g > 255) g = 255;
            b = (uint32_t) (((((b / 255.0) - 0.5) * contrast) + 0.5) * 255.0);
            if (b < 0) b = 0;
            else if (b > 255) b = 255;
            *pixel = (uint32_t) (r << 16) + (g << 8) + b + (a << 24);
        }
        pixels = (char *) pixels + info->stride;
    }
}

static int **applyKernel(JNIEnv *env, AndroidBitmapInfo *info, void *pixels, jobjectArray kernel) {
    int n = info->height;
    int m = info->width;
    int kn = env->GetArrayLength(kernel);
//    jintArray arr0 = (jintArray) env->GetObjectArrayElement(kernel, 0);
//    int km = env->GetArrayLength(arr0);
    int km = 3;
//    env->DeleteLocalRef(arr0);
    int kk = kn / 2;
    int kns = n + (kk + kk);
    int kms = m + (kk + kk);
    // kernel
    int **kernelMatrix = new int *[kn];
    for (int i = 0; i < kn; ++i) {
        kernelMatrix[i] = new int[km];
    }

    kernelMatrix[0][0] = -1;
    kernelMatrix[0][1] = -1;
    kernelMatrix[0][2] = -1;
    kernelMatrix[1][0] = -1;
    kernelMatrix[1][1] = 8;
    kernelMatrix[1][2] = -1;
    kernelMatrix[2][0] = -1;
    kernelMatrix[2][1] = -1;
    kernelMatrix[2][2] = -1;

    // init convMatrix
    int **convMatrix = new int *[kns];
    for (int i = 0; i < kns; i++) {
        convMatrix[i] = new int[kms];
    }
    for (int i = 0; i < kns; ++i) {
        for (int j = 0; j < kms; ++j) {
            convMatrix[i][j] = 0;
        }
    }

    for (int i = kk; i < info->height + kk; ++i) {
        uint32_t *line = (uint32_t *) pixels;
        for (int j = kk; j < info->width + kk; ++j) {
            uint32_t *pixel = &line[j-kk];
            uint32_t gray = *pixel & 0xff;
            convMatrix[i][j] = gray;
        }
        pixels = (char *) pixels + info->stride;
    }

    int **resMatrix = convFull(convMatrix, kernelMatrix, n, m, kn, km);

    for (int i = 0; i < kn; ++i) {
        delete kernelMatrix[i];
    }
    for (int i = 0; i < kns; ++i) {
        delete convMatrix[i];
    }

    return resMatrix;
}

static int **convFull(int **convMatrix, int **kernelMatrix, int n, int m, int kn, int km) {
    int **resMatrix = new int *[n];
    for (int i = 0; i < n; i++) {
        resMatrix[i] = new int[m];
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            int sum = 0;
            for (int ki = i; ki < i + kn; ki++) {
                for (int kj = j; kj < j + km; kj++) {
                    sum += convMatrix[ki][kj] * kernelMatrix[ki - i][kj - j];
                }
            }
            resMatrix[i][j] = sum;
        }
    }
    return resMatrix;
}

static jobject getVertexes(JNIEnv *env, AndroidBitmapInfo *info, void *pixels) {
    jclass listCls = env->FindClass("java/util/ArrayList");
    jmethodID listConstruct = env->GetMethodID(listCls, "<init>", "()V");
    jobject list = env->NewObject(listCls, listConstruct);

    int yy;
    for (yy = 0; yy < info->height; yy++) {
        uint32_t *line = (uint32_t *) pixels;
        int xx;
        for (xx = 0; xx < info->width; xx++) {
            uint32_t *pixel = &line[xx];
            uint32_t r = *pixel >> 16 & 0xff;
            uint32_t g = *pixel >> 8 & 0xff;
            uint32_t b = *pixel & 0xff;
            if (r == 0 && g == 0 && b == 0) {
                jclass pointCls = env->FindClass("com/mht/dutn/android_card_scan/utils/Point");
                jmethodID pointConstruct = env->GetMethodID(pointCls, "<init>", "()V");
                jobject point = env->NewObject(pointCls, pointConstruct);
                jmethodID pointSetX = env->GetMethodID(pointCls, "setX", "(I)V");
                jmethodID pointSetY = env->GetMethodID(pointCls, "setY", "(I)V");
                env->CallVoidMethod(point, pointSetX, xx);
                env->CallVoidMethod(point, pointSetY, yy);
                jmethodID addMethod = env->GetMethodID(listCls, "add", "(Ljava/lang/Object;)Z");
                env->CallBooleanMethod(list, addMethod, point);

                env->DeleteLocalRef(pointCls);
                env->DeleteLocalRef(point);
            }
        }
        pixels = (char *) pixels + info->stride;
    }

    return list;
}

static jobject connectedComponents(JNIEnv *env, jobject points) {
    jclass listCls = env->FindClass("java/util/ArrayList");
    jclass pointCls = env->FindClass("com/mht/dutn/android_card_scan/utils/Point");
    jmethodID listConstruct = env->GetMethodID(listCls, "<init>", "()V");
    jmethodID listGet = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    jobject list = env->NewObject(listCls, listConstruct);
    jint n = env->CallIntMethod(points, env->GetMethodID(listCls, "size", "()I"));

    bool *freeVector = new bool[n];
    int i;
    for (i = 0; i < n; i++) {
        freeVector[i] = true;
    }

    for (int u = 0; u < n; u++) {
        if (freeVector[u]) {
            freeVector[u] = false;
            jobject res = env->NewObject(listCls, listConstruct);
            jmethodID addMethod = env->GetMethodID(listCls, "add", "(Ljava/lang/Object;)Z");
            jobject point = env->CallObjectMethod(points, listGet, u);
            env->CallBooleanMethod(res, addMethod, point);
            env->DeleteLocalRef(point);
            dfs(env, u, points, freeVector, n, res);
            env->CallBooleanMethod(list, addMethod, res);
            env->DeleteLocalRef(res);
        }
    }
    delete freeVector;
    env->DeleteLocalRef(listCls);
    env->DeleteLocalRef(pointCls);

    return list;
}

static void dfs(JNIEnv *env, int u, jobject points, bool *freeVector, int n, jobject res) {
    jclass listCls = env->FindClass("java/util/ArrayList");
    jclass pointCls = env->FindClass("com/mht/dutn/android_card_scan/utils/Point");
    jmethodID listGet = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    jmethodID listAdd = env->GetMethodID(listCls, "add", "(Ljava/lang/Object;)Z");

    int v;
    for (v = 0; v < n; v++) {
        if (u != v && freeVector[v]) {
            jobject p1 = env->CallObjectMethod(points, listGet, u);
            jobject p2 = env->CallObjectMethod(points, listGet, v);
            jint p1x = env->CallIntMethod(p1, env->GetMethodID(pointCls, "getX", "()I"));
            jint p1y = env->CallIntMethod(p1, env->GetMethodID(pointCls, "getY", "()I"));
            jint p2x = env->CallIntMethod(p2, env->GetMethodID(pointCls, "getX", "()I"));
            jint p2y = env->CallIntMethod(p2, env->GetMethodID(pointCls, "getY", "()I"));
            if ((p1x - 1 == p2x) && (p1y == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x - 1 == p2x) && (p1y - 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x == p2x) && (p1y - 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x + 1 == p2x) && (p1y - 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x + 1 == p2x) && (p1y == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x + 1 == p2x) && (p1y + 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x == p2x) && (p1y + 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            } else if ((p1x - 1 == p2x) && (p1y + 1 == p2y)) {
                freeVector[v] = false;
                jobject point = env->CallObjectMethod(points, listGet, v);
                env->CallBooleanMethod(res, listAdd, point);
                env->DeleteLocalRef(point);
                env->DeleteLocalRef(p1);
                env->DeleteLocalRef(p2);
                env->DeleteLocalRef(listCls);
                env->DeleteLocalRef(pointCls);
                dfs(env, v, points, freeVector, n, res);
                return;
            }
            if (p1 != NULL) {
                env->DeleteLocalRef(p1);
            }
            if (p2 != NULL) {
                env->DeleteLocalRef(p2);
            }
        }
    }
    if (listCls != NULL) {
        env->DeleteLocalRef(listCls);
    }
    if (pointCls != NULL) {
        env->DeleteLocalRef(pointCls);
    }
}
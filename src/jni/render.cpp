// local includes
#include "gl_help.h"

// system includes
#include <jni.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

/*
 * Using GLES2
 */
#include <GLES2/gl2.h>
/*
 * Using EGL
 */
#include <EGL/egl.h>
/*
 * Using logs
 */
#include <android/log.h>
/*
 * And use native_window
 */
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define  LOG(...)  __android_log_print(ANDROID_LOG_ERROR,"PTHREADRENDER",__VA_ARGS__)

extern "C"
{
  ANativeWindow * _nativeWindow;
  EGLDisplay _display;
  EGLSurface _surface;
  EGLContext _context;
  EGLConfig _config;


  //{@ DONT TOUCH
  bool g_isRunning = false;
  JavaVM * g_JavaVM;
  jobject g_callbackObj;

  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM* vm, void* reserved)
  {
    g_JavaVM = vm;
    return JNI_VERSION_1_6;
  }

  void * run(void* param)
  {
    JNIEnv * env;
    g_JavaVM->GetEnv((void**) &env, JNI_VERSION_1_6);
    g_JavaVM->AttachCurrentThread(&env, NULL);

    jclass clazz = env->GetObjectClass(g_callbackObj);
    jmethodID mid = env->GetMethodID(clazz, "callback", "()V");
    env->CallVoidMethod(g_callbackObj, mid);

    while (g_isRunning)
      {
        // do nothing
      }

    return NULL;
  } // dont touch

  int config_comp(const void * l, const void * r) {
    return gl_help_compare_config(l, r, _display);
  }

  void createEgl(JNIEnv * env, jobject jsurface)
  {
    LOG("CREATING CONTEXT");

    // init egl here
    _nativeWindow = ANativeWindow_fromSurface(env, jsurface);
    assert(_nativeWindow);
    _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(_display);

    //init egl
    EGLint version[2] = {0};
    assert(eglInitialize(_display,&version[0], &version[1]));

    EGLint attr_list[] = {
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_BLUE_SIZE, 5,
        EGL_STENCIL_SIZE, 0,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig configs[40];
    int num_config;
    assert(eglChooseConfig(_display, attr_list, configs, 40, &num_config));

    qsort(configs, num_config, sizeof(EGLConfig), config_comp);

    for (int i = 0; i < num_config; ++i)
    {
        EGLConfig currentConfig = configs[i];
        _surface = eglCreateWindowSurface(_display, currentConfig, _nativeWindow, 0);

        if (_surface == EGL_NO_SURFACE)
          continue;

        EGLint contextAttrList[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        _context = eglCreateContext(_display, currentConfig, EGL_NO_CONTEXT, contextAttrList);
        if (_context == EGL_NO_CONTEXT)
        {
            eglDestroySurface(_display, _surface);
            continue;
        }
        _config = currentConfig;
        break;
    }

    // final check
    assert(_surface);
    assert(_context);
    LOG("CONTEXT CREATED");

    // binding context
    assert(eglMakeCurrent(_display, _surface, _surface, _context));
    LOG("CONTEXT BINDED");
  }

  void destroyEgl(JNIEnv * env)
  {
    // destroy egl
    ANativeWindow_release(_nativeWindow);
    _nativeWindow = NULL;
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_nativeOnCreateSurface(JNIEnv * env, jobject thiz, jobject jsurface)
  {
    createEgl(env, jsurface);
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_nativeOnDestroySurface(JNIEnv * env, jobject thiz)
  {
    destroyEgl(env);
  }


  //{@ DONT TOUCH
  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_startThreads(JNIEnv * env, jobject thiz)
  {
    if (!g_isRunning)
      {
        g_isRunning = true;
        g_callbackObj = env->NewGlobalRef(thiz);

        // run threads
        pthread_t pid1;
        pthread_create(&pid1, NULL, run, NULL);

        pthread_t pid2;
        pthread_create(&pid2, NULL, run, NULL);
      }
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_stopThreads(JNIEnv * env, jobject thiz)
  {
    g_isRunning = false;
    env->DeleteGlobalRef(g_callbackObj);
  } // dont touch

} // extern "C"

#include <jni.h>
#include <pthread.h>

extern "C"
{

  bool g_isRunning = false;
  JavaVM * g_JavaVM;
  jobject g_callbackObj;

  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM* vm, void* reserved)
  {
    g_JavaVM = vm;
    return JNI_VERSION_1_6;
  }

  void *
  run(void* param)
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
  }

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
  }

} // extern "C"

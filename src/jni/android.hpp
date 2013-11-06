#pragma once

#include <android/log.h>

#define  LOG(...)  __android_log_print(ANDROID_LOG_ERROR,"PTHREADRENDER",__VA_ARGS__)

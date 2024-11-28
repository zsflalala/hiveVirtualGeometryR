#pragma once

#include <android/log.h>

#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, __VA_ARGS__)


namespace hiveVG::TAG_KEYWORD
{
    const char *const MAIN_TAG = "Main";
    const char *const RENDERER_TAG = "CRenderer";
    const char *const SeqFrame_RENDERER_TAG = "CSequenceFrameRenderer";
}

licenses(["by_exception_only"])

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "Audio360",
  srcs = select({
    "@tools//target_cpu:arm64-v8a": [
      "Android/arm64-v8a/libAudio360.so",
    ],
    "@tools//target_cpu:armeabi-v7a": [
      "Android/armeabi-v7a/libAudio360.so",
    ],
    "//conditions:default": [],
  }),
  hdrs = glob([
    "include/*.h",
    "include/*.hh",
  ]),
)

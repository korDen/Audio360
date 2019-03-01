/*
 * Copyright (c) 2018-present, Facebook, Inc.
 */

/**
 * @file TBE_AudioEngineDefinitions.h
 * AudioEngine specific definitions
 */

#pragma once

#define TBE_AUDIOENGINE_VERSION_MAJOR 1
#define TBE_AUDIOENGINE_VERSION_MINOR 5
#define TBE_AUDIOENGINE_VERSION_PATCH 1

#ifndef API_EXPORT
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#if defined(STATIC_LINKED)
#define API_EXPORT
#else
#define API_EXPORT __declspec(dllexport)
#endif
#else
#if defined(__GNUC__)
#define API_EXPORT __attribute__((visibility("default")))
#else
#define API_EXPORT
#endif
#endif
#endif

#if defined(TBE_EMSCRIPTEN)
#undef API_EXPORT
#include <emscripten.h>
#define API_EXPORT EMSCRIPTEN_KEEPALIVE
#endif

#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

struct AAssetManager;

namespace TBE {
//--------- ENUMS --------------//

/// For specifying if the asset must be found from the app bundle
/// or if the asset name includes the absolute path
enum class AssetLocation {
  APP_BUNDLE, /// ONLY mobile: The asset must be found from the app bundle (within the app resources
              /// on iOS,
  /// 'assets' folder on Android)
  ABSOLUTE_PATH, /// Cross platform: the asset name includes the absolute path
};

enum class AssetLoadType {
  MEMORY = 0,
  STREAM = 1,
  BUFFER = 2,
};

enum class SourcePanner {
  STEREO,
  APPROX,
  HRTF,
};

enum class SpeakerPosition {
  LEFT = 0,
  RIGHT,
  CENTER,
  LEFT_SURROUND,
  RIGHT_SURROUND,
  LEFT_BACK_SURROUND,
  RIGHT_BACK_SURROUND,
  LFE,
  END_ENUM
};

enum class AttenuationType {
  LOG = 0, /// Logarithmic distance roll-off model (default)
  LINEAR, /// Linear distance roll-off model
  DISABLE, /// Custom rolloff model
};

enum class EngineError {
  QUEUE_FULL = -21,
  BAD_THREAD = -20,
  NOT_SUPPORTED = -19,
  NO_AUDIO_DEVICE = -18,
  COULD_NOT_CONNECT = -17,
  MEMORY_MAP_FAIL = -16,
  INVALID_URL_FORMAT = -15,
  ERROR_OPENING_TEMP_FILE = -14,
  INVALID_HEADER = -13,
  CURL_FAIL = -12,
  INVALID_CHANNEL_COUNT = -11,
  CANNOT_INIT_DECODER = -10,
  ERROR_OPENING_FILE = -9,
  NO_ASSET = -8,
  CANNOT_ALLOCATE_MEMORY = -7,
  CANNOT_CREATE_AUDIO_DEVICE = -6,
  CANNOT_INITIALISE_CORE = -5,
  INVALID_BUFFER_SIZE = -4,
  INVALID_SAMPLE_RATE = -3,
  NO_OBJECTS_IN_POOL = -2,
  FAIL = -1,
  OK = 0,
};

enum class PlayState { PLAYING, PAUSED, STOPPED, INVALID };

enum class SyncMode { INTERNAL, EXTERNAL };

enum class Event {
  ERROR_BUFFER_UNDERRUN, /// Dispatched by AudioEngine when the mixer is unable to process audio in
                         /// time.
  ERROR_QUEUE_STARVATION, /// Dispatched by SpatDecoderQueue and SpatDecoderFile when enqueued audio
                          /// is not being decoded in time.
  DECODER_INIT, /// Dispatched by SpatDecoderFile when an opened file is ready for playback.
  END_OF_STREAM, /// Dispatched by SpatDecoderFile when an opened file is has completed playing.
  LOOPED, /// Dispatched when an object has looped
  INVALID
};

enum class ChannelMap {
  TBE_8_2, /// 8 channels of hybrid TBE ambisonics and 2 channels of head-locked stereo audio
  TBE_8, /// 8 channels of hybrid TBE ambisonics. NO head-locked stereo audio
  TBE_6_2, /// 6 channels of hybrid TBE ambisonics and 2 channels of head-locked stereo audio
  TBE_6, /// 6 channels of hybrid TBE ambisonics. NO head-locked stereo audio
  TBE_4_2, /// 4 channels of hybrid TBE ambisonics and 2 channels of head-locked stereo audio
  TBE_4, /// 4 channels of hybrid TBE ambisonics. NO head-locked stereo audio
  TBE_8_PAIR0, /// Channels 1 and 2 of TBE hybrid ambisonics
  TBE_8_PAIR1, /// Channels 3 and 4 of TBE hybrid ambisonics
  TBE_8_PAIR2, /// Channels 5 and 6 of TBE hybrid ambisonics
  TBE_8_PAIR3, /// Channels 7 and 8 of TBE hybrid ambisonics
  TBE_CHANNEL0, /// Channels 1 of TBE hybrid ambisonics
  TBE_CHANNEL1, /// Channels 2 of TBE hybrid ambisonics
  TBE_CHANNEL2, /// Channels 3 of TBE hybrid ambisonics
  TBE_CHANNEL3, /// Channels 4 of TBE hybrid ambisonics
  TBE_CHANNEL4, /// Channels 5 of TBE hybrid ambisonics
  TBE_CHANNEL5, /// Channels 6 of TBE hybrid ambisonics
  TBE_CHANNEL6, /// Channels 7 of TBE hybrid ambisonics
  TBE_CHANNEL7, /// Channels 8 of TBE hybrid ambisonics
  HEADLOCKED_STEREO, /// Head-locked stereo audio
  HEADLOCKED_CHANNEL0, /// Channels 1 or left of head-locked stereo audio
  HEADLOCKED_CHANNEL1, /// Channels 2 or right of head-locked stereo audio
  AMBIX_4, /// 4 channels of first order ambiX
  AMBIX_9, /// 9 channels of second order ambiX
  AMBIX_9_2, /// 9 channels of second order ambiX with 2 channels of head-locked audio
  STEREO, /// Stereo audio
  INVALID, /// Invalid/unknown map. This must always be last.
};

enum Options { DEFAULT = 0, DECODE_IN_AUDIO_CALLBACK = 1 << 0 };

inline Options operator|(Options a, Options b) {
  return static_cast<Options>(static_cast<int>(a) | static_cast<int>(b));
}

enum class AmbisonicRenderer {
  VIRTUAL_SPEAKER, /// Deprecated renderer
  AMBISONIC
};

//--------- STRUCTURES --------------//

struct AttenuationProps {
  float minimumDistance{1.f}; /// The distance after which attenuation kicks it
  float maximumDistance{1000.f}; /// The distance at which attenuation stops
  float factor{1.f}; /// The attenuation curve factor. 1 = 6dB drop with doubling of distance. > 1
                     /// steeper curve. < 1 less steep curve!
  bool maxDistanceMute{false}; /// If the sound must be muted at and after its maximum distance

  AttenuationProps() {}

  AttenuationProps(
      float minDistance,
      float maxDistance,
      float factor,
      bool maxDistanceMute = false) {
    this->minimumDistance = minDistance;
    this->maximumDistance = maxDistance;
    this->factor = factor;
    this->maxDistanceMute = maxDistanceMute;
  }
};

enum class AttenuationMode {
  LOGARITHMIC, /// Logarithmic distance attenuation model
  LINEAR, /// Linear distance attenuation model
  DISABLE, /// Disable distance attenutation
};

struct AssetDescriptor {
  size_t offsetInBytes{0}; /// Read offset, in bytes. Set to 0 if unknown.
  size_t lengthInBytes{0}; /// Length of required data from offset, in bytes. Set to 0 if unknown.

  AssetDescriptor() {}

  AssetDescriptor(size_t offset, size_t length) : offsetInBytes(offset), lengthInBytes(length) {}
};

enum class AudioDeviceType {
  DEFAULT, /// Use the system's default audio device
  CUSTOM, /// Specify a custom audio device
  DISABLED /// Disable the audio device
};

// Warning, the default values below have been carefully chosen and must not be changed!

/// Specify the audio settings for the engine.
/// \b NOTE: Depending on the platform, the sample rate and buffer size values might only be a
/// recommendation.
struct AudioSettings {
  float sampleRate{44100.f}; /// Sample rate of the engine in Hz. If 0, the engine will determine
                             /// the best rate or default.
  int32_t bufferSize{1024}; /// Buffer size of the engine in samples. If 0, the engine will
                            /// determine the best size.
  AudioDeviceType deviceType{AudioDeviceType::DEFAULT}; /// Set the audio device type
  const char* customAudioDeviceName{""}; /// The name of the custom audio device. Valid only if
                                         /// DeviceType is set to CUSTOM
};

struct NetworkSettings {
  uint32_t streamingBufferSizeBytes; /// Buffer size, in bytes, of the streaming buffer
  int64_t maxDownloadSpeedBytes; /// Maximum download speed in bytes per second. If set to 0, the
                                 /// speed will be unlimited
  bool printDebugInfo; /// Print debug info to stdout
};

struct MemorySettings {
  int32_t spatDecoderQueuePoolSize{1}; /// Number of spat decoder queued objects in the pool
  int32_t spatDecoderFilePoolSize{1}; /// Number of spat decoder file objects in the pool
  int32_t spatQueueSizePerChannel{4096}; /// Size of the spat queue for each Format, in samples
  int32_t audioObjectPoolSize{128}; /// Number of positional audio objects. Currently experimental
  size_t speakersVirtualizersPoolSize{8}; /// Number of Speakers Virtualizers
};

struct PlatformSettings {
  void* androidEnv{nullptr}; /// Android JNI environment
  AAssetManager* androidAssetManager{nullptr}; /// Android AssetManager
};

// Use with caution. The fields of this struct are likely to change.
struct Experimental {
  AmbisonicRenderer ambisonicRenderer{AmbisonicRenderer::AMBISONIC};
  bool tryAndroidFastPath{false};
};

struct ThreadSettings {
  bool useEventThread{true}; /// Use a separate thread to dispatch event callbacks. If set to false
                             /// AudioEngine::processEventsOnThisThread can be used to dequeue
                             /// events on a user thread
  bool useDecoderThread{
      true}; /// If true, all audio decoding jobs happen on a separate thread. If false, all
             /// decoding jobs happen in the audio callback or whatever thread calls
             /// AudioEngine::getAudioMix. This is similar to the
             /// Options::DECODE_IN_AUDIO_CALLBACK option when creating an AudioObject or
             /// SpatDecoderFile, except it is applied globally for all objects and jobs.
};

struct EngineInitSettings {
  AudioSettings audioSettings;
  MemorySettings memorySettings;
  PlatformSettings platformSettings;
  ThreadSettings threads;
  Experimental experimental;
};

//--------- FUNC PTRS / CALLBACKS --------------//

/// The event callback is used by the AudioEngine and other objects to receive asynchronous events
/// \param event The event
/// \param owner The calling object: AudioEngine, SpatDecoderQueue, SpatDecoderFile
/// \param userData User data specified in the setCallback method, can be nullptr
typedef void (*EventCallback)(Event event, void* owner, void* userData);

class AudioObject;
class EventListener {
 public:
  virtual ~EventListener() {}
  virtual void onNewEvent(Event event) = 0;
  virtual void onNewEvent(Event event, AudioObject* owner) = 0;
};

//--------- HELPERS --------------//

inline int32_t getNumChannelsForMap(ChannelMap map) {
  switch (map) {
    case ChannelMap::TBE_8_2:
      return 10;
    case ChannelMap::TBE_6_2:
    case ChannelMap::TBE_8:
      return 8;
    case ChannelMap::TBE_6:
    case ChannelMap::TBE_4_2:
      return 6;
    case ChannelMap::TBE_4:
    case ChannelMap::AMBIX_4:
      return 4;
    case ChannelMap::TBE_8_PAIR0:
    case ChannelMap::TBE_8_PAIR1:
    case ChannelMap::TBE_8_PAIR2:
    case ChannelMap::TBE_8_PAIR3:
    case ChannelMap::HEADLOCKED_STEREO:
      return 2;
    case ChannelMap::TBE_CHANNEL0:
    case ChannelMap::TBE_CHANNEL1:
    case ChannelMap::TBE_CHANNEL2:
    case ChannelMap::TBE_CHANNEL3:
    case ChannelMap::TBE_CHANNEL4:
    case ChannelMap::TBE_CHANNEL5:
    case ChannelMap::TBE_CHANNEL6:
    case ChannelMap::TBE_CHANNEL7:
    case ChannelMap::HEADLOCKED_CHANNEL0:
    case ChannelMap::HEADLOCKED_CHANNEL1:
      return 1;
    case ChannelMap::AMBIX_9:
      return 9;
    case ChannelMap::AMBIX_9_2:
      return 11;
    default:
      return 0;
  }
}

/// See EBU R128 for more information about these statistics.
struct LoudnessStatistics {
  float integrated{(float)-INFINITY};
  float shortTerm{(float)-INFINITY};
  float momentary{(float)-INFINITY};
  float truePeak{(float)-INFINITY};
};

//--------- DEFAULT VALUES --------------//

static const AudioSettings AudioSettings_default = AudioSettings();
static const NetworkSettings NetworkSettings_default = NetworkSettings();
static const MemorySettings MemorySettings_default = MemorySettings();
static const EngineInitSettings EngineInitSettings_default = EngineInitSettings();
static const size_t kMaxStrSize = 512;
} // namespace TBE

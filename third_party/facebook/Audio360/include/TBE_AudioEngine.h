/*
 * Copyright (c) 2018-present, Facebook, Inc.
 */

#pragma once

#include "TBE_AudioEngineDefinitions.h"
#include "TBE_IOStream.h"
#include "TBE_Quat.hh"
#include "TBE_Vector.hh"

namespace TBE {
class AudioEngine;
class SpatDecoderQueue;
class SpatDecoderFile;
class AudioObject;
class SpeakersVirtualizer;

/// Controls the global state of the engine: audio device setup, object pools for spatialisation
/// objects and listener properties. The AudioEngine must be initialised first and destroyed last.
class AudioEngine {
 public:
  /// Callback type for getting the audio mix right before it is written to the
  /// device buffer. Clients can register a callback with \ref setAudioMixCallback()
  /// and get the full audio render to do any post-processing steps they would like.
  ///
  /// \param interleavedAudio - pointer to an interleaved buffer of audio data containing the
  ///                           final mix, just before it is written to the device buffer
  /// \param numChannels - the number of audio channels in the buffer
  /// \param numSamplesPerChannel - the number of samples for each channel
  /// \param userData - user data pointer
  typedef void (*AudioMixCallback)(
      float* interleavedAudio,
      size_t numChannels,
      size_t numSamplesPerChannel,
      void* userData);

  virtual ~AudioEngine() {}

  /// Start the audio device and all audio processing.
  /// start() and suspend() can be used to pause all processing when the app is in the background.
  /// This has no effect if the audio device is disabled.
  /// \return Relevant error or EngineError::OK
  virtual EngineError start() = 0;

  /// Suspend all audio processing.
  /// This has no effect if the audio device is disabled.
  /// \return Relevant error or EngineError::OK
  virtual EngineError suspend() = 0;

  /// Set the orientation of the listener through direction vectors (using Unity's left-handed
  /// coordinate convention) \param forwardVector Forward vector of the listener \param upVector Up
  /// vector of the listener \see TBVector for the co-ordinate system used by the audio engine. \see
  /// setListenerRotation(TBQuat quat)
  virtual void setListenerRotation(TBVector forwardVector, TBVector upVector) = 0;

  /// Set the orientation of the listener through a quaternion (using Unity's left-handed coordinate
  /// convention) \param quat Listener quaternion \see TBVector for the co-ordinate system used by
  /// the audio engine. \see setListenerRotation(TBVector forward, TBVector upVector)
  virtual void setListenerRotation(TBQuat quat) = 0;

  /// !!! It is strongly recommended that you use one of the other functions to set listener
  /// rotation - set rotation using forward/up vectors or a quaternion in order to avoid gimbal lock
  /// problems. Set the listener rotation in degrees. \param yaw Range from -180 to 180 (negative is
  /// left) \param pitch Range from -180 to 180 (positive is up) \param roll Range from -180 to 180
  /// (negative is left)
  virtual void setListenerRotation(float yaw, float pitch, float roll) = 0;

  /// Set the position of the listener in space.
  /// \param position of the listener.
  /// \see TBVector for the co-ordinate system used by the audio engine.
  virtual void setListenerPosition(TBVector position) = 0;

  /// \return the position of the listener
  /// \see TBVector for the co-ordinate system used by the audio engine.
  virtual TBVector getListenerPosition() const = 0;

  /// \return the rotation of the listener as a quaternion.
  virtual TBQuat getListenerRotation() const = 0;

  /// \return The listener's forward vector
  virtual TBVector getListenerForward() const = 0;

  /// \return The listener's up vector
  virtual TBVector getListenerUp() const = 0;

  /// Enable positional tracking by specifying the initial/default position of the listener.
  /// The current position of the listener is automatically queried from AudioEngine
  /// (AudioEngine::setListenerPosition(..)).
  /// The API works by subtracting the current listener position from the initial listener position
  /// to arrive at the delta. The positional tracking range is limited to 1 unit magnitude in each
  /// direction. \param enable Set to true to enable positional tracking \param
  /// initialListenerPosition Initial/default position of the listener \return EngineError::OK or
  /// EngineError::NOT_SUPPORTED if an invalid renderer is spacified on initialisation
  virtual EngineError enablePositionalTracking(bool enable, TBVector initialListenerPosition) = 0;

  /// \return true if positional tracking is enabled
  virtual bool positionalTrackingEnabled() const = 0;

  /// \return Buffer size of the engine in samples. In most cases this would be the value
  /// specified on initialisation. Some platforms might return a value different
  /// from what is specified on initialisation.
  virtual int getBufferSize() const = 0;

  /// \return Sample rate of the engine in Hz. In most cases this would be the value
  /// specified on initialisation. Some platforms might return a value different
  /// from what is specified on initialisation.
  virtual float getSampleRate() const = 0;

  /// If the audio is constructed with the audio device disabled, this method can be used to return
  /// mixed spatialised buffers within your app's audio callback. \b WARNING: calling this method
  /// when an audio device is active can result in undefined behaviour. \param buffer Pointer to
  /// existing interleaved float buffer \param numOfSamples In most cases this would the buffer size
  /// * 2 (since it is stereo) \param numOfChannels Number of channels in buffer. This should be
  /// currently set to 2. \return Relevant error or EngineError::OK
  virtual EngineError getAudioMix(float* buffer, int numOfSamples, int numOfChannels) = 0;

  /// Call this function to register a callback for getting the final audio mix data just before
  /// it is written to the device buffer. It is the caller's responsibility to ensure:
  ///   1) The callback pointer is valid for the lifetime of the engine
  ///   2) Any operations done in the callback do not block the audio thread. This includes
  ///      memory allocation and release, file system and network access and anything that
  ///      may take time and cause the audio thread to cause a buffer underrun.
  ///
  /// \param AudioMixCallback - the callback that will be executed on each audio frame
  /// \param userData - pointer to user specific data
  /// \return Relevant error or EngineError::OK
  virtual EngineError setAudioMixCallback(AudioMixCallback callback, void* userData) = 0;

  /// Create a new SpatDecoderQueue object from an existing pool.
  /// \param spatDecoder Pointer to SpatDecoderQueue that must be initialised.
  /// \return Relevant error or EngineError::OK
  virtual EngineError createSpatDecoderQueue(SpatDecoderQueue*& spatDecoder) = 0;

  /// Destroy an existing and valid instance to SpatDecoderQueue and return it to the pool.
  virtual void destroySpatDecoderQueue(SpatDecoderQueue*& spatDecoder) = 0;

  /// Creates a virtual speaker layout that can playback arbitrary speaker layout formats
  /// such as stereo, 5.0, 5.1, 7.0 and 7.1.
  ///
  /// The Speakers Virtualizer uses AudioObjects under the hood to create the virtual speakers.
  /// For this reason, this call will fail if you do not have enough objects left in the audio
  /// object pool. You can customize the number of audio objects available via \ref
  /// engineSettings.memorySettings.audioObjectPoolSize
  ///
  /// \param virtualizer A pointer to a SpeakersVirtualizer that will be filled in upon success
  /// \param layout a SpeakerPosition::EndEnum terminated array of SpeakerPositions specifying
  ///        the speaker layout to be used and the order with which they are interleaved e.g.
  ///        [SpeakerPosition::LEFT, SpeakerPosition::RIGHT, SpeakerPosition::CENTER,
  ///        SpeakerPosition::END_ENUM] specifies a 3 channel audio interleaved stream, where
  ///        channel 1 is the audio for the left speaker, channel 2 is the right speaker and channel
  ///        3 is center.
  /// \param channelBufferSizeInSamples The size of the audio buffer for each channel.
  ///
  /// \return EngineError::OK on success or EngineError::NO_OBJECTS_IN_POOL
  virtual EngineError createSpeakersVirtualizer(
      SpeakersVirtualizer*& virtualizer,
      SpeakerPosition const* layout,
      size_t channelBufferSizeInSamples = 8192) = 0;

  /// Destroys a SpeakersVirtualizer and returns its AudioObjects to the pool
  /// \param virtualizer The object to be destroyed. This MUST have been previously created with
  /// createSpeakersVirtualizer()
  virtual void destroySpeakersVirtualizer(SpeakersVirtualizer*& virtualizer) = 0;

  /// Create a new SpatDecoderFile object from an existing pool.
  /// \param spatDecoder Pointer to SpatDecoderFile that must be initialised.
  /// \param options Currently only Options::DEFAULT and Options::DECODE_IN_AUDIO_CALLBACK are
  /// supported, where Options::DECODE_IN_AUDIO_CALLBACK will decode all audio within the audio
  /// mixer callback rather than a separate thread. DECODE_IN_AUDIO_CALLBACK is useful for cases
  /// where you might use the audio engine as an in-place processor with no multi-threading or audio
  /// device support. \return Relevant error or EngineError::OK
  virtual EngineError createSpatDecoderFile(
      SpatDecoderFile*& spatDecoder,
      Options options = Options::DEFAULT) = 0;

  /// Destroy an existing and valid instance to SpatDecoderFile and return it to the pool.
  virtual void destroySpatDecoderFile(SpatDecoderFile*& spatDecoder) = 0;

  /// Create an audio object that can position mono audio. This is currently experimental and might
  /// return a null object. \param audioObject Pointer to AudioObject that must be initialised.
  /// \param options Currently only Options::DEFAULT and Options::DECODE_IN_AUDIO_CALLBACK are
  /// supported, where Options::DECODE_IN_AUDIO_CALLBACK will decode all audio within the audio
  /// mixer callback rather than a separate thread. DECODE_IN_AUDIO_CALLBACK is useful for cases
  /// where you might use the audio engine as an in-place processor with no multi-threading or audio
  /// device support. \return Relevant error or EngineError::OK
  virtual EngineError createAudioObject(
      AudioObject*& audioObject,
      Options options = Options::DEFAULT) = 0;

  /// Destroy an existing and valid instance to AudioObject and return it to the pool.
  virtual void destroyAudioObject(AudioObject*& audioObject) = 0;

  /// Set a function to receive events from the audio engine.
  /// \param callback Event callback function
  /// \param userData User data. Can be nullptr
  /// \return Relevant error or EngineError::OK
  virtual EngineError setEventCallback(EventCallback callback, void* userData) = 0;

  /// Play a test sine tone at the specified frequency and gain (linear). This will overwrite
  /// any other audio that might be mixed or processed by the engine.
  virtual void enableTestTone(bool enable, float frequency = 440.f, float gain = 0.5f) = 0;

  /// \return Major version as int
  virtual int getVersionMajor() const = 0;

  /// \return Minor version as int
  virtual int getVersionMinor() const = 0;

  /// \return Patch version as int
  virtual int getVersionPatch() const = 0;

  /// \returns The version hash
  virtual const char* getVersionHash() const = 0;

  /// The rendered loudness statistics since construction or since the last
  /// call to resetIntegratedLoudness(). This is not a measure of the
  /// loudness of the ambisonic field, but rather a measure of the loudness
  /// as binaurally rendered for the listener (including head tracking and
  /// HRTFs).
  /// Threadsafe.
  /// Care must be taken not to call in parallel with enableLoudness().
  virtual LoudnessStatistics getRenderedLoudness() = 0;

  /// Reset loudness statistics state.
  /// Threadsafe.
  /// Care must be taken not to call in parallel with enableLoudness().
  virtual void resetLoudness() = 0;

  /// Default state: disabled. Threadsafe but not reentrant.
  /// Care must be taken not to call in parallel with getRenderedLoudness() or resetLoudness()
  virtual void enableLoudness(bool enabled = true) = 0;

  /// Process all audio engine events on the thread from which this method is called from. Only
  /// supported if this engine is configured to run without an event thread in the constructor.
  /// Thread safe but non re-entrant.
  // \return EngineError::OK or EngineError::NOT_SUPPORTED
  virtual EngineError processEventsOnThisThread() = 0;

  /// Returns the elapsed time in samples since the engine was initialised and start() was called.
  /// The DSP clock count will be suspended when suspend() is called and resumed when start() is
  /// called.
  virtual int64_t getDSPTime() const = 0;

  /// Sets the number of output buffers. Thread safe.
  /// Currently only applicable on Android when an audio device is used and can be used for
  /// increasing the buffer size when bluetooth audio devices are connected. \param numOfBuffers
  /// Number of buffers. Default is 1. Minimum 1, Maximum 12. A value greater than 12 will be
  /// clamped. \return EngineError::OK if successful or EngineError::NOT_SUPPORTED.
  virtual EngineError setNumOutputBuffers(unsigned int numOfBuffers) = 0;

  /// Returns the current number of output buffers. Default: 1. Thread safe.
  virtual unsigned int getNumOutputBuffers() const = 0;

  /// Returns the output latency in samples, if an audio device is in use. On most platforms this is
  /// likely to be a best guess. The latency can be used to compensate for sync in video players.
  virtual int32_t getOutputLatencySamples() const = 0;

  /// Returns the output latency in milliseconds, if an audio device is in use. On most platforms
  /// this is likely to be a best guess. The latency can be used to compensate for sync in video
  /// players.
  virtual double getOutputLatencyMs() const = 0;

  /// Returns the name of the output audio device. This could be the name of the default device as
  /// specified by the OS or a custom device specified on intialisation. On mobile devices this is
  /// likely to return "default".
  virtual const char* getOutputAudioDeviceName() const = 0;

  /// Static method that returns the number of available audio devices.
  API_EXPORT static int32_t getNumAudioDevices();

  /// Static method that returns the name of a device. The name can used on initialisation of the
  /// AudioEngine to route audio to this device. \param index A value between 0 and
  /// getNumAudioDevices() - 1 \return Name of the device.
  API_EXPORT static const char* getAudioDeviceName(int index);

  /// Static method that returns the name of an audio device based on its ID. Currently only
  /// applicable on Windows. \param id A string containing the GUID of the IMMDevice on Windows
  /// (IMMDevice->GetId). \return The name of the device, if found
  API_EXPORT static const char* getAudioDeviceNameFromId(wchar_t* id);
};

class TransportControl {
 public:
  /// Begin dequeueing and playing back audio. Threadsafe but non reentrant, all transport methods
  /// must not be called in parallel. \return Relevant error or EngineError::OK
  virtual EngineError play() = 0;

  /// Schedule a play event after a period of time. Any subsequent call to this function or any play
  /// function will disregard this event if it hasn't already been triggered. Threadsafe but non
  /// reentrant, all transport methods must not be called in parallel. \param millisecondsFromNow
  /// Number of milliseconds from now after which playback will begin. Must be a positive number.
  /// \return Relevant error or EngineError::OK
  virtual EngineError playScheduled(float millisecondsFromNow) = 0;

  /// Begin playback with a fadein. Any subsequent call to this function or any play function will
  /// disregard this event if it hasn't already been triggered. Threadsafe but non reentrant, all
  /// transport methods must not be called in parallel. \param fadeDurationInMs Duration of the
  /// fadein in milliseconds. Must be a positive number. \return Relevant error or EngineError::OK
  virtual EngineError playWithFade(float fadeDurationInMs) = 0;

  /// Pause playback. Any subsequent call to this function or any pause function will disregard this
  /// event if it hasn't already been triggered. Threadsafe but non reentrant, all transport methods
  /// must not be called in parallel. \return Relevant error or EngineError::OK
  virtual EngineError pause() = 0;

  /// Schedule a pause event after a period of time. Any subsequent call to this function or any
  /// pause function will disregard this event if it hasn't already been triggered. Threadsafe but
  /// non reentrant, all transport methods must not be called in parallel. \param
  /// millisecondsFromNow Number of milliseconds from now after which playback pause. Must be a
  /// positive number.
  /// \return Relevant error or EngineError::OK
  virtual EngineError pauseScheduled(float millisecondsFromNow) = 0;

  /// Fadeout the audio and pause. Any subsequent call to this function or any pause function will
  /// disregard this event if it hasn't already been triggered. Threadsafe but non reentrant, all
  /// transport methods must not be called in parallel. \param fadeDurationInMs Duration of the
  /// fadeout in milliseconds. Must be a positive number. \return Relevant error or EngineError::OK
  virtual EngineError pauseWithFade(float fadeDurationInMs) = 0;

  /// Stops playback and resets the playhead to the start of the asset. Threadsafe but non
  /// reentrant, all transport methods must not be called in parallel. \return Relevant error or
  /// EngineError::OK
  virtual EngineError stop() = 0;

  /// Schedule a stop event after a period of time. Any subsequent call to this function or any stop
  /// function will disregard this event if it hasn't already been triggered. Threadsafe but non
  /// reentrant, all transport methods must not be called in parallel. \param millisecondsFromNow
  /// Number of milliseconds from now after which playback will be stopped. Must be a positive
  /// number. \return Relevant error or EngineError::OK
  virtual EngineError stopScheduled(float millisecondsFromNow) = 0;

  /// Fadeout and stop playback. Any subsequent call to this function or any stop function will
  /// disregard this event if it hasn't already been triggered. Threadsafe but non reentrant, all
  /// transport methods must not be called in parallel. \param fadeDurationInMs Duration of the
  /// fadeout in milliseconds. Must be a positive number. \return Relevant error or EngineError::OK
  virtual EngineError stopWithFade(float fadeDurationInMs) = 0;

  /// \return Playback state, either PlayState::PLAYING or PlayState::PAUSED
  virtual PlayState getPlayState() const = 0;

 protected:
  virtual ~TransportControl(){};
};

/// An object with a 3D position and rotation
class Object3D : public TransportControl {
 public:
  /// Set the position of the object
  /// \param position Position of the object in world space
  /// \return  EngineError::OK or EngineError::NOT_SUPPORTED
  virtual EngineError setPosition(TBVector position) = 0;

  /// \return The current position of the object
  virtual TBVector getPosition() const = 0;

  /// Set the rotation of the object
  /// \param rotation Rotation of the object (local space)
  /// \return  EngineError::OK or EngineError::NOT_SUPPORTED
  virtual EngineError setRotation(TBQuat rotation) = 0;

  /// Set the rotation of the object
  /// \param forward Forward direction vector
  /// \param up Up direction vector
  /// \return  EngineError::OK or EngineError::NOT_SUPPORTED
  virtual EngineError setRotation(TBVector forward, TBVector up) = 0;

  /// \return The current rotation of the object
  virtual TBQuat getRotation() const = 0;

 protected:
  virtual ~Object3D(){};
};

/// Interface for spatial audio decoders.
class SpatDecoderInterface : public Object3D {
 public:
  /// Enable mix focus. This gets a specified area of the mix to be more audible than surrounding
  /// areas, by reducing the volume of the area that isn't in focus. The focus area uses a cosine
  /// bump. \param enableFocus Enable focus \param followListener The focus area follows the
  /// listener's gaze
  virtual void enableFocus(bool enableFocus, bool followListener) = 0;

  /// DEPRECATED: Use \ref setOffFocusLeveldB() and \ref setFocusWidthDegrees() instead!
  /// Set the properties of the focus effect. This will be audible only if
  /// enableFocus(..) is set to true.
  /// \param offFocusLevel The level of the area that isn't in focus. A clamped ranged between 0
  /// and 1. 1 is no focus. 0 is maximum focus (the off focus area is reduced by 24dB) \param
  /// focusWidth The focus area specified in degrees
  virtual void setFocusProperties(float offFocusLevel, float focusWidth) = 0; // deprecated

  /// Set the attenuation level outside of the area of focus. This will only take
  /// effect if focus has been enabled by enableFocus(...) and the value is negative.
  /// \param offFocusLevelDB The attenuation level in dB outside of the focus area.
  /// This value has a range between -24.0 and 0.0. Out of bounds values are clamped.
  virtual void setOffFocusLeveldB(float offFocusLevelDB) = 0;

  /// Set the width angle in degrees of the focus area, between 40 and 120 degrees.
  /// This will only take effect if focus has been enabled via enableFocus(...)
  /// \param focusWidthDegrees the focus angle in degrees
  virtual void setFocusWidthDegrees(float focusWidthDegrees) = 0;

  /// Set the orientation of the focus area as a quaternion. This orientation is from the
  /// perspective of the listener and is audible only if followListener in enableFocus(..) is set to
  /// false. \param focusQuat Orientation of the focus area as a quaternion.
  virtual void setFocusOrientationQuat(TBQuat focusQuat) = 0;

  /// Set the volume in linear gain, with an optional ramp time
  /// \param linearGain Linear gain, where 0 is mute and 1 is unity gain
  /// \param rampTimeMs Ramp time to the new gain value in milliseconds. If 0, no ramp will be
  /// applied. \param forcePreviousRamp This forces a previous ramp message (if any) to conclude
  /// right away
  virtual void setVolume(float linearGain, float rampTimeMs, bool forcePreviousRamp = false) = 0;

  /// Set the volume in decibels, with an optional ramp time
  /// \param dB Volume in decibels, where 0 is unity gain.
  /// \param rampTimeMs Ramp time to the new gain value in milliseconds. If 0, no ramp will be
  /// applied. \param forcePreviousRamp This forces a previous ramp message (if any) to conclude
  /// right away
  virtual void setVolumeDecibels(float dB, float rampTimeMs, bool forcePreviousRamp = false) = 0;

  /// \return The current volume
  /// \see setVolume, setVolumeDecibels
  virtual float getVolume() const = 0;

  /// \return The current volume in decibels
  /// \see setVolume, setVolumeDecibels
  virtual float getVolumeDecibels() const = 0;

  /// Begin dequeueing and playing back audio. Threadsafe but non reentrant, all transport methods
  /// must not be called in parallel. \return Relevant error or EngineError::OK
  virtual EngineError play() = 0;

  /// Schedule a play event after a period of time. Any subsequent call to this function or any play
  /// function will disregard this event if it hasn't already been triggered. Threadsafe but non
  /// reentrant, all transport methods must not be called in parallel. \param millisecondsFromNow
  /// Number of milliseconds from now after which playback will begin. Must be a positive number.
  /// \return Relevant error or EngineError::OK
  virtual EngineError playScheduled(float millisecondsFromNow) = 0;

  /// Begin playback with a fadein. Any subsequent call to this function or any play function will
  /// disregard this event if it hasn't already been triggered. Threadsafe but non reentrant, all
  /// transport methods must not be called in parallel. \param fadeDurationInMs Duration of the
  /// fadein in milliseconds. Must be a positive number. \return Relevant error or EngineError::OK
  virtual EngineError playWithFade(float fadeDurationInMs) = 0;

  /// Pause playback. Any subsequent call to this function or any pause function will disregard this
  /// event if it hasn't already been triggered. Threadsafe but non reentrant, all transport methods
  /// must not be called in parallel. \return Relevant error or EngineError::OK
  virtual EngineError pause() = 0;

  /// Schedule a pause event after a period of time. Any subsequent call to this function or any
  /// pause function will disregard this event if it hasn't already been triggered. Threadsafe but
  /// non reentrant, all transport methods must not be called in parallel. \param
  /// millisecondsFromNow Number of milliseconds from now after which playback pause. Must be a
  /// positive number.
  /// \return Relevant error or EngineError::OK
  virtual EngineError pauseScheduled(float millisecondsFromNow) = 0;

  /// Fadeout the audio and pause. Any subsequent call to this function or any pause function will
  /// disregard this event if it hasn't already been triggered. Threadsafe but non reentrant, all
  /// transport methods must not be called in parallel. \param fadeDurationInMs Duration of the
  /// fadeout in milliseconds. Must be a positive number. \return Relevant error or EngineError::OK
  virtual EngineError pauseWithFade(float fadeDurationInMs) = 0;

  /// \return Playback state, either PlayState::PLAYING or PlayState::PAUSED
  virtual PlayState getPlayState() const = 0;

  /// Set a function to receive events from this object. Currently Event::DECODER_INIT is sent when
  /// the a file is loaded and ready for playback. \param callback Event callback function \param
  /// userData User data. Can be nullptr \return Relevant error or EngineError::OK
  virtual EngineError setEventCallback(EventCallback callback, void* userData) = 0;

 protected:
  virtual ~SpatDecoderInterface() {}
};

/// An object for enqueuing and processing spatial audio buffers.
/// Data that is enqueued will be dequeued and processed by the audio engine in the audio device
/// callback. If the audio device is disabled, the data will be processed when
/// AudioEngine::getAudioMix is called. The audio queue is implemented as a lock-free circular
/// buffer (thread safe for one producer and one consumer).
class SpatDecoderQueue : public SpatDecoderInterface {
 public:
  /// Get the free space in the queue (in number of samples) for the kind of data you are enqueueing
  /// \param channelMap Channel map for the data
  /// \return The free space of the queue in samples
  virtual int32_t getFreeSpaceInQueue(ChannelMap channelMap) const = 0;

  /// The size of the queue in samples for the kind of data you are enqueueing
  /// \param channelMap Channel map for the data
  /// \return the size of the queue in samples
  virtual int32_t getQueueSize(ChannelMap channelMap) const = 0;

  /// Enqueue interleaved float buffers of data.
  /// \param interleavedBuffer Interleaved float buffer
  /// \param numTotalSamples Number of total samples in buffer (including all channels)
  /// \param channelMap The channel map for the data being enqueued
  /// \return Number of samples successfully been queued. Should be the same as numTotalSamples.
  virtual int32_t
  enqueueData(const float* interleavedBuffer, int32_t numTotalSamples, ChannelMap channelMap) = 0;

  /// Enqueue interleaved 16 bit int buffers of data.
  /// \param interleavedBuffer Interleaved 16 bit int buffer
  /// \param numTotalSamples Number of total samples in buffer (including all channels)
  /// \param channelMap The channel map for the data being enqueued
  /// \return Number of samples successfully been queued. Should be the same as numTotalSamples.
  virtual int32_t
  enqueueData(const int16_t* interleavedBuffer, int32_t numTotalSamples, ChannelMap channelMap) = 0;

  /// Enqueue silence for a specific channel map configuration
  /// \param channelMap The channel map for the data being enqueued
  /// \param numTotalSamples Number of total samples in buffer (including all channels)
  /// \return Number of samples successfully been queued. Should be the same as numTotalSamples.
  virtual int32_t enqueueSilence(int32_t numTotalSamples, ChannelMap channelMap) = 0;

  /// Flushes data / clears the audio queue. It also resets the endOfStream flag (if specified in
  /// enqueueData()) to false.
  virtual void flushQueue() = 0;

  /// \return The number of samples dequeued and processed per channel.
  virtual uint64_t getNumSamplesDequeuedPerChannel() const = 0;

  /// Specify if the end of the stream has been reached. This ensures that data that might be less
  /// than the buffer size is dequeued correctly
  virtual void setEndOfStream(bool endOfStream) = 0;

  /// Returns true if the end of stream flag has been set using setEndOfStream(..)
  virtual bool getEndOfStreamStatus() const = 0;

 protected:
  virtual ~SpatDecoderQueue() {}
};

class SpeakersVirtualizer : public TransportControl {
 public:
  /// Enqueue interleaved float buffers of data.
  /// This method MUST be called consistently on the same thread!
  /// \param interleavedBuffer Interleaved float buffer. The number of channels MUST be the same as
  /// the
  ///        number of speakers given to the \ref createSpeakersVirtualizer() function
  /// \param numTotalSamples Number of total samples in buffer (this includes all channels)
  /// \param numEnqueued Filled in with the number of samples successfully queued.
  /// \param endOfStream Optional parameter to indicate the end of stream, which will allow the
  /// audio callback
  ///        to consume the remaining samples in the audio queues, even if they are less than the
  ///        required number of samples for the callback
  /// \return EngineError::OK if all samples have been queued
  ///         EngineError::QUEUE_FULL if some samples have been queued (check numEnqueued)
  ///         EngineError::BAD_THREAD if this method is called from another thread
  ///         EngineError::INVALID_BUFFER_SIZE if the numTotalSamples is not divisible by the
  ///         channel count EngineError::FAIL if there are no speaker objects
  virtual EngineError enqueueData(
      const float* interleavedBuffer,
      int32_t numTotalSamples,
      int32_t& numEnqueued,
      bool endOfStream = false) = 0;

  /// Enqueue interleaved int16_t buffers of data.
  /// This method MUST be called consistently on the same thread!
  /// \param interleavedBuffer Interleaved float buffer. The number of channels MUST be the same as
  /// the
  ///        number of speakers given to the \ref createSpeakersVirtualizer() function
  /// \param numTotalSamples Number of total samples in buffer (this includes all channels)
  /// \param numEnqueued Filled in with the number of samples successfully queued.
  /// \param endOfStream Optional parameter to indicate the end of stream, which will allow the
  /// audio callback
  ///        to consume the remaining samples in the audio queues, even if they are less than the
  ///        required number of samples for the callback
  /// \return EngineError::OK if all samples have been queued
  ///         EngineError::QUEUE_FULL if some samples have been queued (check numEnqueued)
  ///         EngineError::BAD_THREAD if this method is called from another thread
  ///         EngineError::INVALID_BUFFER_SIZE if the numTotalSamples is not divisible by the
  ///         channel count EngineError::FAIL if there are no speaker objects
  virtual EngineError enqueueData(
      const int16_t* interleavedBuffer,
      int32_t numTotalSamples,
      int32_t& numEnqueued,
      bool endOfStream = false) = 0;

  /// Set a function to receive events from this object. Currently Event::ERROR_BUFFER_UNDERRUN
  /// is sent when the audio callback requested more samples that are enqueued and end of stream
  /// has not been signaled
  /// \param callback Event callback function
  /// \param userData User data. Can be nullptr
  /// \return Relevant error or EngineError::OK
  virtual EngineError setEventCallback(EventCallback callback, void* userData) = 0;

  /// Get the free space in the queue (in total number of samples for all channels)
  /// for the kind of data you are enqueueing
  /// \return The free space of the queue in samples
  virtual int32_t getFreeSpaceInQueue() const = 0;

  /// The size of the queue in samples for the kind of data you are enqueueing
  /// \return the size of the queue in samples
  virtual int32_t getQueueSize() const = 0;

  /// Flushes data / clears the audio queues. It also resets the endOfStream flag
  virtual void flushQueue() = 0;

  /// Specify if the end of the stream has been reached. This ensures that data that might be less
  /// than the buffer size is dequeued correctly
  virtual void setEndOfStream(bool endOfStream) = 0;

  /// Returns true if the end of stream flag has been set using setEndOfStream(..)
  virtual bool getEndOfStreamStatus() const = 0;

  /// \return The number of samples dequeued and processed per channel.
  virtual uint64_t getNumSamplesDequeuedPerChannel() const = 0;

  /// Set the volume in linear gain, with an optional ramp time
  /// \param linearGain Linear gain, where 0 is mute and 1 is unity gain
  /// \param rampTimeMs Ramp time to the new gain value in milliseconds. If 0, no ramp will be
  /// applied. \param forcePreviousRamp This forces a previous ramp message (if any) to conclude
  /// right away
  virtual void setVolume(float linearGain, float rampTimeMs, bool forcePreviousRamp = false) = 0;

  /// Set the volume in decibels, with an optional ramp time
  /// \param dB Volume in decibels, where 0 is unity gain.
  /// \param rampTimeMs Ramp time to the new gain value in milliseconds. If 0, no ramp will be
  /// applied. \param forcePreviousRamp This forces a previous ramp message (if any) to conclude
  /// right away
  virtual void setVolumeDecibels(float dB, float rampTimeMs, bool forcePreviousRamp = false) = 0;

  /// \return The current volume
  /// \see setVolume, setVolumeDecibels
  virtual float getVolume() const = 0;

  /// \return The current volume in decibels
  /// \see setVolume, setVolumeDecibels
  virtual float getVolumeDecibels() const = 0;

 protected:
  virtual ~SpeakersVirtualizer(){};
};

class SpatDecoderFile : public SpatDecoderInterface {
 public:
  /// Opens an asset for playback. Currently .wav, .opus and .tbe files are supported.
  /// While the asset is opened synchronously, it is loaded into the streaming buffer
  /// asynchronously. An Event::DECODER_INIT event will be dispatched to the event callback when the
  /// streaming buffer is ready for the asset to play. \see  setEventCallback, close \param
  /// nameAndPath Name and path to the file \param map The channel mapping / spatial audio format of
  /// the file \return Relevant error or EngineError::OK
  virtual EngineError open(const char* nameAndPath, ChannelMap map = ChannelMap::TBE_8_2) = 0;

  /// Opens IOStream object for playback. Currently .wav, .opus, and .tbe codecs are supported.
  /// Two instances of IOStream are required as SpatDecoderFile might use two streams for seamless
  /// synchronisation. While the asset is opened synchronously, it is loaded into the streaming
  /// buffer asynchronously. An Event::DECODER_INIT event will be dispatched to the event callback
  /// when the streaming buffer is ready for the asset to play. \see  setEventCallback, close \param
  /// streams Two instances of IOStream for the same audio asset that must be played \param
  /// shouldOwnStreams If the lifecycle of the streams must be owned by this object \param map The
  /// channel mapping / spatial audio format of the file \return Relevant error or EngineError::OK
  virtual EngineError
  open(TBE::IOStream* streams[2], bool shouldOwnStreams, ChannelMap map = ChannelMap::TBE_8_2) = 0;

  /// Opens an asset for playback using an asset descriptor. Useful for playing back chunks within a
  /// larger file. Currently .wav, .opus, and .tbe codecs are supported. While the asset is opened
  /// synchronously, it is loaded into the streaming buffer asynchronously. An Event::DECODER_INIT
  /// event will be dispatched to the event callback when the streaming buffer is ready for the
  /// asset to play.
  /// \see  setEventCallback, close
  /// \param nameAndPath Name and path to the file
  /// \param ad AssetDescriptor for the file
  /// \param map The channel mapping / spatial audio format of the file
  /// \return Relevant error or EngineError::OK
  virtual EngineError
  open(const char* nameAndPath, AssetDescriptor ad, ChannelMap map = ChannelMap::TBE_8_2) = 0;

  /// Close an open file or stream objects and release resources
  virtual void close() = 0;

  /// Returns true if a file or stream is open
  virtual bool isOpen() const = 0;

  /// Seek to a specific time in samples
  /// Thread safe.
  /// \param timeInSamples Time in samples to seek to
  /// \return OK if successful or FAIL otherwise
  virtual EngineError seekToSample(size_t timeInSamples) = 0;

  /// Seek to a specific time in milliseconds
  /// Thread safe.
  /// \param timeInMs Time in milliseconds to seek to
  /// \return OK if successful or FAIL otherwise
  virtual EngineError seekToMs(float timeInMs) = 0;

  /// Returns the elapsed playback time in samples
  /// Thread safe.
  virtual size_t getElapsedTimeInSamples() const = 0;

  /// Returns the elapsed playback time in milliseconds
  /// Thread safe.
  virtual double getElapsedTimeInMs() const = 0;

  /// Returns the duration of the asset in samples.
  /// Thread safe.
  virtual size_t getAssetDurationInSamples() const = 0;

  /// Returns the duration of the asset in milliseconds.
  /// Thread safe.
  virtual float getAssetDurationInMs() const = 0;

  /// Set synchronisation mode. Use setExternalClockInMs() if using external sync to specify the
  /// time. \param syncMode SyncMode::INTERNAL for internal synchronisation or SyncMode::EXTERNAL
  /// for an external source.
  virtual void setSyncMode(SyncMode syncMode) = 0;

  /// Returns the current synchronisation mode. SyncMode::INTERNAL by default
  virtual SyncMode getSyncMode() const = 0;

  /// Set the external time source in milliseconds when using SyncMode::EXTERNAL
  /// \param externalClockInMs Elapsed time in milliseconds
  virtual void setExternalClockInMs(double externalClockInMs) = 0;

  /// Set how often the engine tries to synchronise to the external clock. Very low values can
  /// result in stutter, very high values can result in synchronisation latency This method along
  /// with setResyncThresholdMs() can be used to fine-tune synchronisation. \param freewheelInMs
  /// Freewheel time or how often the engine tries to synchronise to the external clock
  virtual void setFreewheelTimeInMs(double freewheelInMs) = 0;

  /// Returns the current freewheel time
  virtual double getFreewheelTimeInMs() = 0;

  /// The time threshold after which the engine will synchronise to the external clock.
  /// This method along with setFreewheelTimeInMs() can be used to fine-tune synchronisation.
  /// \param resyncThresholdMs Synchronisation threshold in milliseconds
  virtual void setResyncThresholdMs(double resyncThresholdMs) = 0;

  /// Returns current synchronisation threshold
  virtual double getResyncThresholdMs() const = 0;

  /// Apply a volume fade over a period of time. The ramp is applied immediately with the current
  /// volume being set to startLinearGain and then ramping to endLinearGain over fadeDurationMs
  /// \param startLinearGain Linear gain value at the start of the fade, where 0 is mute and 1 is
  /// unity gain \param endLinearGain Linear gain value at the end of the fade, where 0 is mute and
  /// 1 is unity gain \param fadeDurationMs Duration of the ramp in milliseconds
  virtual void
  applyVolumeFade(float startLinearGain, float endLinearGain, float fadeDurationMs) = 0;

  /// Enable looping of the currently loaded file. Use this method for sample accurate looping
  /// rather than manually seeking the file to 0 when it finishes playing. \param shouldLoop Looping
  /// is enabled if true
  virtual void enableLooping(bool shouldLoop) = 0;

  /// Returns true if looping is enabled
  virtual bool loopingEnabled() const = 0;

 protected:
  virtual ~SpatDecoderFile() {}
};

/// A class for decoding compressed and uncompressed audio formats. The source could be a file or
/// data stream. Supported codecs:
/// 1. Opus stream (opus packets from demuxers, use TBE_CreateAudioFormatDecoderFromHeader)
/// 2. Opus file (*.opus files)
/// 3. WAV files (including broadcast WAV)
/// 4. Native codecs on iOS and macOS
class AudioFormatDecoder {
 public:
  enum class Info {
    PRE_SKIP /// Some codecs such as opus have a decoding latency. AudioFormatDecoder already
             /// compensates for this.
  };

  virtual ~AudioFormatDecoder(){};

  /// Returns the number of channels in the audio format
  /// \return Number of channels in the audio format
  virtual int32_t getNumOfChannels() const = 0;

  /// Returns the number of total samples, including all channels.
  /// \return Total number of samples, including all channels.
  virtual size_t getNumTotalSamples() const = 0;

  /// Returns the number of samples in a single channel;
  /// \return  Number of samples in a single channel
  virtual size_t getNumSamplesPerChannel() const = 0;

  /// Returns the number of milliseconds in a single channel;
  /// \return  Number of milliseconds in a single channel
  virtual double getMsPerChannel() const = 0;

  /// Returns the current sample position. In some cases this will always return zero if the audio
  /// packets do not have timing information (such as opus). \return The current sample position.
  virtual size_t getSamplePosition() = 0;

  /// Seek to a sample position. This is only applicable when using an audio file or stream.
  /// \return EngineError::OK on success or corresponding error
  virtual EngineError seekToSample(size_t samplePosition) = 0;

  /// Decode a packet of audio data into pcm float samples. This method must only be used for
  /// in-place processing, such as when you are managing a file stream yourself. \param data Input
  /// audio data \param dataSize Size of input audio data in bytes \param bufferOut Audio buffer to
  /// write decoded samples into (interleaved for multichannel audio) \param numOfSamplesInBuffer
  /// Number of total samples in buffer (including all channels for multichannel audio). The number
  /// of samples should not be greater getMaxBufferSizePerChannel() * getNumOfChannels() \return
  /// Number of samples decoded and written into bufferOut
  virtual size_t
  decode(const char* data, size_t dataSize, float* bufferOut, int32_t numOfSamplesInBuffer) = 0;

  /// Decode an open stream/file into pcm float samples. This method must only be used when
  /// AudioFormatDecoder is also managing the stream (such as a file). \param bufferOut Audio buffer
  /// to write decoded samples into (interleaved for multichannel audio) \param numOfSamplesInBuffer
  /// Number of total samples in buffer (including all channels for multichannel audio). The number
  /// of samples should not be greater getMaxBufferSizePerChannel() * getNumOfChannels() \return
  /// Number of samples decoded and written into bufferOut
  virtual size_t decode(float* bufferOut, int32_t numOfSamplesInBuffer) = 0;

  /// Returns the sample rate of the audio format
  /// \return The sample rate of the audio format in Hz.
  virtual float getSampleRate() const = 0;

  /// Returns the output sample rate. The audio would be resampled to this rate if the AudioFormat
  /// allows for it. \return The output sample rate in Hz.
  virtual float getOutputSampleRate() const = 0;

  /// Returns the number of bits in the audio format. (Eg: 24 bit or 16 bit wav file)
  /// \return The number of bits in the audio format.
  virtual int32_t getNumBits() const = 0;

  /// Returns true if the end of the stream has been reached. This method must only be used when
  /// AudioFormatDecoder is also managing the stream (such as a file). \return True if the end of
  /// the stream has been reached
  virtual bool endOfStream() = 0;

  /// Returns true if there was an error while decoding. If decode(..) returns 0, it is likely that
  /// either endOfStream() or decoderError() will return true.
  virtual bool decoderError() = 0;

  /// \return The maximum number of samples allocated for each channel in internal buffers.
  virtual int32_t getMaxBufferSizePerChannel() const = 0;

  /// \return The name of audio format (Eg: opus, wav)
  virtual const char* getName() const = 0;

  /// Flushes the internal buffers and resets the state of the decoder. resetToZero must be set to
  /// true if the audio format stream was reset to its initial position (i.e rewound to the start of
  /// the stream).
  virtual void flush(bool resetToZero = false) = 0;

  /// \return Returns information related to the audio format/codec.
  virtual int32_t getInfo(Info info) = 0;
};
} // namespace TBE

extern "C" {

/// Create a new instance of the AudioEngine.
/// \param engine Pointer to the engine instance
/// \param initSettings Initialisation settings. Can be TBE::EngineInitSettings_default.
/// \see TBE::EngineInitSettings for all the initialisation settings.
/// \return Relevant error or EngineError::OK
API_EXPORT TBE::EngineError TBE_CreateAudioEngine(
    TBE::AudioEngine*& engine,
    TBE::EngineInitSettings initSettings = TBE::EngineInitSettings());

/// Destroy an existing instance of AudioEngine. All child objects (such as SpatDecoderQueue will be
/// destroyed and made invalid). \param engine Pointer to the engine instance
API_EXPORT void TBE_DestroyAudioEngine(TBE::AudioEngine*& engine);

/// Create an instance of AudioFormatDecoder from header information. The object can be de-allocated
/// with delete. \param decoder A null reference to AudioFormatDecoder \param headerData Header data
/// \param headerDataSize Size of the header data in bytes
/// \return Relevant error or EngineError::OK
API_EXPORT TBE::EngineError TBE_CreateAudioFormatDecoderFromHeader(
    TBE::AudioFormatDecoder*& decoder,
    const char* headerData,
    size_t headerDataSize);

/// Create an instance of AudioFormatDecoder from a file. The object can be de-allocated with
/// delete. Formats currently supported: wav, .tbe. \param decoder A null reference to
/// AudioFormatDecoder \param file Path to file \param maxBufferSizePerChannel Maximum buffer size
/// per channel in samples. This is used to allocate internal buffers and should typically match the
/// size specified in the decode(..) method. \param outputSampleRate Output sample rate. The audio
/// format will be resampled to match this rate, if required. Setting this to 0 would disable
/// resampling and set the output sample rate to match the sample rate of the audio format. \return
/// Relevant error or EngineError::OK
API_EXPORT TBE::EngineError TBE_CreateAudioFormatDecoder(
    TBE::AudioFormatDecoder*& decoder,
    const char* file,
    int maxBufferSizePerChannel,
    float outputSampleRate);
}

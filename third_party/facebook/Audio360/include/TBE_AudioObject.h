/*
 * Copyright (c) 2018-present, Facebook, Inc.
 */

#pragma once

#include "TBE_AudioEngine.h"

namespace TBE {
class AudioObject : public SpatDecoderInterface {
 public:
  /// Callback for providing PCM data to the AudioObject from the client rather than a file.
  /// If stereo data is provided, the data must be interleaved
  /// \param channelBuffer The audio channel buffer that the client needs to fill with data
  /// \param numSamples The number of samples the client must fill in the buffer
  /// \param numChannels The number of interleaved channels in the buffer
  /// \param userData pointer to custom user data.
  typedef void (
      *BufferCallback)(float* channelBuffer, size_t numSamples, size_t numChannels, void* userData);

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

  /// Use this function to set a callback for audio data to be provided by the client
  /// The client must ensure the callback pointer is valid for the lifetime of the AudioObject
  /// If a file has been previously opened with the \ref open() call it will be closed
  /// \param BufferCallback The callback pointer to be called for providing audio
  /// \param numChannels The number of channels of audio data the client wants to provide (max is 2)
  /// \param userData Pointer to custom user data that will be passed to the client on each
  /// invocation
  virtual EngineError
  setAudioBufferCallback(BufferCallback callback, size_t numChannels, void* userData) = 0;

  /// Opens an asset for playback. Currently mono/stereo .wav and .opus files are supported across
  /// all platforms. Native codecs are supported on iOS and macOS. While the asset is opened
  /// synchronously, it is loaded into the streaming buffer asynchronously. An Event::DECODER_INIT
  /// event will be dispatched to the event callback when the streaming buffer is ready for the
  /// asset to play. If a callback has been provided with \ref setAudioBufferCallback, the callback
  /// will be unset. \see  setEventCallback, close \param nameAndPath Name and path to the file
  /// \return Relevant error or EngineError::OK
  virtual EngineError open(const char* nameAndPath) = 0;

  /// Opens an asset for playback using an asset descriptor. Useful for playing back chunks within a
  /// larger file. Currently mono/stereo .wav and .opus files are supported across all platforms.
  /// Native codecs are supported on iOS and macOS. While the asset is opened synchronously, it is
  /// loaded into the streaming buffer asynchronously. An Event::DECODER_INIT event will be
  /// dispatched to the event callback when the streaming buffer is ready for the asset to play.
  /// \see  setEventCallback, close
  /// \param nameAndPath Name and path to the file
  /// \param ad AssetDescriptor for the file
  /// \param map The channel mapping / spatial audio format of the file
  /// \return Relevant error or EngineError::OK
  virtual EngineError open(const char* nameAndPath, AssetDescriptor ad) = 0;

  /// Close an open file and release resources
  virtual void close() = 0;

  /// Returns true if a file is open
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
  virtual float getAssetDurationInMs() = 0;

  /// Set a function to receive events from this object. Currently Event::DECODER_INIT is sent when
  /// the a file is loaded and ready for playback. \param callback Event callback function \param
  /// userData User data. Can be nullptr \return Relevant error or EngineError::OK
  virtual EngineError setEventCallback(EventCallback callback, void* userData) = 0;

  /// Set spatialisation on or off.
  /// When spatialisation is enabled, the setPosition(..) method has an effect on how the sound
  /// source is perceived. Default: spatialisation is enabled \param spatialise When false, the
  /// object is no longer spatialised
  virtual void shouldSpatialise(bool spatialise) = 0;

  /// Returns true if the object is spatialised
  virtual bool isSpatialised() = 0;

  /// Set looping on/off. Only applicable to file-backed AudioObjects, does nothing when a callback
  /// is used \param loop When true, the audio object will loop back to the beginning \return true
  /// if the object is cabpable of looping (e.g. it is streamed from a file), false if not
  virtual bool enableLooping(bool loop) = 0;

  /// Returns whether the object is looping or not
  virtual bool loopingEnabled() = 0;

  /// Set the distance attenuation mode.
  /// Default is AttenuationMode::LOGARITHMIC
  /// \param AttenuationMode Attenuation mode: logarithmic, linear, disabled
  virtual void setAttenuationMode(AttenuationMode mode) = 0;

  /// \return The current distance attenuation mode
  virtual AttenuationMode getAttenuationMode() const = 0;

  /// Set the attenuation properties, only applicable if the attenuation mode is either set to
  /// logarithmic or linear. \param AttenuationProps Attenuation properties including minimum
  /// distance, maximum distance, attenuation factor and max distance mute
  virtual void setAttenuationProperties(AttenuationProps props) = 0;

  /// \return The attenuation properties
  virtual AttenuationProps getAttenuationProperties() const = 0;

  /// Set the pitch multiplier. E.g a value of 2 will get the sound to playback at twice the speed
  /// and twice the pitch. This feature only works for files/streams and will not have any effect if
  /// audio is provided via the buffer callback. \param pitch Pitch multiplier, between a range of
  /// 0.001 and 4
  virtual void setPitch(float pitch) = 0;

  /// Returns the current pitch
  /// \return the pitch (as specified in setPitch())
  virtual float getPitch() const = 0;

 protected:
  virtual ~AudioObject() {}
};
} // namespace TBE

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>



class AudioPluginAudioProcessor final : public juce::AudioProcessor {
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool must_repaint = false;
    juce::Image spectrogram = { juce::Image::RGB, 512, 512, true };

private:
    void pushNextSampleIntoFifo(float sample, juce::MidiBuffer& midiMessages) noexcept;

	juce::AudioParameterFloat* _gain;
	juce::AudioParameterFloat* _threshold;
	juce::AudioParameterInt* _voices;

    constexpr static std::size_t _fft_order = 10;
    constexpr static std::size_t _fft_size = 1 << _fft_order;
    std::array<float, 2 * _fft_size> _fft_data = {};
    std::array<float, _fft_size> _fifo = {};
    std::size_t _fifo_index = 0;
    juce::dsp::FFT _fft = { _fft_order };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};

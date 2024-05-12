#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class AudioPluginAudioProcessor final : public juce::AudioProcessor {
public:
    bool must_repaint = false;
    juce::Image spectrogram = { juce::Image::RGB, 512, 512, true };

    AudioPluginAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    ~AudioPluginAudioProcessor() override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
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

private:
    double _sample_rate;
    std::size_t _fft_order;
    std::size_t _resampling_factor;
    std::size_t _fft_size;
    std::vector<float> _fft_data = {};
    std::vector<float> _fifo = {};
    std::size_t _fifo_index = 0;
    std::unique_ptr<juce::dsp::FFT> _fft = nullptr;

	juce::AudioParameterFloat* _gain;
	juce::AudioParameterFloat* _threshold;
	juce::AudioParameterInt* _voices;
	juce::AudioParameterInt* _lowest;
	juce::AudioParameterInt* _range;
	juce::AudioParameterFloat* _resolution;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};

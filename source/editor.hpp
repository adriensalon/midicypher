#pragma once

#include "processor.hpp"

class AudioPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:    
    void timerCallback() override;

    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& _processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};

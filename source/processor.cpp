#include "processor.hpp"
#include "editor.hpp"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // addParameter(_cutoff = new juce::AudioParameterFloat("cutoff_param_id", "Cutoff", 0.f, 1.f, 0.8f));
    addParameter(_voices = new juce::AudioParameterInt("voices_param_id", "Voices", 1, 12, 6));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AudioPluginAudioProcessor::producesMidi() const
{
    return true;
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
    return true;
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);    
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    juce::ignoreUnused(layouts);
    return true;
}


template <std::size_t N>
std::size_t findMaxIndex(const std::array<float, N>& data, std::size_t size) {
    const auto maxIter = std::max_element(data.begin(), data.begin() + size);
    return std::distance(data.begin(), maxIter);
}

float binToFrequency(std::size_t binIndex, std::size_t fftSize, double sampleRate) {
    return static_cast<float>(binIndex * sampleRate / fftSize);
}

int freqToMidi(float frequency) {
    float midiNote = 69.f + 12.f * std::log2f(frequency / 440.f);
    return static_cast<int>(std::roundf(midiNote));
}

void AudioPluginAudioProcessor::pushNextSampleIntoFifo(float sample, juce::MidiBuffer& midiMessages) noexcept
{
    if (_fifo_index == _fft_size) {        
        std::fill(_fft_data.begin(), _fft_data.end(), 0.0f);
        std::copy(_fifo.begin(), _fifo.end(), _fft_data.begin());

        auto rightHandEdge = spectrogram.getWidth() - 1;
        auto imageHeight   = spectrogram.getHeight();
        spectrogram.moveImageSection (0, 0, 1, 0, rightHandEdge, imageHeight);
        _fft.performFrequencyOnlyForwardTransform (_fft_data.data());
        auto maxLevel = juce::FloatVectorOperations::findMinAndMax(_fft_data.data(), _fft_size / 2);
        for (auto y = 1; y < imageHeight; ++y) {
            auto skewedProportionY = 1.0f - std::exp (std::log ((float) y / (float) imageHeight) * 0.2f);
            auto fftDataIndex = (size_t) juce::jlimit<int> (0, _fft_size / 2, (int) (skewedProportionY * _fft_size / 2));
            auto level = juce::jmap (_fft_data[fftDataIndex], 0.0f, juce::jmax (maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
            spectrogram.setPixelAt (rightHandEdge, y, juce::Colour::fromHSV (level, 1.0f, level, 1.0f));
        }
        const auto _max_index = findMaxIndex(_fft_data, _fft_size / 2);
        const auto _max_frequency = binToFrequency(_max_index, _fft_size, getSampleRate());
        const auto _max_note = freqToMidi(_max_frequency);
        midiMessages.addEvent(juce::MidiMessage::noteOn(0, _max_note, 0.5f), 0);

        must_repaint = true;
        _fifo_index = 0;
    }
    _fifo[(size_t) _fifo_index++] = sample;
}



void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() > 0) {
        auto* channelData = buffer.getReadPointer(0);
        for (auto i = 0; i < buffer.getNumSamples(); ++i) {
            pushNextSampleIntoFifo(channelData[i], midiMessages);
        }
    }
}

bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream(destData, true).writeInt(*_voices);

    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    *_voices = juce::MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false).readInt();

    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

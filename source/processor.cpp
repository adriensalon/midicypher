#include "processor.hpp"
#include "editor.hpp"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(_gain = new juce::AudioParameterFloat("gain_param_id", "Gain", 0.f, 12.f, 1.f));
    addParameter(_threshold = new juce::AudioParameterFloat("threshold_param_id", "Threshold", 0.f, 1.f, 0.f));
    addParameter(_voices = new juce::AudioParameterInt("voices_param_id", "Voices", 1, 12, 6));
    addParameter(_lowest = new juce::AudioParameterInt("lowest_param_id", "Lowest", 0, 127, 0));
    addParameter(_range = new juce::AudioParameterInt("range_param_id", "Range", 0, 127, 127));
    addParameter(_resolution = new juce::AudioParameterFloat("resolution_param_id", "Resolution", 0.1f, 20.f, 1.f));
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    _sample_rate = sampleRate;

    // processing
    _fft_order = 10;
    _resampling_factor = 1;

    _fft_size = static_cast<std::size_t>(1) << _fft_order;
    _fft_data.resize(2 * _fft_size);
    _fifo.resize(_fft_size);
    _fifo_index = 0;
    _fft = std::make_unique<juce::dsp::FFT>(static_cast<int>(_fft_order));
    spectrogram.clear(spectrogram.getBounds());
    juce::ignoreUnused(samplesPerBlock);
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() > 0) {
        auto* channelData = buffer.getReadPointer(0);
        for (auto _index = 0; _index < buffer.getNumSamples(); ++_index) {
            if (_fifo_index == _fft_size) {
                std::fill(_fft_data.begin(), _fft_data.end(), 0.0f);
                std::copy(_fifo.begin(), _fifo.end(), _fft_data.begin());
                _fifo_index = 0;

                _fft->performFrequencyOnlyForwardTransform(_fft_data.data());

                const auto _max_iter = std::max_element(_fft_data.begin(), _fft_data.begin() + _fft_size);
                const auto _max_index = std::distance(_fft_data.begin(), _max_iter);
                const auto _max_frequency = static_cast<float>(_max_index * _sample_rate / _fft_size);
                const auto _max_note_coarse = 69.f + 12.f * std::log2f(_max_frequency / 440.f);
                const auto _max_note = static_cast<int>(std::roundf(_max_note_coarse));
                midiMessages.addEvent(juce::MidiMessage::noteOn(0, _max_note, 0.5f), 0);

                auto rightHandEdge = spectrogram.getWidth() - 1;
                auto imageHeight = spectrogram.getHeight();
                spectrogram.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);
                auto maxLevel = juce::FloatVectorOperations::findMinAndMax(_fft_data.data(), _fft_size / 2);
                for (auto y = 1; y < imageHeight; ++y) {
                    auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 0.2f);
                    auto fftDataIndex = (size_t)juce::jlimit<int>(0, static_cast<int>(_fft_size) / 2, (int)(skewedProportionY * static_cast<int>(_fft_size) / 2));
                    auto level = juce::jmap(_fft_data[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
                    spectrogram.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level / 2.f, 1.0f, level, 1.0f));
                }
                must_repaint = true;
            }
            _fifo[(size_t)_fifo_index++] = channelData[_index];
        }
    }
}

// -------------------------------------------------------

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

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    juce::ignoreUnused(layouts);
    return true;
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

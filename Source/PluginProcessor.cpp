/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleSamplerAudioProcessor::SimpleSamplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#else
     : apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Load the mapping XML from BinaryData
    int xmlSize = 0;
    const char* xmlData = BinaryData::getNamedResource ("mapping_xml", xmlSize);
    
    if (xmlData != nullptr) {
        sampler.loadSamplesFromXml (xmlData, xmlSize);
        mixer.loadFromXml (xmlData, xmlSize);
        mixer.assignParameters (apvts);
    }
}

SimpleSamplerAudioProcessor::~SimpleSamplerAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleSamplerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleSamplerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleSamplerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleSamplerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleSamplerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleSamplerAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    sampler.prepareToPlay(sampleRate, samplesPerBlock);
    mixer.prepare(sampleRate, samplesPerBlock);
    samplerOutputBuffer.setSize(sampler.getNumOutputChannels(), samplesPerBlock);
}

void SimpleSamplerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
         return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Clear the main buffer because the sampler adds to it
    for (int i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Resize buffer if needed (e.g. if XML loaded after prepareToPlay)
    if (samplerOutputBuffer.getNumChannels() != sampler.getNumOutputChannels() || samplerOutputBuffer.getNumSamples() != buffer.getNumSamples())
        samplerOutputBuffer.setSize(sampler.getNumOutputChannels(), buffer.getNumSamples());

    samplerOutputBuffer.clear();
    
    sampler.processBlock(samplerOutputBuffer, midiMessages);
    mixer.processBlock(samplerOutputBuffer, buffer);
}

//==============================================================================
bool SimpleSamplerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleSamplerAudioProcessor::createEditor()
{
    return new SimpleSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleSamplerAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleSamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    int xmlSize = 0;
    const char* xmlData = BinaryData::getNamedResource ("mapping_xml", xmlSize);

    if (xmlData != nullptr && xmlSize > 0)
    {
        juce::XmlDocument doc (juce::String::createStringFromData (xmlData, xmlSize));
        auto root = doc.getDocumentElement();

        if (root != nullptr && root->hasTagName ("Mappings"))
        {
            auto* mixerNode = root->getChildByName ("Mixer");
            if (mixerNode != nullptr)
            {
                for (auto* child : mixerNode->getChildIterator())
                {
                    if (child->hasTagName ("Group"))
                    {
                        juce::String type = child->getStringAttribute ("type");
                        juce::String name = child->getStringAttribute ("name");

                        if (type.equalsIgnoreCase ("ambisonic"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Azimuth", 1 }, name + " Azimuth", -180.0f, 180.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Elevation", 1 }, name + " Elevation", -90.0f, 90.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", 0.0f, 2.0f, 1.0f));
                        }
                        else if (type.equalsIgnoreCase ("stereo"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", 0.0f, 2.0f, 1.0f));
                        }
                        else if (type.equalsIgnoreCase ("mono"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                        }

                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

                        // EQ
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_EQ_On", 1 }, name + " EQ On", true));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_LS_Freq", 1 }, name + " EQ LS Freq", 20.0f, 1000.0f, 100.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_LS_Gain", 1 }, name + " EQ LS Gain", -15.0f, 15.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B1_Freq", 1 }, name + " EQ B1 Freq", 100.0f, 5000.0f, 500.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B1_Q", 1 }, name + " EQ B1 Q", 0.1f, 10.0f, 1.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B1_Gain", 1 }, name + " EQ B1 Gain", -15.0f, 15.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B2_Freq", 1 }, name + " EQ B2 Freq", 500.0f, 10000.0f, 2000.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B2_Q", 1 }, name + " EQ B2 Q", 0.1f, 10.0f, 1.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_B2_Gain", 1 }, name + " EQ B2 Gain", -15.0f, 15.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_HS_Freq", 1 }, name + " EQ HS Freq", 1000.0f, 20000.0f, 5000.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_HS_Gain", 1 }, name + " EQ HS Gain", -15.0f, 15.0f, 0.0f));

                        // Compressor
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Comp_On", 1 }, name + " Comp On", true));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Attack", 1 }, name + " Comp Attack", 0.1f, 100.0f, 10.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Release", 1 }, name + " Comp Release", 10.0f, 1000.0f, 100.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Thresh", 1 }, name + " Comp Thresh", -60.0f, 0.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Ratio", 1 }, name + " Comp Ratio", 1.0f, 20.0f, 1.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Gain", 1 }, name + " Comp Gain", 0.0f, 24.0f, 0.0f));
                    }
                }
            }
        }
    }

    return { params.begin(), params.end() };
}

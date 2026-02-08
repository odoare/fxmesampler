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
        sampler.assignParameters (apvts);
        mixer.loadFromXml (xmlData, xmlSize);
        mixer.assignParameters (apvts);
    }

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String resourceName = BinaryData::namedResourceList[i];
        if (resourceName.endsWithIgnoreCase ("_xml") && resourceName != "mapping_xml")
        {
            juce::String displayName = resourceName.dropLastCharacters(4).replaceCharacter('_', ' ');
            presets.push_back({ displayName, BinaryData::namedResourceList[i] });
        }
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
    return juce::jmax(1, (int)presets.size());
}

int SimpleSamplerAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void SimpleSamplerAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < (int)presets.size())
    {
        currentProgram = index;
        int size = 0;
        const char* data = BinaryData::getNamedResource(presets[index].resourceName, size);
        if (data != nullptr && size > 0)
        {
            setStateInformation(data, size);
        }
    }
}

const juce::String SimpleSamplerAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < (int)presets.size())
        return presets[index].name;
    return "Default";
}

void SimpleSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (index >= 0 && index < (int)presets.size())
        presets[index].name = newName;
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
    
    sampler.updateParams();
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    if (xml != nullptr)
    {
        destData.setSize (0);
        juce::MemoryOutputStream stream (destData, false);
        xml->writeTo (stream);

        auto xmlFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("samplerdata.xml");

        if (!xmlFile.getParentDirectory().exists())
            xmlFile.getParentDirectory().createDirectory();

        // if (xml->writeTo (xmlFile))
        //     // DBG ("Saved samplerdata.xml to: " + xmlFile.getFullPathName());
        //     std::cout << "Saved to:" << xmlFile.getFullPathName() << std::endl;
        // else
        //     // DBG ("Failed to save samplerdata.xml to: " + xmlFile.getFullPathName());
        //     std::cout << "Failed to save to:" << xmlFile.getFullPathName() << std::endl;
    }
}

void SimpleSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (juce::XmlDocument::parse (juce::String::createStringFromData (data, sizeInBytes)));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
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
            // Parse SampleGroups for parameters
            for (auto* child : root->getChildIterator())
            {
                if (child->hasTagName ("SampleGroup"))
                {
                    juce::String name = child->getStringAttribute ("name");
                    bool oneShot = child->getBoolAttribute ("oneShot", true);
                    float attack = (float) child->getDoubleAttribute ("attack", 0.001);
                    float decay = (float) child->getDoubleAttribute ("decay", 0.0);
                    float sustain = (float) child->getDoubleAttribute ("sustain", 1.0);
                    float release = (float) child->getDoubleAttribute ("release", 0.1);
                    float detune = (float) child->getDoubleAttribute ("detune", 0.0);

                    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_OneShot", 1 }, name + " One Shot", oneShot));
                    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Attack", 1 }, name + " Attack", 0.0f, 5.0f, attack));
                    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Decay", 1 }, name + " Decay", 0.0f, 5.0f, decay));
                    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Sustain", 1 }, name + " Sustain", 0.0f, 1.0f, sustain));
                    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Release", 1 }, name + " Release", 0.0f, 5.0f, release));
                    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Detune", 1 }, name + " Detune", -12.0f, 12.0f, detune));
                }
            }

            auto* mixerNode = root->getChildByName ("Mixer");
            if (mixerNode != nullptr)
            {
                for (auto* child : mixerNode->getChildIterator())
                {
                    if (child->hasTagName ("Strip"))
                    {
                        juce::String type = child->getStringAttribute ("type");
                        juce::String name = child->getStringAttribute ("name");

                        if (type.equalsIgnoreCase ("ambisonic"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Azimuth", 1 }, name + " Azimuth", -180.0f, 180.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Elevation", 1 }, name + " Elevation", -90.0f, 90.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                        }
                        else if (type.equalsIgnoreCase ("stereo"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                        }
                        else if (type.equalsIgnoreCase ("ms"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                        }
                        else if (type.equalsIgnoreCase ("mono"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                        }
                        else if (type.equalsIgnoreCase ("stereoreverb"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                        }
                        else if (type.equalsIgnoreCase ("reverb"))
                        {
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
                            params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
                        }

                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false));

                        // Effect Order
                        juce::StringArray orderOptions;
                        orderOptions.add ("EQ -> Comp -> Tube");
                        orderOptions.add ("EQ -> Tube -> Comp");
                        orderOptions.add ("Comp -> EQ -> Tube");
                        orderOptions.add ("Comp -> Tube -> EQ");
                        orderOptions.add ("Tube -> EQ -> Comp");
                        orderOptions.add ("Tube -> Comp -> EQ");
                        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { name + "_Order", 1 }, name + " Order", orderOptions, 0));

                        // EQ
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_EQ_On", 1 }, name + " EQ On", false));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_PostGain", 1 }, name + " EQ Post Gain", -24.0f, 24.0f, 0.0f));
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
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Comp_On", 1 }, name + " Comp On", false));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_PreGain", 1 }, name + " Comp Pre Gain", -24.0f, 24.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Attack", 1 }, name + " Comp Attack", 0.1f, 100.0f, 10.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Release", 1 }, name + " Comp Release", 10.0f, 1000.0f, 100.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Thresh", 1 }, name + " Comp Thresh", -60.0f, 0.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Ratio", 1 }, name + " Comp Ratio", 1.0f, 20.0f, 1.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Gain", 1 }, name + " Comp Gain", -24.0f, 24.0f, 0.0f));

                        // Tube
                        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Tube_On", 1 }, name + " Tube On", false));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Drive", 1 }, name + " Tube Drive", 0.0f, 40.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Bias", 1 }, name + " Tube Bias", 0.0f, 0.5f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Out", 1 }, name + " Tube Out", -20.0f, 20.0f, 0.0f));
                        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { name + "_Tube_Model", 1 }, name + " Tube Model", juce::StringArray { "Standard", "Dynamic" }, 0));
                    }
                }
            }
        }
    }

    // Master Parameters
    juce::String name = "Master";
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Pan", 1 }, name + " Pan", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Width", 1 }, name + " Width", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Level", 1 }, name + " Level", -60.0f, 6.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Mute", 1 }, name + " Mute", false));
    // params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Solo", 1 }, name + " Solo", false)); // Master solo not really needed

    juce::StringArray orderOptions;
    orderOptions.add ("EQ -> Comp -> Tube");
    orderOptions.add ("EQ -> Tube -> Comp");
    orderOptions.add ("Comp -> EQ -> Tube");
    orderOptions.add ("Comp -> Tube -> EQ");
    orderOptions.add ("Tube -> EQ -> Comp");
    orderOptions.add ("Tube -> Comp -> EQ");
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { name + "_Order", 1 }, name + " Order", orderOptions, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_EQ_On", 1 }, name + " EQ On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_EQ_PostGain", 1 }, name + " EQ Post Gain", -24.0f, 24.0f, 0.0f));
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

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Comp_On", 1 }, name + " Comp On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_PreGain", 1 }, name + " Comp Pre Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Attack", 1 }, name + " Comp Attack", 0.1f, 100.0f, 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Release", 1 }, name + " Comp Release", 10.0f, 1000.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Thresh", 1 }, name + " Comp Thresh", -60.0f, 0.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Ratio", 1 }, name + " Comp Ratio", 1.0f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Comp_Gain", 1 }, name + " Comp Gain", -24.0f, 24.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { name + "_Tube_On", 1 }, name + " Tube On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Drive", 1 }, name + " Tube Drive", 0.0f, 40.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Bias", 1 }, name + " Tube Bias", 0.0f, 0.5f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { name + "_Tube_Out", 1 }, name + " Tube Out", -20.0f, 20.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { name + "_Tube_Model", 1 }, name + " Tube Model", juce::StringArray { "Standard", "Dynamic" }, 0));

    return { params.begin(), params.end() };
}

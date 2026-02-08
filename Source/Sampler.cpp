/*
  ==============================================================================

    Sampler.cpp
    Created: 30 Jan 2026 3:56:29pm
    Author:  doare

  ==============================================================================
*/

#include "Sampler.h"
#include <map>

//==============================================================================
void Sound::load (juce::AudioFormatManager& formatManager)
{
    if (data == nullptr || dataSize <= 0)
        return;

    auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) dataSize, false);
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (std::move (stream)));

    if (reader != nullptr)
    {
        audioBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&audioBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
        sourceSampleRate = reader->sampleRate;
    }
}

//==============================================================================
void Voice::start (const Sound* sound, int note, float velocity, double sampleRate)
{
    //std::cout << "Start a sound on note " << note << std::endl;
    activeSound = sound;
    currentNote = note;
    currentPosition = 0.0;
    currentVelocity = velocity;
    currentSampleRate = sampleRate;

    envelopeVal = 0.0f;
    state = State::Attack;

    if (activeSound)
    {
        double detune = 0.0;
        double attack = activeSound->attack;
        double decay = activeSound->decay;
        double sustain = activeSound->sustain;
        double release = activeSound->release;

        if (activeSound->group)
        {
            auto* g = activeSound->group;
            detune = g->detune;
            attack = g->attack;
            decay = g->decay;
            sustain = g->sustain;
            release = g->release;
        }

        double pitchRatio = std::pow (2.0, (note - activeSound->basePitch + detune) / 12.0);
        increment = (activeSound->sourceSampleRate / currentSampleRate) * pitchRatio;
        
        double attackSamples = attack * currentSampleRate;
        double decaySamples = decay * currentSampleRate;
        double releaseSamples = release * currentSampleRate;

        sustainLevel = sustain;

        attackRate = (attackSamples > 0.0) ? (1.0 / attackSamples) : 1.0;
        decayRate = (decaySamples > 0.0) ? ((1.0 - sustainLevel) / decaySamples) : 1.0;
        releaseRate = (releaseSamples > 0.0) ? (1.0 / releaseSamples) : 1.0;
    }
}

void Voice::stop()
{
    state = State::Idle;
    activeSound = nullptr;
}

void Voice::choke()
{
    if (state == State::Idle)
        return;

    state = State::Release;
    // Fast fade out (50ms) to avoid clicks
    double chokeSamples = 0.05 * currentSampleRate;
    releaseRate = (chokeSamples > 0.0) ? (1.0 / chokeSamples) : 1.0;
}

void Voice::noteOff()
{
    if (state != State::Idle && state != State::Release)
    {
        state = State::Release;
    }
}

bool Voice::isActive() const
{
    return state != State::Idle;
}

void Voice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (state == State::Idle || activeSound == nullptr)
        return;

    const auto& sourceBuffer = activeSound->audioBuffer;
    int numSourceChannels = sourceBuffer.getNumChannels();
    int numOutputChannels = outputBuffer.getNumChannels();
    const auto& targetChannels = activeSound->outputChannels;

    for (int s = 0; s < numSamples; ++s)
    {
        // Envelope processing
        if (state == State::Attack)
        {
            envelopeVal += (float) attackRate;
            if (envelopeVal >= 1.0f)
            {
                envelopeVal = 1.0f;
                state = State::Decay;
            }
        }
        else if (state == State::Decay)
        {
            envelopeVal -= (float) decayRate;
            if (envelopeVal <= (float) sustainLevel)
            {
                envelopeVal = (float) sustainLevel;
                state = State::Sustain;
            }
        }
        else if (state == State::Sustain)
        {
            if (envelopeVal <= 0.0001f) // Optimization for silence
            {
                stop();
                return;
            }
        }
        else if (state == State::Release)
        {
            envelopeVal -= (float) releaseRate;
            if (envelopeVal <= 0.0f)
            {
                envelopeVal = 0.0f;
                stop();
                return;
            }
        }

        // Sample playback
        int pos = (int) currentPosition;
        float alpha = (float) (currentPosition - pos);

        if (pos >= sourceBuffer.getNumSamples() - 1)
        {
            stop();
            return;
        }

        for (size_t i = 0; i < targetChannels.size(); i += 2)
        {
            int srcCh = targetChannels[i];
            int outCh = targetChannels[i + 1];

            if (outCh < numOutputChannels && srcCh < numSourceChannels)
            {
                float sample = sourceBuffer.getSample (srcCh, pos);
                float nextSample = sourceBuffer.getSample (srcCh, pos + 1);
                float interpolated = sample + alpha * (nextSample - sample);

                outputBuffer.addSample (outCh, startSample + s, interpolated * envelopeVal * currentVelocity);
            }
        }

        currentPosition += increment;
    }
}

//==============================================================================
Sampler::Sampler()
{
    formatManager.registerBasicFormats();
    
    for (int i = 0; i < maxVoices; ++i)
        voices.push_back (std::make_unique<Voice>());
}

Sampler::~Sampler()
{
}

void Sampler::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::ignoreUnused (samplesPerBlock);
}

void Sampler::addSound (const Sound& sound)
{
    juce::ScopedLock sl (lock);
    sounds.push_back (sound);
    // Load the audio data into the buffer for the newly added sound
    sounds.back().load (formatManager);
}

void Sampler::loadSamplesFromXml (const void* xmlData, int xmlSize)
{
    if (xmlData == nullptr || xmlSize <= 0)
        return;

    juce::XmlDocument doc (juce::String::createStringFromData (xmlData, xmlSize));
    auto root = doc.getDocumentElement();

    if (root == nullptr || ! root->hasTagName ("Mappings"))
        return;

    std::vector<Sound> newSounds;
    std::vector<std::unique_ptr<SampleGroup>> newSampleGroups;
    int newNumOutputChannels = 2;

    // Parse Master settings
    auto* master = root->getChildByName ("Master");
    if (master != nullptr)
    {
        newNumOutputChannels = master->getStringAttribute ("channels", "2").getIntValue();
    }

    // Parse SampleGroups
    std::map<juce::String, SampleGroup*> groups;
    for (auto* child : root->getChildIterator())
    {
        if (child->hasTagName ("SampleGroup"))
        {
            auto group = std::make_unique<SampleGroup>();
            group->name = child->getStringAttribute ("name");
            group->muteGroup = child->getIntAttribute ("muteGroup");
            group->isOneShot = child->getBoolAttribute ("oneShot", true);
            group->attack = child->getDoubleAttribute ("attack", 0.001);
            group->decay = child->getDoubleAttribute ("decay", 0.0);
            group->sustain = child->getDoubleAttribute ("sustain", 1.0);
            group->release = child->getDoubleAttribute ("release", 0.1);
            group->detune = child->getDoubleAttribute ("detune", 0.0);

            // Parse output channels for the group
            group->outputChannels.clear();
            juce::String channelList = child->getStringAttribute ("channels", "0,1");
            auto tokens = juce::StringArray::fromTokens (channelList, ", ", "");
            for (auto& t : tokens)
            {
                if (t.trim().isEmpty())
                    continue;

                if (t.contains (":"))
                {
                    auto parts = juce::StringArray::fromTokens (t, ":", "");
                    if (parts.size() == 2)
                    {
                        group->outputChannels.push_back (parts[0].getIntValue()); // Source
                        group->outputChannels.push_back (parts[1].getIntValue()); // Dest
                    }
                }
                else
                {
                    group->outputChannels.push_back (-1); // Auto Source
                    group->outputChannels.push_back (t.getIntValue()); // Dest
                }
            }

            if (group->outputChannels.empty()) { group->outputChannels = { -1, 0, -1, 1 }; }
            groups[group->name] = group.get();
            newSampleGroups.push_back(std::move(group));
        }
    }

    for (auto* child : root->getChildIterator())
    {
        if (child->hasTagName ("Sound"))
        {
            Sound sound;
            sound.name = child->getStringAttribute ("name");
            
            juce::String resourceName = child->getStringAttribute ("resource").replaceCharacter ('.', '_').replaceCharacter (' ', '_');
            sound.data = BinaryData::getNamedResource (resourceName.toRawUTF8(), sound.dataSize);

            if (sound.data == nullptr)
            {
                for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
                {
                    if (resourceName.equalsIgnoreCase (BinaryData::namedResourceList[i]))
                    {
                        sound.data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], sound.dataSize);
                        break;
                    }
                }
            }

            sound.midiNoteRange = juce::Range<int> (child->getIntAttribute ("noteLow"), child->getIntAttribute ("noteHigh") + 1);
            sound.velocityRange = juce::Range<int> (child->getIntAttribute ("velLow"), child->getIntAttribute ("velHigh") + 1);
            sound.basePitch = child->getIntAttribute ("basePitch");
            
            // Apply Group settings
            juce::String groupName = child->getStringAttribute ("group");
            if (groups.count (groupName))
            {
                const auto* g = groups[groupName];
                sound.muteGroup = g->muteGroup;
                sound.isOneShot = g->isOneShot;
                sound.attack = g->attack;
                sound.decay = g->decay;
                sound.sustain = g->sustain;
                sound.release = g->release;
                sound.outputChannels = g->outputChannels;
                sound.group = groups[groupName];
            }
            else
            {
                // Fallback defaults if no group specified
                sound.outputChannels = { -1, 0, -1, 1 };
            }

            sound.load (formatManager);

            // Resolve auto-source channels (-1)
            int numSourceChannels = sound.audioBuffer.getNumChannels();
            for (size_t i = 0; i < sound.outputChannels.size(); i += 2)
            {
                if (sound.outputChannels[i] == -1)
                {
                    int pairIndex = (int)(i / 2);
                    sound.outputChannels[i] = (numSourceChannels > 1) ? pairIndex : 0;
                }
            }

            newSounds.push_back (sound);
        }
    }

    {
        juce::ScopedLock sl (lock);
        for (auto& v : voices) v->stop();
        
        sounds = std::move(newSounds);
        sampleGroups = std::move(newSampleGroups);
        numOutputChannels = newNumOutputChannels;
    }
}

void Sampler::assignParameters (juce::AudioProcessorValueTreeState& apvts)
{
    juce::ScopedLock sl (lock);
    for (auto& group : sampleGroups)
    {
        juce::String prefix = group->getName() + "_";
        group->oneShotParam = apvts.getRawParameterValue (prefix + "OneShot");
        group->attackParam = apvts.getRawParameterValue (prefix + "Attack");
        group->decayParam = apvts.getRawParameterValue (prefix + "Decay");
        group->sustainParam = apvts.getRawParameterValue (prefix + "Sustain");
        group->releaseParam = apvts.getRawParameterValue (prefix + "Release");
        group->detuneParam = apvts.getRawParameterValue (prefix + "Detune");
    }
}

void Sampler::updateParams()
{
    juce::ScopedLock sl (lock);
    for (auto& group : sampleGroups)
    {
        if (group->oneShotParam) group->isOneShot = *group->oneShotParam > 0.5f;
        if (group->attackParam) group->attack = *group->attackParam;
        if (group->decayParam) group->decay = *group->decayParam;
        if (group->sustainParam) group->sustain = *group->sustainParam;
        if (group->releaseParam) group->release = *group->releaseParam;
        if (group->detuneParam) group->detune = *group->detuneParam;
    }
}

Voice* Sampler::findFreeVoice()
{
    for (auto& v : voices)
    {
        if (!v->isActive())
            return v.get();
    }
    // If all voices are busy, steal the first one (simplest voice stealing)
    return voices[0].get();
}

void Sampler::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedLock sl (lock);
    // Process MIDI events to trigger sounds
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();
            int velocity = message.getVelocity();
            float velocityFloat = message.getFloatVelocity();

            for (const auto& sound : sounds)
            {
                if (sound.midiNoteRange.contains (note) && sound.velocityRange.contains (velocity))
                {
                    // Handle Mute Groups
                    if (sound.muteGroup > 0) // Choke group 0 corresponds to no mute behaviour
                    {
                        for (auto& v : voices)
                        {
                            if (v->isActive() && v->getSound() && v->getSound()->muteGroup == sound.muteGroup)
                                v->choke();
                        }
                    }

                    if (auto* voice = findFreeVoice())
                    {
                        voice->start (&sound, note, velocityFloat, currentSampleRate);
                    }
                }
            }
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            for (auto& voice : voices)
            {
                if (voice->isActive() && voice->getSound())
                {
                    bool isOneShot = voice->getSound()->isOneShot;
                    if (voice->getSound()->group) isOneShot = voice->getSound()->group->isOneShot;
                    
                    if (!isOneShot && voice->getNote() == note)
                    {
                        voice->noteOff();
                    }
                }
            }
        }
    }

    // Render active voices
    for (auto& voice : voices)
    {
        if (voice->isActive())
        {
            voice->renderNextBlock (buffer, 0, buffer.getNumSamples());
        }
    }
}

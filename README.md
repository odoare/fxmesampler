# FxmeSampler

FxmeSampler is a JUCE-based sampling instrument plugin featuring a flexible mixer, built-in effects (Dynamics, EQ, Reverb, Delay, Tube Saturation), and XML-based mapping configuration.

## Overview

The instrument's architecture is defined by a `mapping.xml` file embedded in the plugin's binary resources. This file controls:
*   **Sample Mapping:** Which samples play on which notes/velocities.
*   **Voice Architecture:** Envelopes (ADSR), looping, and playback modes (One-shot).
*   **Routing:** How samples are routed to mixer channels.
*   **Mixer Layout:** Definition of strips, buses, and effect chains.
*   **UI Customization:** Colors, icons, and welcome screen.

## Building

This project is built using the JUCE framework.

1.  Open the `.jucer` file in the Projucer.
2.  Ensure all audio samples and images referenced in `mapping.xml` are added to the **Binary Data** section in Projucer.
3.  Save the project to generate build files for your IDE (Xcode, Visual Studio, Makefile, etc.).
4.  Build the `FxmeSampler` target.

## Configuration: `mapping.xml`

The `mapping.xml` file is the core configuration file. Below is the complete specification of its structure and attributes.

### Root Element
The root element must be `<Mappings>`.

```xml
<Mappings>
    <!-- Child elements go here -->
</Mappings>
```

### 1. Welcome Tab
Optional. Defines the content for the "Welcome" tab in the UI.

```xml
<WelcomeTab text="My Instrument" img="logo.png" />
```

| Attribute | Type | Description |
| :--- | :--- | :--- |
| `text` | String | The text displayed on the welcome screen. |
| `img` | String | The filename of the image resource to display. |

### 2. Master Settings
Optional. Configures the master output strip.

```xml
<Master channels="10" img="master_icon.png" color="55,169,181"/>
```

| Attribute | Type | Description |
| :--- | :--- | :--- |
| `channels` | Integer | Total number of internal channels feeding the mixer strips. This must correspond to the sum of channels required by all defined strips (e.g., 4 for Ambisonic, 2 for Stereo). |
| `img` | String | Icon resource for the master strip. |
| `color` | String | Color for the strip (format: "r,g,b" or color name). |

### 3. Mixer Configuration
The `<Mixer>` element contains definitions for channel strips and buses.

#### `<Strip>`
Defines a mixer channel strip.

```xml
<Strip type="stereo" name="Snare" img="snare.jpg" color="100,100,100" effectChain="dynamics"/>
```

| Attribute | Type | Description |
| :--- | :--- | :--- |
| `type` | String | Strip type: `mono`, `stereo`, `ms` (Mid-Side), `ambisonic` (4-ch), `reverb` (mono), `stereoreverb`. |
| `name` | String | Display name of the strip. |
| `img` | String | Icon resource name. |
| `color` | String | Strip color. |
| `effectChain` | String | Effect chain type. Default is "dynamics". |
| `resource` | String | (For reverb strips) Comma-separated list of Impulse Response (IR) filenames. |

#### `<Bus>`
Defines an auxiliary bus (always stereo).

```xml
<Bus name="RoomReverb" effectChain="Reverb" resource="room_ir.wav" color="purple"/>
```

| Attribute | Type | Description |
| :--- | :--- | :--- |
| `name` | String | Name of the bus. |
| `effectChain` | String | `Dynamics`, `Reverb`, `Delay`, or `None`. |
| `resource` | String | (For Reverb buses) Comma-separated list of IR filenames. |
| `img` | String | Icon resource name. |
| `color` | String | Bus color. |

### 4. Sample Groups
`<SampleGroup>` elements define shared properties for a set of sounds, such as envelopes and routing.

```xml
<SampleGroup name="Snare" channels="0:6, 1:7" muteGroup="1" midiChannel="10"
             oneShot="false" loop="true" 
             attack="0.001" decay="0.2" sustain="0.5" release="0.3" detune="0.0"/>
```

| Attribute | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `name` | String | Required | Unique identifier for the group. |
| `channels` | String | "0,1" | Output routing. Format: `src:dest` or `dest`. <br>Example: `0:6, 1:7` maps source ch 0 to output 6, source ch 1 to output 7. |
| `muteGroup` | Integer | 0 | Sounds in the same non-zero mute group cut each other off (e.g., Open/Closed Hi-Hat). |
| `midiChannel`| String | "0" | MIDI channel (1-16) or "omni" (0). |
| `oneShot` | Boolean | true | If `true`, plays full sample ignoring note-off. If `false`, enters release phase on note-off. |
| `loop` | Boolean | false | If `true`, loops the sample between `loopStart` and `loopEnd`. |
| `attack` | Float | 0.001 | Attack time in seconds. |
| `decay` | Float | 0.0 | Decay time in seconds. |
| `sustain` | Float | 1.0 | Sustain level (0.0 to 1.0). |
| `release` | Float | 0.1 | Release time in seconds. |
| `detune` | Float | 0.0 | Pitch offset in semitones. |

### 5. Sounds
`<Sound>` elements define individual samples.

```xml
<Sound name="Snare_Hit" group="Snare" resource="snare.wav" 
       basePitch="60" noteLow="60" noteHigh="60" velLow="0" velHigh="127"
       sampleStart="0" sampleEnd="-1" loopStart="500" loopEnd="2000"/>
```

| Attribute | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `name` | String | Required | Name of the sound. |
| `group` | String | - | Name of the parent `<SampleGroup>`. Inherits properties from the group. |
| `resource` | String | Required | Filename of the audio sample in Binary Data. |
| `basePitch` | Integer | 60 | MIDI note number where sample plays at original pitch. |
| `noteLow` | Integer | - | Lowest MIDI note that triggers this sound. |
| `noteHigh` | Integer | - | Highest MIDI note that triggers this sound. |
| `velLow` | Integer | 0 | Lowest velocity. |
| `velHigh` | Integer | 127 | Highest velocity. |
| `sampleStart`| Integer | 0 | Sample index to start playback. |
| `sampleEnd` | Integer | -1 | Sample index to stop playback (-1 = end of file). |
| `loopStart` | Integer | 0 | Sample index for loop start point. |
| `loopEnd` | Integer | -1 | Sample index for loop end point (-1 = sampleEnd). |

## Resource Handling

The plugin loads files from JUCE's `BinaryData`.
1.  **Filenames:** In `mapping.xml`, refer to files by their original filename (e.g., `my sample.wav`).
2.  **Internal Mapping:** The code automatically converts filenames to valid C++ variable names (replacing spaces and dots with underscores) to locate them in `BinaryData`.

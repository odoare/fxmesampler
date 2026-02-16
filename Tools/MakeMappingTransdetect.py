import os
import glob
import sys
import wave
import struct
import math

try:
    import numpy as np
except ImportError:
    print("Error: numpy is required for this script. Please install it via 'pip install numpy'.")
    sys.exit(1)

# Try to import scipy for better wav support
try:
    from scipy.io import wavfile as scipy_wav
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

def read_wav_file(filepath):
    """
    Reads a wav file and returns the audio data as a normalized float32 numpy array,
    along with the number of channels and sample rate.
    """
    # Method 1: Try scipy if available (handles float and 24-bit better)
    if HAS_SCIPY:
        try:
            framerate, data = scipy_wav.read(filepath)
            n_channels = 1 if len(data.shape) == 1 else data.shape[1]
            
            # Normalize based on dtype
            if data.dtype == np.int16:
                audio_data = data.astype(np.float32) / 32768.0
            elif data.dtype == np.int32:
                audio_data = data.astype(np.float32) / 2147483648.0
            elif data.dtype == np.uint8:
                audio_data = (data.astype(np.float32) - 128.0) / 128.0
            elif data.dtype == np.float32 or data.dtype == np.float64:
                audio_data = data.astype(np.float32)
            else:
                # Fallback for other types (e.g. 24-bit read as int32 by some scipy versions)
                # If scipy reads 24-bit, it usually scales it or returns int32.
                # We assume int32 normalization if it's integer.
                if np.issubdtype(data.dtype, np.integer):
                     # Check max value to guess bit depth if needed, but safe to assume int32 container
                     audio_data = data.astype(np.float32) / 2147483648.0
                else:
                     audio_data = data.astype(np.float32)
            
            return audio_data, n_channels, framerate
        except Exception:
            pass # Fallback to standard wave module if scipy fails

    # Method 2: Standard wave module
    try:
        with wave.open(filepath, 'rb') as wf:
            n_channels = wf.getnchannels()
            sampwidth = wf.getsampwidth()
            framerate = wf.getframerate()
            n_frames = wf.getnframes()
            try:
                comptype = wf.getcomptype()
            except AttributeError:
                comptype = 'NONE'
            
            # Read all frames
            raw_data = wf.readframes(n_frames)
            
            # Determine dtype based on sample width
            audio_data = None
            max_val = 1.0
            
            if sampwidth == 2:
                audio_data = np.frombuffer(raw_data, dtype=np.int16).astype(np.float32)
                max_val = 32768.0
            elif sampwidth == 1:
                # 8-bit WAV is unsigned
                audio_data = np.frombuffer(raw_data, dtype=np.uint8).astype(np.float32)
                audio_data = audio_data - 128.0
                max_val = 128.0
            elif sampwidth == 3:
                # 24-bit: Read as uint8, reshape to triplets, pad to 4 bytes, view as int32
                a = np.frombuffer(raw_data, dtype=np.uint8)
                a = a.reshape(-1, 3)
                b = np.zeros((a.shape[0], 4), dtype=np.uint8)
                # Copy 3 bytes to MSB positions (effectively shifting left by 8 bits)
                # Little endian file: [L, M, H] -> Memory: [0, L, M, H] (Little Endian int32)
                # Wait, standard WAV is little endian. 
                # 24-bit sample: byte0, byte1, byte2.
                # We want to form an int32. 
                # If we put byte0 at pos 1, byte1 at pos 2, byte2 at pos 3, and 0 at pos 0:
                # The int32 value is (byte2<<24) | (byte1<<16) | (byte0<<8) | 0.
                # This effectively multiplies the 24-bit value by 256.
                b[:, 1:] = a
                audio_data = b.view(np.int32).astype(np.float32)
                max_val = 2147483648.0 # 2^31 (since we shifted 24-bit to 32-bit)
            elif sampwidth == 4:
                if comptype == 'IEEE_FLOAT':
                    audio_data = np.frombuffer(raw_data, dtype=np.float32)
                    max_val = 1.0
                else:
                    audio_data = np.frombuffer(raw_data, dtype=np.int32).astype(np.float32)
                    max_val = 2147483648.0
            else:
                print(f"Warning: Unsupported sample width {sampwidth} in {filepath}")
                return None, None, None

            # Reshape if stereo/multichannel
            if n_channels > 1:
                audio_data = audio_data.reshape(-1, n_channels)
            
            # Normalize to float -1.0 to 1.0
            if max_val != 1.0:
                audio_data = audio_data / max_val
            
            return audio_data, n_channels, framerate
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None, None, None

def detect_transients(audio_data, sample_rate, threshold_db=-40, min_silence_ms=50):
    """
    Detects transients in the audio data.
    Returns a list of dictionaries containing start, end, and energy of each hit.
    """
    # Convert to mono energy envelope for detection
    if len(audio_data.shape) > 1:
        mono = np.mean(audio_data, axis=1)
    else:
        mono = audio_data
        
    # Rectify
    abs_signal = np.abs(mono)
    
    # Calculate linear threshold
    threshold_linear = 10 ** (threshold_db / 20.0)
    
    # Boolean array where signal is above threshold
    is_above = abs_signal > threshold_linear
    
    # Find state changes (rising and falling edges)
    # Pad with False to detect edges at the very beginning or end
    padded = np.concatenate(([False], is_above, [False]))
    diff = np.diff(padded.astype(int))
    
    starts = np.where(diff == 1)[0]
    ends = np.where(diff == -1)[0]
    
    if len(starts) == 0:
        return []
        
    # Merge regions separated by less than min_silence_ms
    min_silence_samples = int(min_silence_ms * sample_rate / 1000.0)
    
    merged_starts = [starts[0]]
    merged_ends = []
    
    for i in range(len(starts) - 1):
        # Check gap between current end and next start
        gap = starts[i+1] - ends[i]
        if gap > min_silence_samples:
            # Gap is large enough, close current hit and start new one
            merged_ends.append(ends[i])
            merged_starts.append(starts[i+1])
        # else: ignore this end and next start, effectively merging them
        
    merged_ends.append(ends[-1])
    
    hits = []
    for s, e in zip(merged_starts, merged_ends):
        # Calculate energy (sum of squared samples) for the region
        segment = mono[s:e]
        energy = np.sum(segment**2)
        
        hits.append({
            'start': int(s),
            'end': int(e),
            'energy': float(energy)
        })
        
    return hits

def generate_mappings(folder_path):
    wav_files = glob.glob(os.path.join(folder_path, "*.wav"))
    
    if not wav_files:
        print(f"No .wav files found in {folder_path}")
        return

    # Data structure: groups[group_name] = { 'channels': n, 'mute_group': m, 'families': [ {family_info}, ... ] }
    groups = {}
    
    print(f"Scanning {len(wav_files)} files...")
    
    for file_path in wav_files:
        filename = os.path.basename(file_path)
        name_no_ext = os.path.splitext(filename)[0]
        parts = name_no_ext.split(' ')
        
        # Naming convention: SampleGroupName SampleFamillyName MuteGroup BasePitch NoteLow NoteHigh.wav
        if len(parts) < 6:
            print(f"Skipping {filename}: Does not match naming convention (parts < 6).")
            continue
            
        try:
            note_high = int(parts[-1])
            note_low = int(parts[-2])
            base_pitch = int(parts[-3])
            mute_group = int(parts[-4])
            
            family_name = parts[-5]
            group_name = "_".join(parts[:-5]) # Join remaining parts as group name
            
            # Analyze audio
            audio_data, n_channels, sample_rate = read_wav_file(file_path)
            if audio_data is None:
                continue
                
            # Initialize group if new
            if group_name not in groups:
                groups[group_name] = {
                    'channels': n_channels,
                    'mute_group': mute_group,
                    'families': []
                }
            
            # Verify channel consistency within group
            if groups[group_name]['channels'] != n_channels:
                print(f"Warning: {filename} has {n_channels} channels, but group {group_name} expects {groups[group_name]['channels']}. Skipping.")
                continue
            
            # Detect hits
            hits = detect_transients(audio_data, sample_rate)
            if not hits:
                print(f"Warning: No transients detected in {filename}.")
                continue
                
            # Sort hits by energy (ascending)
            hits.sort(key=lambda x: x['energy'])
            
            groups[group_name]['families'].append({
                'name': family_name,
                'file': filename,
                'base': base_pitch,
                'low': note_low,
                'high': note_high,
                'hits': hits
            })
            
        except ValueError:
             print(f"Skipping {filename}: Could not parse numeric fields.")
             continue

    if not groups:
        print("No valid groups found.")
        return

    # Generate XML
    xml = []
    xml.append('<?xml version="1.0" encoding="UTF-8"?>')
    xml.append('<Mappings>')
    
    # Calculate total channels for Master
    total_channels = sum(g['channels'] for g in groups.values())
    xml.append(f'  <Master channels="{total_channels}" img="logo.png" color="55,169,181"/>')
    xml.append('')
    
    # Mixer Configuration
    xml.append('  <Mixer>')
    sorted_group_names = sorted(groups.keys())
    for gn in sorted_group_names:
        ctype = "stereo" if groups[gn]['channels'] == 2 else "mono"
        xml.append(f'    <Strip type="{ctype}" name="{gn}" img="{gn}.jpg" color="100,100,100"/>')
    xml.append('  </Mixer>')
    xml.append('')
    
    # Sample Groups and Sounds
    current_channel_offset = 0
    
    for gn in sorted_group_names:
        group = groups[gn]
        n_ch = group['channels']
        
        # Build channel string (e.g., "0:0, 1:1" for stereo starting at offset 0)
        ch_mapping = [f"{i}:{current_channel_offset + i}" for i in range(n_ch)]
        ch_str = ", ".join(ch_mapping)
        
        xml.append(f'  <SampleGroup name="{gn}" channels="{ch_str}" muteGroup="{group["mute_group"]}" oneShot="true" attack="0.001" decay="0.0" sustain="1.0" release="0.1"/>')
        
        for fam in group['families']:
            hits = fam['hits']
            count = len(hits)
            if count == 0: continue
            
            # Distribute velocities across 0-127
            step = 128 / count
            
            for i, hit in enumerate(hits):
                vel_low = int(i * step)
                vel_high = int((i + 1) * step - 1)
                if i == count - 1:
                    vel_high = 127
                
                xml.append(f'  <Sound name="{fam["name"]}" group="{gn}" resource="{fam["file"]}" basePitch="{fam["base"]}" noteLow="{fam["low"]}" noteHigh="{fam["high"]}" velLow="{vel_low}" velHigh="{vel_high}" sampleStart="{hit["start"]}" sampleEnd="{hit["end"]}"/>')
        
        xml.append('')
        current_channel_offset += n_ch
        
    xml.append('</Mappings>')
    
    output_file = os.path.join(folder_path, "mappings.xml")
    try:
        with open(output_file, "w") as f:
            f.write("\n".join(xml))
        print(f"Successfully generated: {output_file}")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    folder = "."
    if len(sys.argv) > 1:
        folder = sys.argv[1]
    generate_mappings(folder)
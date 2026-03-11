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

try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# Try to import scipy for better wav support
try:
    from scipy.io import wavfile as scipy_wav
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

try:
    from atoms_transient_detector import TransientDetector
    import scipy.signal
    HAS_ATOMS = True
except ImportError:
    print("Warning: atoms_transient_detector.py not found or scipy.signal is missing.")
    print("Falling back to simple RMS-based transient detector.")
    HAS_ATOMS = False

HAS_ATOMS = False

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

def detect_transients_atoms(audio_data, sample_rate):
    """
    Detects transients using the TransientDetector class from atoms_transient_detector.
    """
    if len(audio_data.shape) > 1:
        # Use only the first channel for transient detection
        mono_audio = audio_data[:, 0]
    else:
        mono_audio = audio_data

    td = TransientDetector(mono_audio, fs=sample_rate)
    td.master_algorithm()
    support_signal = td.y

    if len(support_signal) == 0:
        return []

    max_peak = np.max(np.abs(support_signal))
    if max_peak == 0:
        return []

    # Find peaks in the support signal.
    # Height threshold is 5% of the max peak.
    # Distance is 20ms to avoid multiple detections for one event.
    peaks, _ = scipy.signal.find_peaks(np.abs(support_signal),
                                       height=max_peak * 0.05,
                                       distance=int(sample_rate * 0.02))

    if len(peaks) == 0:
        return []

    # Sort peaks by time, just in case find_peaks doesn't guarantee it.
    peaks.sort()

    hits = []
    for i in range(len(peaks)):
        start = peaks[i]
        end = peaks[i+1] if i < len(peaks) - 1 else len(mono_audio)

        # Calculate energy for the segment
        segment = mono_audio[start:end]
        energy = float(np.sum(segment**2))

        hits.append({'start': int(start), 'end': int(end), 'energy': energy})

    return hits

def detect_transients(audio_data, sample_rate, threshold_db=-60, release_threshold_db=-100, min_silence_ms=700):
    """
    Detects transients in the audio data.
    Returns a list of dictionaries containing start, end, and energy of each hit.
    """
    # Convert to mono energy envelope for detection
    if len(audio_data.shape) > 1:
        # mono = np.mean(audio_data, axis=1)
        mono = audio_data[:,0]
    else:
        mono = audio_data
        
    # Calculate RMS envelope for detection
    window_size = 256
    squared = mono ** 2
    window = np.ones(window_size) / window_size
    mean_sq = np.convolve(squared, window, mode='same')
    rms_envelope = np.sqrt(mean_sq)
    
    # Calculate linear threshold
    threshold_linear = 10 ** (threshold_db / 20.0)
    
    # Boolean array where signal is above threshold
    is_above = rms_envelope > threshold_linear
    
    # Find state changes (rising and falling edges)
    # Pad with False to detect edges at the very beginning or end
    padded = np.concatenate(([False], is_above, [False]))
    diff = np.diff(padded.astype(int))
    
    starts = np.where(diff == 1)[0]
    
    if len(starts) == 0:
        return []
        
    # Filter starts based on min_silence_ms
    min_silence_samples = int(min_silence_ms * sample_rate / 1000.0)
    valid_starts = []
    
    for s in starts:
        # Check distance from last valid start
        if len(valid_starts) > 0 and s - valid_starts[-1] < min_silence_samples:
            continue
            
        # Check energy in the preceding window (must be silence)
        win_start = max(0, s - min_silence_samples)
        if s > win_start:
            prev_window = mono[win_start:s]
            rms = np.sqrt(np.mean(prev_window**2))
            if rms < threshold_linear:
                valid_starts.append(s)
        else:
            valid_starts.append(s)
    
    # Adjust starts back by 1ms
    offset_samples = int(1. * sample_rate / 1000.0)
    adjusted_starts = [max(0, s - offset_samples) for s in valid_starts]
    
    hits = []
    for i in range(len(adjusted_starts)):
        start = adjusted_starts[i]
        if i < len(adjusted_starts) - 1:
            end = adjusted_starts[i+1]
        else:
            end = len(audio_data)
            
        # Calculate energy (sum of squared samples) for the region
        segment = mono[start:end]
        energy = float(np.sum(segment**2))
        
        hits.append({'start': start, 'end': end, 'energy': energy})
        
    return hits

def find_best_transients(audio_data, sample_rate, expected_count):
    # Try varying thresholds with default silence first
    # Scan from -80dB to -6dB
    thresholds = range(-40, -5, 1)
    
    # Silence options to try. 
    # We prioritize 500ms (default), then try others.
    silence_options = [1000, 500, 200, 50, 20, 10, 5, 100]
    silence_options = [1000, 700, 500, 200]

    for silence in silence_options:
        valid_hits = []
        for th in thresholds:
            hits = detect_transients(audio_data, sample_rate, threshold_db=th, min_silence_ms=silence)
            if len(hits) == expected_count:
                valid_hits.append(hits)
            elif len(hits) < expected_count and len(valid_hits) > 0:
                break
        
        if valid_hits:
            # Return the one from the middle of the valid range (most stable)
            return valid_hits[len(valid_hits) // 2]
            
    return None

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
        
        # Naming convention: NumberOfSamples SampleGroupName SampleFamillyName MuteGroup BasePitch NoteLow NoteHigh.wav
        if len(parts) < 7:
            print(f"Skipping {filename}: Does not match naming convention (parts < 7).")
            continue
            
        try:
            expected_count = int(parts[0])
            note_high = int(parts[-1])
            note_low = int(parts[-2])
            base_pitch = int(parts[-3])
            mute_group = int(parts[-4])
            
            family_name = parts[-5]
            group_name = "_".join(parts[1:-5]) # Join remaining parts as group name
            
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
            if expected_count == 1:
                if len(audio_data.shape) > 1:
                    mono = np.mean(audio_data, axis=1)
                else:
                    mono = audio_data
                energy = np.sum(mono**2)
                hits = [{
                    'start': 0,
                    'end': len(audio_data),
                    'energy': float(energy)
                }]
            else:
                if HAS_ATOMS:
                    hits = detect_transients_atoms(audio_data, sample_rate)
                else: # Fallback to old method
                    hits = find_best_transients(audio_data, sample_rate, expected_count)
                    if hits is None:
                         hits = detect_transients(audio_data, sample_rate)

                if len(hits) != expected_count and expected_count > 0:
                     print(f"Warning: {filename}: Expected {expected_count} hits, found {len(hits)}.")
                     if len(hits) > expected_count:
                         print(f"Info: Using the {expected_count} most energetic hits.")
                         hits.sort(key=lambda x: x['energy'], reverse=True)
                         hits = hits[:expected_count]
                         hits.sort(key=lambda x: x['start'])

                if not hits:
                    print(f"Warning: No transients detected in {filename}.")
                    continue
                
                # Per user request, ensure the first transient starts at sample 0
                if hits:
                    hits[0]['start'] = 0
            
            if HAS_MATPLOTLIB:
                plt.figure(figsize=(12, 6))
                if len(audio_data.shape) > 1:
                    plot_data = np.mean(audio_data, axis=1)
                else:
                    plot_data = audio_data
                
                plt.plot(plot_data, color='lightgray', label='Waveform')
                
                for i, hit in enumerate(hits):
                    plt.axvline(x=hit['start'], color='r', linestyle='-', alpha=0.8, label='Start' if i==0 else "")
                    plt.axvline(x=hit['end'], color='b', linestyle='--', alpha=0.6, label='End' if i==0 else "")
                
                plt.title(f"File: {filename} | Detected: {len(hits)} | Expected: {expected_count}")
                plt.legend()
                print(f"Displaying {filename}. Close plot window to continue...")
                plt.show()
                
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
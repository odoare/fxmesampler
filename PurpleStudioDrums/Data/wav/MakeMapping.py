import os
import glob
import sys

def generate_mappings(folder_path):
    # Find all wav files in the folder
    wav_files = glob.glob(os.path.join(folder_path, "*.wav"))
    
    if not wav_files:
        print(f"No .wav files found in {folder_path}")
        return

    # Data structure: groups[group_name][family_name] = list of sample_info
    groups = {}
    
    print(f"Scanning {len(wav_files)} files...")

    for file_path in wav_files:
        filename = os.path.basename(file_path)
        name_no_ext = os.path.splitext(filename)[0]
        
        # Split by spaces (assuming the convention uses spaces as separators)
        # If your files use underscores, you might want to change this to name_no_ext.split('_')
        parts = name_no_ext.split(' ')
        
        # Fallback: if splitting by space didn't yield enough parts, try underscore
        if len(parts) < 7:
            parts = name_no_ext.split('_')

        # Check if we have enough parts for the convention:
        # SampleGroupName SampleFamillyName Number MuteGroup BasePitch NoteLow NoteHigh
        if len(parts) >= 7:
            try:
                # Parse the numeric fields from the end
                note_high = int(parts[-1])
                note_low = int(parts[-2])
                base_pitch = int(parts[-3])
                mute_group = int(parts[-4])
                number = int(parts[-5])
                
                # The family name is the one before the number
                family_name = parts[-6]
                
                # The group name is everything before the family name
                # We join with underscores to ensure a valid XML ID
                group_name = "_".join(parts[:-6])
                
                if group_name not in groups:
                    groups[group_name] = {}
                if family_name not in groups[group_name]:
                    groups[group_name][family_name] = []
                    
                groups[group_name][family_name].append({
                    'number': number,
                    'mute': mute_group,
                    'base': base_pitch,
                    'low': note_low,
                    'high': note_high,
                    'file': filename
                })
            except ValueError:
                print(f"Skipping {filename}: Could not parse numeric fields.")
        else:
            print(f"Skipping {filename}: Does not match naming convention.")

    if not groups:
        print("No valid samples found matching the convention.")
        return

    # Start generating XML content
    xml = []
    xml.append('<?xml version="1.0" encoding="UTF-8"?>')
    xml.append('<Mappings>')
    
    # 1. Master Configuration
    # We allocate 2 output channels (stereo) per SampleGroup
    total_channels = len(groups) * 2
    xml.append(f'  <Master channels="{total_channels}" img="logo.png" color="55,169,181"/>')
    xml.append('')

    # 2. Mixer Configuration
    xml.append('  <Mixer>')
    sorted_groups = sorted(groups.keys())
    
    for group_name in sorted_groups:
        # One stereo strip per SampleGroup
        xml.append(f'    <Group type="stereo" name="{group_name}" img="{group_name}.jpg" color="100,100,100"/>')
    xml.append('  </Mixer>')
    xml.append('')
    
    # 3. SampleGroups and Sounds
    channel_offset = 0
    
    for group_name in sorted_groups:
        families = groups[group_name]
        
        # Use settings from the first sample found for the group defaults (mute group etc)
        first_sample = list(families.values())[0][0]
        mute_grp = first_sample['mute']
        
        # Route this group to its dedicated stereo pair
        # Syntax: "0:DestL, 1:DestR" (Source 0 to DestL, Source 1 to DestR)
        ch_l = channel_offset
        ch_r = channel_offset + 1
        routing = f"0:{ch_l}, 1:{ch_r}"
        
        xml.append(f'  <SampleGroup name="{group_name}" channels="{routing}" muteGroup="{mute_grp}" oneShot="true" attack="0.001" decay="0.0" sustain="1.0" release="0.1"/>')
        
        # Process each family (velocity layers)
        for family_name in sorted(families.keys()):
            samples = families[family_name]
            
            # Sort samples by 'number' (velocity layer index)
            samples.sort(key=lambda x: x['number'])
            
            count = len(samples)
            if count == 0: continue
            
            # Calculate velocity ranges
            step = 128 / count
            
            for i, sample in enumerate(samples):
                vel_low = int(i * step)
                vel_high = int((i + 1) * step - 1)
                
                # Ensure the last sample covers up to 127
                if i == count - 1:
                    vel_high = 127
                
                sound_name = f"{family_name}_{sample['number']}"
                
                xml.append(f'  <Sound name="{sound_name}" group="{group_name}" resource="{sample["file"]}" basePitch="{sample["base"]}" noteLow="{sample["low"]}" noteHigh="{sample["high"]}" velLow="{vel_low}" velHigh="{vel_high}"/>')
        
        xml.append('')
        channel_offset += 2

    xml.append('</Mappings>')
    
    # Write to file
    output_file = os.path.join(folder_path, "mappings.xml")
    try:
        with open(output_file, "w") as f:
            f.write("\n".join(xml))
        print(f"Successfully generated: {output_file}")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    # Use current directory if no argument provided
    folder = "."
    if len(sys.argv) > 1:
        folder = sys.argv[1]
        
    generate_mappings(folder)

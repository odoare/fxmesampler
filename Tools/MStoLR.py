import sys
import numpy as np

try:
    from scipy.io import wavfile
except ImportError:
    print("Error: scipy is required for this script.")
    print("Please install it via 'pip install scipy'")
    sys.exit(1)

def ms_to_lr(ms_data):
    """
    Converts a Mid-Side stereo numpy array to Left-Right.
    Assumes channel 0 is Mid and channel 1 is Side.
    """
    mid = ms_data[:, 0].astype(np.float64)
    side = ms_data[:, 1].astype(np.float64)
    
    left = mid + side
    right = mid - side
    
    # Find the peak value to determine if normalization is needed.
    peak = np.max(np.abs(np.stack((left, right))))

    # Determine the maximum possible value for the data type.
    target_max = 1.0
    if np.issubdtype(ms_data.dtype, np.integer):
        info = np.iinfo(ms_data.dtype)
        target_max = float(info.max)
    
    # If the peak value exceeds the maximum, normalize the whole audio.
    if peak > target_max:
        print("Info: Peak value exceeds range. Normalizing audio to prevent clipping.")
        scaling_factor = target_max / peak
        left *= scaling_factor
        right *= scaling_factor

    # Stack the L and R channels back into a stereo array of the original type
    lr_data = np.stack((left, right), axis=-1).astype(ms_data.dtype)
    
    return lr_data

def main():
    if len(sys.argv) != 3:
        print("Usage: python MStoLR.py <input_ms_file.wav> <output_lr_file.wav>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    try:
        sample_rate, data = wavfile.read(input_file)
        print(f"Read '{input_file}' at {sample_rate} Hz, data type: {data.dtype}")

        if len(data.shape) != 2 or data.shape[1] != 2:
            print("Error: Input file must be a stereo WAV file.")
            sys.exit(1)

        print("Converting from Mid-Side to Left-Right...")
        lr_data = ms_to_lr(data)

        wavfile.write(output_file, sample_rate, lr_data)
        print(f"Successfully wrote Left-Right audio to '{output_file}'")

    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
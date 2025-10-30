import wave
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import resample
import os
import sys
import hashlib

"""
python script.py 128 ./my_audio_folder yes
Recursively process all .wav files under ./my_audio_folder
Save converted files in ./output/
Create a single samples.txt with all sample arrays and labels
Use yes to show graphs. Use no to skip plotting.
"""


def convert_to_signed_16bit(data, sampwidth):
    if sampwidth == 1:
        data = np.frombuffer(data, dtype=np.uint8)
        data = (data.astype(np.int16) - 128) * 256
    elif sampwidth == 2:
        data = np.frombuffer(data, dtype=np.int16)
    elif sampwidth == 4:
        data = np.frombuffer(data, dtype=np.int32)
        data = (data >> 16).astype(np.int16)
    else:
        raise ValueError("Unsupported sample width")
    return data

def safe_clip_to_int16(arr):
    clipped = np.clip(arr, -32768, 32767)
    return clipped.astype(np.int16)

def is_duplicate_fuzzy(new_waveform, existing_waveforms, threshold=0.995):
    norm_new = new_waveform / np.linalg.norm(new_waveform)
    for existing in existing_waveforms:
        norm_existing = existing / np.linalg.norm(existing)
        similarity = np.dot(norm_new, norm_existing)
        if similarity >= threshold:
            return True
    return False

def find_wav_files(root_dir):
    wav_files = []
    for dirpath, _, filenames in os.walk(root_dir):
        for f in filenames:
            if f.lower().endswith('.wav'):
                wav_files.append(os.path.join(dirpath, f))
    return wav_files

def write_c_array(output_dir, sample_matrix, label_list):
    txt_path = os.path.join(output_dir, "samples.txt")
    with open(txt_path, 'w') as f:
        f.write(f"const int16_t samples[{len(sample_matrix)}][256] = {{\n")
        for i, row in enumerate(sample_matrix):
            f.write("  { " + ", ".join(str(val) for val in row) + " }")
            f.write(",\n" if i < len(sample_matrix) - 1 else "\n")
        f.write("};\n\n")
        f.write("// Labels:\n")
        for i, label in enumerate(label_list):
            f.write(f"// [{i}] {label}\n")

def process_file(input_path, num_samples, output_dir, sample_matrix, label_list, show_graphs):
    with wave.open(input_path, 'rb') as wav_file:
        n_channels = wav_file.getnchannels()
        sampwidth = wav_file.getsampwidth()
        framerate = wav_file.getframerate()
        n_frames = wav_file.getnframes()
        raw_data = wav_file.readframes(n_frames)

    audio_data = convert_to_signed_16bit(raw_data, sampwidth)
    if n_channels > 1:
        audio_data = audio_data[::n_channels]

    extracted = audio_data[:num_samples]
    resampled = resample(extracted, 256)
    resampled = safe_clip_to_int16(resampled)

    print("Checking for duplicates...")
    if is_duplicate_fuzzy(resampled, sample_matrix, threshold=0.995):
        print(f"Near-duplicate waveform detected — skipping {input_path}")
        return

    rel_path = os.path.relpath(input_path)
    parts = os.path.splitext(rel_path)[0].split(os.sep)
    flat_name = "_".join(parts) + ".wav"
    output_path = os.path.join(output_dir, flat_name)

    with wave.open(output_path, 'wb') as out_wav:
        out_wav.setnchannels(1)
        out_wav.setsampwidth(2)
        out_wav.setframerate(256)
        out_wav.writeframes(resampled.tobytes())

    sample_matrix.append(resampled)
    label_list.append(flat_name)

    if show_graphs:
        plt.figure(figsize=(10, 4))
        plt.plot(audio_data[:num_samples], color='red', label='Original')
        plt.plot(np.linspace(0, num_samples, 256), resampled, color='green', label='Resampled')
        plt.title(f"Waveform: {flat_name}")
        plt.xlabel("Sample Index")
        plt.ylabel("Amplitude")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()

def main():
    if len(sys.argv) < 4:
        print("Usage: python script.py <num_samples> <root_directory> <show_graphs:yes|no>")
        return

    num_samples = int(sys.argv[1])
    root_dir = sys.argv[2]
    show_graphs = sys.argv[3].lower() == "yes"
    output_dir = "output"
    os.makedirs(output_dir, exist_ok=True)

    wav_files = find_wav_files(root_dir)
    sample_matrix = []
    label_list = []

    print(f"Found {len(wav_files)} WAV files. Starting processing...")

    for wav_file in wav_files:
        print(f"Processing: {wav_file}")
        process_file(wav_file, num_samples, output_dir, sample_matrix, label_list, show_graphs)

    write_c_array(output_dir, sample_matrix, label_list)
    print(f"\nAll unique files processed. Output saved in '{output_dir}'.")

if __name__ == "__main__":
    main()

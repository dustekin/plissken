"""Encode 22.05 kHz mono PCM samples as 4-bit IMA ADPCM."""
import struct
import wave


SAMPLE_RATE = 22050
STEP_TABLE = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
)
INDEX_TABLE = (-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8)


def encode_sample(path):
    with wave.open(str(path), "rb") as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (
            1,
            2,
            SAMPLE_RATE,
            "NONE",
        ):
            raise ValueError(f"{path}: expected uncompressed 22.05 kHz, mono, 16-bit PCM")
        raw = wav.readframes(wav.getnframes())
        frames = wav.getnframes()
    if frames < 2:
        raise ValueError(f"{path}: sample is empty")
    samples = struct.unpack("<" + "h" * frames, raw)
    predictor = samples[0]
    step_index = 0
    codes = []
    for target in samples[1:]:
        difference = target - predictor
        code = 0
        if difference < 0:
            code = 8
            difference = -difference
        step = STEP_TABLE[step_index]
        delta = step >> 3
        if difference >= step:
            code |= 4
            difference -= step
            delta += step
        if difference >= step >> 1:
            code |= 2
            difference -= step >> 1
            delta += step >> 1
        if difference >= step >> 2:
            code |= 1
            delta += step >> 2
        predictor += -delta if code & 8 else delta
        predictor = max(-32768, min(32767, predictor))
        step_index = max(0, min(88, step_index + INDEX_TABLE[code]))
        codes.append(code)

    encoded = bytearray(struct.pack("<h", samples[0]))
    for index in range(0, len(codes), 2):
        byte = codes[index]
        if index + 1 < len(codes):
            byte |= codes[index + 1] << 4
        encoded.append(byte)
    return encoded, frames

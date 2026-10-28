"""WARNING演出用の音声切り出しとPCM変換"""
from pathlib import Path
import json
import wave
import numpy as np

source = Path(r'C:/Users/2230190/Downloads/420661__kinoton__alarm-siren-fast-oscillations.wav')
destination = Path('Data/Audio/Scene/BossWarning.wav')
if destination.exists():
    raise FileExistsError(destination)
with wave.open(str(source), 'rb') as stream:
    rate = stream.getframerate()
    channels = stream.getnchannels()
    assert stream.getsampwidth() == 3 and stream.getcomptype() == 'NONE'
    raw = np.frombuffer(stream.readframes(stream.getnframes()), dtype=np.uint8).reshape(-1, 3)
values = raw[:, 0].astype(np.int32) | (raw[:, 1].astype(np.int32) << 8) | (raw[:, 2].astype(np.int32) << 16)
values = (values ^ 0x800000) - 0x800000
samples = values.reshape(-1, channels).astype(np.float64) / 8388608.0
# 波形の立ち上がりを10ms単位で探し、先頭に5msの余裕を残す
hop = rate // 100
levels = np.array([np.sqrt(np.mean(samples[i:i+hop] ** 2)) for i in range(0, len(samples), hop)])
active = np.flatnonzero(levels > max(0.001, levels.max() * 0.04))
start = max(0, int(active[0]) * hop - int(rate * 0.005)) if len(active) else 0
length = round(rate * 3.2)
result = samples[start:start+length].copy()
assert len(result) == length
result -= result.mean(axis=0)
attack = round(rate * 0.012)
release = round(rate * 0.18)
result[:attack] *= (0.5 - 0.5 * np.cos(np.linspace(0, np.pi, attack)))[:, None]
result[-release:] *= (0.5 + 0.5 * np.cos(np.linspace(0, np.pi, release)))[:, None]
peak = np.max(np.abs(result))
result *= 10 ** (-3 / 20) / peak
pcm = np.round(np.clip(result, -1, 1) * 32767).astype('<i2')
with wave.open(str(destination), 'wb') as stream:
    stream.setnchannels(channels)
    stream.setsampwidth(2)
    stream.setframerate(rate)
    stream.writeframes(pcm.tobytes())
with wave.open(str(destination), 'rb') as stream:
    assert stream.getnframes() == length and stream.getsampwidth() == 2
assert np.max(np.abs(pcm.astype(np.int32))) < 32767
assert np.all(pcm[0] == 0) and np.all(pcm[-1] == 0)
report = dict(source=str(source), source_url='https://freesound.org/people/Kinoton/sounds/420661/', author='Kinoton', title='Alarm Siren, Fast Oscillations', source_license_label='CC0; author also links terms of use on their profile', source_start_seconds=start/rate, duration_seconds=3.2, sample_rate=rate, channels=channels, bits=16, peak_dbfs=round(float(20*np.log10(np.max(np.abs(pcm.astype(float)))/32768)), 3), fade_in_seconds=0.012, fade_out_seconds=0.18, pitch_change=False, time_stretch=False)
Path('Data/Audio/Scene/BossWarning.processing.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False, indent=2))

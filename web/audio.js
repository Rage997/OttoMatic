// Complete sounds live in Web Audio, so rendering stalls cannot starve playback.
Module.ottoAudio = (() => {
  let context, master;
  const sources = new Map();

  function init() {
    if (!context) {
      context = new AudioContext();
      master = context.createGain();
      master.connect(context.destination);
    }
    return context.sampleRate;
  }

  function position(s) {
    let p = s.offset + (s.playing ? (context.currentTime - s.startedAt) * s.pitch : 0);
    if (s.loop && p >= s.buffer.duration) {
      p = s.loopStart + (p - s.loopStart) % (s.buffer.duration - s.loopStart);
    }
    return Math.min(p, s.buffer.duration);
  }

  function detach(s) {
    if (s.node) {
      s.node.onended = null;
      s.node.stop();
      s.node.disconnect();
      s.node = null;
    }
    s.playing = false;
  }

  function release(id) {
    const s = sources.get(id);
    if (!s) return;
    detach(s);
    s.splitter.disconnect();
    s.left.disconnect();
    s.right.disconnect();
    s.merger.disconnect();
    sources.delete(id);
  }

  function load(id, data, frames, sampleRate, bits, channels, bigEndian) {
    release(id);
    if (!frames) return;
    init();
    const buffer = context.createBuffer(2, frames, sampleRate);
    const bytes = bits / 8;
    const pcm = new DataView(HEAPU8.buffer, data, frames * channels * bytes);
    for (let channel = 0; channel < 2; channel++) {
      const out = buffer.getChannelData(channel);
      const inputChannel = channels === 1 ? 0 : channel;
      for (let frame = 0; frame < frames; frame++) {
        const offset = (frame * channels + inputChannel) * bytes;
        out[frame] = bits === 8
          ? (pcm.getUint8(offset) - 128) / 128
          : pcm.getInt16(offset, !bigEndian) / 32768;
      }
    }
    const splitter = context.createChannelSplitter(2);
    const left = context.createGain(), right = context.createGain();
    const merger = context.createChannelMerger(2);
    splitter.connect(left, 0); splitter.connect(right, 1);
    left.connect(merger, 0, 0); right.connect(merger, 0, 1);
    merger.connect(master);
    sources.set(id, {buffer, splitter, left, right, merger, node: null,
      offset: 0, startedAt: 0, pitch: 1, loop: false, loopStart: 0, playing: false});
  }

  function play(id, loop, sustain, rewind) {
    const s = sources.get(id);
    if (!s || s.playing) return;
    s.loop = !!loop;
    s.loopStart = sustain / s.buffer.sampleRate;
    if (rewind || s.offset >= s.buffer.duration) s.offset = 0;
    const node = context.createBufferSource();
    node.buffer = s.buffer;
    node.loop = s.loop;
    node.loopStart = s.loopStart;
    node.loopEnd = s.buffer.duration;
    node.playbackRate.value = s.pitch;
    node.connect(s.splitter);
    s.node = node;
    s.startedAt = context.currentTime;
    s.playing = true;
    node.onended = () => {
      // A stopped/replaced source may already have an ended event queued.
      if (sources.get(id) !== s || s.node !== node) return;
      s.offset = s.buffer.duration;
      s.playing = false;
      s.node = null;
      node.disconnect();
      Module._ottoAudioEnded(id);
    };
    node.start(s.startedAt, s.offset);
  }

  return {
    get context() { return context; },
    init, load, play, release,
    pause(id) {
      const s = sources.get(id);
      if (!s) return;
      s.offset = position(s);
      detach(s);
    },
    stop(id) {
      const s = sources.get(id);
      if (!s) return;
      detach(s);
      s.offset = 0;
    },
    rewind(id) {
      const s = sources.get(id);
      if (!s) return;
      const playing = s.playing;
      detach(s);
      s.offset = 0;
      if (playing) play(id, s.loop, s.loopStart * s.buffer.sampleRate, true);
    },
    position(id) {
      const s = sources.get(id);
      return s ? position(s) : 0;
    },
    gains(id, left, right) {
      const s = sources.get(id);
      if (!s) return;
      s.left.gain.value = left;
      s.right.gain.value = right;
    },
    pitch(id, pitch) {
      const s = sources.get(id);
      if (!s) return;
      s.offset = position(s);
      s.startedAt = context.currentTime;
      s.pitch = pitch;
      if (s.node) s.node.playbackRate.value = pitch;
    },
    loop(id, loop, sustain) {
      const s = sources.get(id);
      if (!s) return;
      s.offset = position(s);
      s.startedAt = context.currentTime;
      s.loop = !!loop;
      s.loopStart = sustain / s.buffer.sampleRate;
      if (s.node) {
        s.node.loop = s.loop;
        s.node.loopStart = s.loopStart;
      }
    },
    masterGain(gain) { if (master) master.gain.value = gain; },
    shutdown() {
      for (const id of sources.keys()) release(id);
      if (context) context.close();
      context = master = undefined;
    }
  };
})();

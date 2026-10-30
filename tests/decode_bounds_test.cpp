// Crafted packets that claim more samples than the decoder and its caller have
// room for. Run under AddressSanitizer to see the writes themselves: without it
// a small overrun can land in allocator padding and only the status check trips.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "ALACAudioTypes.h"
#include "ALACBitUtilities.h"
#include "ALACDecoder.h"

namespace {

const uint32_t kFrameLength = 16;
const uint8_t kGuardByte = 0xA5;
const size_t kGuardSize = 64;

// Zero bits are a valid entropy-coded payload, so a compressed element can consume
// this instead of a real one, and the reads stay inside the packet.
const size_t kPaddingBytes = 4096;

enum { kTagSCE = ID_SCE, kTagCPE = ID_CPE };

class PacketWriter {
public:
  PacketWriter() : bitCount_(0) {}

  void put(uint32_t value, uint32_t numBits) {
    for (uint32_t i = numBits; i-- > 0;) {
      if (bitCount_ % 8 == 0) bytes_.push_back(0);
      if ((value >> i) & 1) bytes_.back() |= (uint8_t)(0x80u >> (bitCount_ % 8));
      ++bitCount_;
    }
  }

  void elementHeader(uint32_t tag, bool escape, bool partial, uint32_t partialSamples) {
    put(tag, 3);
    put(0, 4);   // element instance tag
    put(0, 12);  // unused
    put(partial ? 1 : 0, 1);
    put(0, 2);   // bytes shifted
    put(escape ? 1 : 0, 1);
    if (partial) {
      put(partialSamples >> 16, 16);
      put(partialSamples & 0xFFFF, 16);
    }
  }

  void escapeElement(uint32_t tag, bool partial, uint32_t declaredSamples, uint32_t writtenSamples) {
    elementHeader(tag, true, partial, declaredSamples);
    const uint32_t perSample = tag == kTagCPE ? 2 : 1;
    for (uint32_t i = 0; i < writtenSamples * perSample; ++i) put(i + 1, 16);
  }

  // No predictor coefficients: unpc_block just copies the residuals.
  void compressedElement(uint32_t tag, uint32_t partialSamples) {
    elementHeader(tag, false, true, partialSamples);
    put(0, 8);  // mix bits
    put(0, 8);  // mix res
    const uint32_t channels = tag == kTagCPE ? 2 : 1;
    for (uint32_t i = 0; i < channels; ++i) {
      put(0, 8);  // mode, den shift
      put(0, 8);  // pb factor, coefficient count
    }
  }

  std::vector<uint8_t> finish() {
    std::vector<uint8_t> packet = bytes_;
    packet.resize(packet.size() + kPaddingBytes, 0);
    return packet;
  }

private:
  std::vector<uint8_t> bytes_;
  size_t bitCount_;
};

void putBE32(uint8_t* out, uint32_t value) {
  out[0] = (uint8_t)(value >> 24);
  out[1] = (uint8_t)(value >> 16);
  out[2] = (uint8_t)(value >> 8);
  out[3] = (uint8_t)value;
}

bool initDecoder(ALACDecoder& decoder, uint32_t channels) {
  uint8_t cookie[sizeof(ALACSpecificConfig)];
  memset(cookie, 0, sizeof(cookie));
  putBE32(cookie, kFrameLength);
  cookie[5] = 16;   // bit depth
  cookie[6] = 40;   // pb
  cookie[7] = 10;   // mb
  cookie[8] = 14;   // kb
  cookie[9] = (uint8_t)channels;
  cookie[11] = 255; // max run
  return decoder.Init(cookie, sizeof(cookie)) == ALAC_noErr;
}

struct Result {
  int32_t status;
  uint32_t outSamples;
  bool guardIntact;
  std::vector<int16_t> output;
};

// capacity is what the buffer holds, which is also what gets requested unless
// the case is about a caller asking for more than it should.
Result decode(std::vector<uint8_t> packet, uint32_t requested, uint32_t capacity, uint32_t channels) {
  Result result;
  ALACDecoder decoder;
  if (!initDecoder(decoder, channels)) {
    result.status = -1;
    result.outSamples = 0;
    result.guardIntact = false;
    return result;
  }

  const size_t outputBytes = capacity * channels * sizeof(int16_t);
  std::vector<uint8_t> buffer(outputBytes + kGuardSize, kGuardByte);

  BitBuffer bits;
  BitBufferInit(&bits, &packet[0], (uint32_t)packet.size());

  result.outSamples = 0;
  result.status = decoder.Decode(&bits, &buffer[0], requested, channels, &result.outSamples);

  result.guardIntact = true;
  for (size_t i = outputBytes; i < buffer.size(); ++i) {
    if (buffer[i] != kGuardByte) result.guardIntact = false;
  }
  result.output.resize(capacity * channels);
  memcpy(&result.output[0], &buffer[0], outputBytes);
  return result;
}

int failures = 0;

void report(const char* name, bool passed, const Result& result) {
  // Flushed per case because an unfixed decoder can crash on the next one.
  printf("%s  %-44s status=%d samples=%u guard=%s\n", passed ? "PASS" : "FAIL", name, (int)result.status,
         result.outSamples, result.guardIntact ? "ok" : "OVERWRITTEN");
  fflush(stdout);
  if (!passed) ++failures;
}

void expectRejected(const char* name, const std::vector<uint8_t>& packet, uint32_t requested, uint32_t capacity,
                    uint32_t channels) {
  const Result result = decode(packet, requested, capacity, channels);
  report(name, result.status == kALAC_ParamError && result.guardIntact, result);
}

void expectDecoded(const char* name, const std::vector<uint8_t>& packet, uint32_t expectedSamples, uint32_t channels,
                   bool expectRamp) {
  const Result result = decode(packet, kFrameLength, kFrameLength, channels);
  bool passed = result.status == ALAC_noErr && result.outSamples == expectedSamples && result.guardIntact;
  // Compressed packets are padding, so only the escape ones have known samples.
  for (uint32_t i = 0; passed && expectRamp && i < expectedSamples * channels; ++i) {
    if (result.output[i] != (int16_t)(i + 1)) passed = false;
  }
  report(name, passed, result);
}

std::vector<uint8_t> escapePacket(uint32_t tag, bool partial, uint32_t declared, uint32_t written) {
  PacketWriter writer;
  writer.escapeElement(tag, partial, declared, written);
  return writer.finish();
}

std::vector<uint8_t> compressedPacket(uint32_t tag, uint32_t partialSamples) {
  PacketWriter writer;
  writer.compressedElement(tag, partialSamples);
  return writer.finish();
}

}  // namespace

int main() {
  // Frames the decoder has to keep accepting.
  expectDecoded("SCE escape, full frame", escapePacket(kTagSCE, false, 0, kFrameLength), kFrameLength, 1, true);
  expectDecoded("SCE escape, partial == frameLength", escapePacket(kTagSCE, true, kFrameLength, kFrameLength),
                kFrameLength, 1, true);
  expectDecoded("SCE escape, short partial", escapePacket(kTagSCE, true, 8, 8), 8, 1, true);
  expectDecoded("CPE escape, partial == frameLength", escapePacket(kTagCPE, true, kFrameLength, kFrameLength),
                kFrameLength, 2, true);
  expectDecoded("SCE compressed, short partial", compressedPacket(kTagSCE, 8), 8, 1, false);
  expectDecoded("CPE compressed, partial == frameLength", compressedPacket(kTagCPE, kFrameLength), kFrameLength, 2,
                false);

  // Each path that writes a different buffer, one sample over. The escape paths
  // fill mMixBufferU/V, the compressed ones go through mPredictor first.
  expectRejected("SCE escape, partial > frameLength", escapePacket(kTagSCE, true, kFrameLength + 1, kFrameLength + 1),
                 kFrameLength, kFrameLength, 1);
  expectRejected("CPE escape, partial > frameLength", escapePacket(kTagCPE, true, kFrameLength + 1, kFrameLength + 1),
                 kFrameLength, kFrameLength, 2);
  expectRejected("SCE compressed, partial > frameLength", compressedPacket(kTagSCE, kFrameLength + 1), kFrameLength,
                 kFrameLength, 1);
  expectRejected("CPE compressed, partial > frameLength", compressedPacket(kTagCPE, kFrameLength + 1), kFrameLength,
                 kFrameLength, 2);

  // A check made once per packet would miss this: each element has its own header.
  {
    PacketWriter writer;
    writer.escapeElement(kTagSCE, true, 8, 8);
    writer.escapeElement(kTagSCE, true, kFrameLength + 1, kFrameLength + 1);
    expectRejected("second SCE, partial > frameLength", writer.finish(), kFrameLength, kFrameLength, 2);
  }

  // The only size the decoder knows for the output buffer is the one requested.
  expectRejected("partial within frameLength, over the request", escapePacket(kTagSCE, true, 12, 12), 8, 8, 1);

  expectRejected("requested numSamples > frameLength", escapePacket(kTagSCE, false, 0, kFrameLength + 1),
                 kFrameLength + 1, kFrameLength + 1, 1);

  // Kept last: this one takes an unfixed decoder down rather than failing politely.
  expectRejected("SCE escape, partial = 0x7FFFFFFF", escapePacket(kTagSCE, true, 0x7FFFFFFF, kFrameLength),
                 kFrameLength, kFrameLength, 1);

  printf("%d failure(s)\n", failures);
  return failures == 0 ? 0 : 1;
}

#include "NdefRecord.h"

#undef LOG_TAG
#define LOG_TAG "nfcd"
#include <utils/Log.h>

static bool ensureSanePayloadSize(long size);
static bool validateTnf(uint8_t tnf, std::vector<uint8_t>& type, std::vector<uint8_t>& id, std::vector<uint8_t>& payload);

static const uint8_t FLAG_MB = 0x80;
static const uint8_t FLAG_ME = 0x40;
static const uint8_t FLAG_CF = 0x20;
static const uint8_t FLAG_SR = 0x10;
static const uint8_t FLAG_IL = 0x08;

// 10 MB payload limit.
static const int MAX_PAYLOAD_SIZE = 10 * (1 << 20);

NdefRecord::NdefRecord(uint8_t tnf, std::vector<uint8_t>& type, std::vector<uint8_t>& id, std::vector<uint8_t>& payload)
{
  mTnf = tnf;

  for(uint32_t i = 0; i < type.size(); i++)
    mType.push_back(type[i]);
  for(uint32_t i = 0; i < id.size(); i++)
    mId.push_back(id[i]);
  for(uint32_t i = 0; i < payload.size(); i++)
    mPayload.push_back(payload[i]);
}

NdefRecord::NdefRecord(uint8_t tnf, uint32_t typeLength, uint8_t* type, uint32_t idLength, uint8_t* id, uint32_t payloadLength, uint8_t* payload)
{
  mTnf = tnf;

  for (uint32_t i = 0; i < typeLength; i++)
    mType.push_back((uint8_t)type[i]);
  for (uint32_t i = 0; i < idLength; i++)
    mId.push_back((uint8_t)id[i]);
  for (uint32_t i = 0; i < payloadLength; i++)
    mPayload.push_back((uint8_t)payload[i]);
}

NdefRecord::~NdefRecord()
{
}

bool NdefRecord::parse(std::vector<uint8_t>& buf, bool ignoreMbMe, std::vector<NdefRecord>& records)
{
  return NdefRecord::parse(buf, ignoreMbMe, records, 0);
}

bool NdefRecord::parse(std::vector<uint8_t>& buf, bool ignoreMbMe, std::vector<NdefRecord>& records, int offset)
{
  bool inChunk = false;
  uint8_t chunkTnf = -1;
  bool me = false;
  uint32_t index = offset;

  while(!me) {
    std::vector<uint8_t> type;
    std::vector<uint8_t> id;
    std::vector<uint8_t> payload;
    std::vector<std::vector<uint8_t> > chunks;

    uint8_t flag = buf[index++];

    bool mb = (flag & FLAG_MB) != 0;
    me = (flag & FLAG_ME) != 0;
    bool cf = (flag & FLAG_CF) != 0;
    bool sr = (flag & FLAG_SR) != 0;
    bool il = (flag & FLAG_IL) != 0;
    uint8_t tnf = flag & 0x07;

    if (!mb && records.size() == 0 && !inChunk && !ignoreMbMe) {
      ALOGE("expected MB flag");
      return false;
    } else if (mb && records.size() != 0 && !ignoreMbMe) {
      ALOGE("unexpected MB flag");
      return false;
    } else if (inChunk && il) {
      ALOGE("unexpected IL flag in non-leading chunk");
      return false;
    } else if (cf && me) {
      ALOGE("unexpected ME flag in non-trailing chunk");
      return false ;
    } else if (inChunk && tnf != NdefRecord::TNF_UNCHANGED) {
      ALOGE("expected TNF_UNCHANGED in non-leading chunk");
      return false ;
    } else if (!inChunk && tnf == NdefRecord::TNF_UNCHANGED) {
      ALOGE("unexpected TNF_UNCHANGED in first chunk or unchunked record");
      return false;
    }

    uint32_t typeLength = buf[index++] & 0xFF;
    uint32_t payloadLength;
    if (sr) {
      payloadLength = buf[index++] & 0xFF;
    } else {
      payloadLength = ((uint32_t)buf[index]     << 24) |
                      ((uint32_t)buf[index + 1] << 16) |
                      ((uint32_t)buf[index + 2] <<  8) |
                      ((uint32_t)buf[index + 3]);
      index += 4;
    }
    uint32_t idLength = il ? (buf[index++] & 0xFF) : 0;

    if (inChunk && typeLength != 0) {
      ALOGE("expected zero-length type in non-leading chunk");
      return false;
    }

    if (!inChunk) {
      for (uint32_t idx = 0; idx < typeLength; idx++) {
        type.push_back(buf[index++]);
      }
      for (uint32_t idx = 0; idx < idLength; idx++) {
        id.push_back(buf[index++]);
      }
    }

    if (!ensureSanePayloadSize(payloadLength)) {
      return false;
    }

    for (uint32_t idx = 0; idx < payloadLength; idx++) {
      payload.push_back(buf[index++]);
    }

    if (cf && !inChunk) {
      // first chunk.
      chunks.clear();
      chunkTnf = tnf;
    }
    if (cf || inChunk) {
      // any chunk.
      chunks.push_back(payload);
    }
    if (!cf && inChunk) {
      // last chunk, flatten the payload.
      payloadLength = 0;
      for (uint32_t idx = 0; idx < chunks.size(); idx++) {
        payloadLength += chunks[idx].size();
      }
      if (!ensureSanePayloadSize(payloadLength)) {
        return false;
      }

      for(uint32_t i = 0; i < chunks.size(); i++) {
        for(uint32_t j = 0; j < chunks[i].size(); j++) {
          payload.push_back(chunks[i][j]);
        }
      }
      tnf = chunkTnf;
    }
    if (cf) {
      // more chunks to come.
      inChunk = true;
      continue;
    } else {
      inChunk = false;
    }

    bool isValid = validateTnf(tnf, type, id, payload);
    if (isValid == false) {
      return false;
    }

    NdefRecord record(tnf, type, id, payload);
    records.push_back(record);

    if (ignoreMbMe) {  // for parsing a single NdefRecord.
      break;
    }
  }
  return true;
}

bool ensureSanePayloadSize(long size)
{
  if (size > MAX_PAYLOAD_SIZE) {
    ALOGE("payload above max limit: %d > ", MAX_PAYLOAD_SIZE);
    return false;
  }
  return true;
}

bool validateTnf(uint8_t tnf, std::vector<uint8_t>& type, std::vector<uint8_t>& id, std::vector<uint8_t>& payload)
{
  bool isValid = true;
  switch (tnf) {
    case NdefRecord::TNF_EMPTY:
      if (type.size() != 0 || id.size() != 0 || payload.size() != 0) {
        ALOGE("unexpected data in TNF_EMPTY record");
        isValid = false;
      }
      break;
    case NdefRecord::TNF_WELL_KNOWN:
    case NdefRecord::TNF_MIME_MEDIA:
    case NdefRecord::TNF_ABSOLUTE_URI:
    case NdefRecord::TNF_EXTERNAL_TYPE:
      break;
    case NdefRecord::TNF_UNKNOWN:
    case NdefRecord::TNF_RESERVED:
      if (type.size() != 0) {
        ALOGE("unexpected type field in TNF_UNKNOWN or TNF_RESERVEd record");
        isValid = false;
      }
      break;
    case NdefRecord::TNF_UNCHANGED:
      ALOGE("unexpected TNF_UNCHANGED in first chunk or logical record");
      isValid = false;
      break;
    default:
      ALOGE("unexpected tnf value");
      isValid = false;
      break;
  }
  return isValid;
}

void NdefRecord::writeToByteBuffer(std::vector<uint8_t>& buf, bool mb, bool me)
{
  bool sr = mPayload.size() < 256;
  bool il = mId.size() > 0;

  uint8_t flags = (uint8_t)((mb ? FLAG_MB : 0) |
                            (me ? FLAG_ME : 0) |
                            (sr ? FLAG_SR : 0) |
                            (il ? FLAG_IL : 0) | mTnf);
  buf.push_back(flags);

  buf.push_back((uint8_t)mType.size());
  if (sr) {
    buf.push_back((uint8_t)mPayload.size());
  } else {
    buf.push_back((mPayload.size() >> 24) & 0xff);
    buf.push_back((mPayload.size() >> 16) & 0xff);
    buf.push_back((mPayload.size() >>  8) & 0xff);
    buf.push_back(mPayload.size() & 0xff);
  }
  if (il) {
    buf.push_back((uint8_t)mId.size());
  }

  for (uint32_t i = 0; i < mType.size(); i++)
    buf.push_back(mType[i]);
  for (uint32_t i = 0; i < mId.size(); i++)
    buf.push_back(mId[i]);
  for (uint32_t i = 0; i < mPayload.size(); i++)
    buf.push_back(mPayload[i]);
}

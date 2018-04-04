// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2018
// MIT License

#pragma once

#include "../Deserialization/StringWriter.hpp"
#include "../JsonVariant.hpp"
#include "../Memory/JsonBuffer.hpp"
#include "../TypeTraits/IsConst.hpp"
#include "endianess.hpp"
#include "ieee754.hpp"

namespace ArduinoJson {
namespace Internals {

// Parse JSON string to create JsonArrays and JsonObjects
// This internal class is not indended to be used directly.
// Instead, use JsonBuffer.parseArray() or .parseObject()
template <typename TReader, typename TWriter>
class MsgPackDeserializer {
 public:
  MsgPackDeserializer(JsonBuffer *buffer, TReader reader, TWriter writer,
                      uint8_t nestingLimit)
      : _buffer(buffer),
        _reader(reader),
        _writer(writer),
        _nestingLimit(nestingLimit) {}
  bool parse(JsonArray &destination);
  bool parse(JsonObject &destination);
  bool parse(JsonVariant &variant) {
    using namespace Internals;
    uint8_t c = readOne();

    if ((c & 0x80) == 0) {
      variant = c;
      return true;
    }

    if ((c & 0xe0) == 0xe0) {
      variant = static_cast<int8_t>(c);
      return true;
    }

    switch (c) {
      case 0xc0:
        variant = static_cast<char *>(0);
        return true;

      case 0xc2:
        variant = false;
        return true;

      case 0xc3:
        variant = true;
        return true;

      case 0xcc:
        variant = readInteger<uint8_t, 1>();
        return true;

      case 0xcd:
        variant = readInteger<uint16_t, 2>();
        return true;

      case 0xce:
        variant = readInteger<uint32_t, 4>();
        return true;

      case 0xcf:
#if ARDUINOJSON_USE_LONG_LONG || ARDUINOJSON_USE_INT64
        variant = readInteger<uint64_t, 8>();
#else
        variant = readInteger<uint32_t, 8>();
#endif
        return true;

      case 0xd0:
        variant = readInteger<int8_t, 1>();
        return true;

      case 0xd1:
        variant = readInteger<int16_t, 2>();
        return true;

      case 0xd2:
        variant = readInteger<int32_t, 4>();
        return true;

      case 0xd3:
#if ARDUINOJSON_USE_LONG_LONG || ARDUINOJSON_USE_INT64
        variant = readInteger<int64_t, 8>();
#else
        variant = readInteger<int32_t, 8>();
#endif
        return true;

      case 0xca:
        variant = readFloat<float>();
        return true;

      case 0xcb:
        variant = readDouble<double>();
        return true;

      default:
        return false;
    }
  }

 private:
  uint8_t readOne() {
    char c = _reader.current();
    _reader.move();
    return static_cast<uint8_t>(c);
  }

  void read(uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; i++) p[i] = readOne();
  }

  template <typename T>
  void read(T &value) {
    read(reinterpret_cast<uint8_t *>(&value), sizeof(value));
  }

  template <typename T, uint8_t size>
  T readInteger() {
    T value = static_cast<T>(readOne());
    for (uint8_t i = 1; i < size; i++) {
      value = static_cast<T>(value << 8);
      value = static_cast<T>(value | readOne());
    }
    return value;
  }

  template <typename T>
  typename EnableIf<sizeof(T) == 4, T>::type readFloat() {
    T value;
    read(value);
    fixEndianess(value);
    return value;
  }

  template <typename T>
  typename EnableIf<sizeof(T) == 8, T>::type readDouble() {
    T value;
    read(value);
    fixEndianess(value);
    return value;
  }

  template <typename T>
  typename EnableIf<sizeof(T) == 4, T>::type readDouble() {
    uint8_t i[8];  // input is 8 bytes
    T value;       // output is 4 bytes
    uint8_t *o = reinterpret_cast<uint8_t *>(&value);
    read(i, 8);
    doubleToFloat(i, o);
    fixEndianess(value);
    return value;
  }

  JsonBuffer *_buffer;
  TReader _reader;
  TWriter _writer;
  uint8_t _nestingLimit;
};

template <typename TJsonBuffer, typename TString, typename Enable = void>
struct MsgPackDeserializerBuilder {
  typedef typename StringTraits<TString>::Reader InputReader;
  typedef MsgPackDeserializer<InputReader, TJsonBuffer &> TParser;

  static TParser makeMsgPackDeserializer(TJsonBuffer *buffer, TString &json,
                                         uint8_t nestingLimit) {
    return TParser(buffer, InputReader(json), *buffer, nestingLimit);
  }
};

template <typename TJsonBuffer, typename TChar>
struct MsgPackDeserializerBuilder<
    TJsonBuffer, TChar *, typename EnableIf<!IsConst<TChar>::value>::type> {
  typedef typename StringTraits<TChar *>::Reader TReader;
  typedef StringWriter<TChar> TWriter;
  typedef MsgPackDeserializer<TReader, TWriter> TParser;

  static TParser makeMsgPackDeserializer(TJsonBuffer *buffer, TChar *json,
                                         uint8_t nestingLimit) {
    return TParser(buffer, TReader(json), TWriter(json), nestingLimit);
  }
};

template <typename TJsonBuffer, typename TString>
inline typename MsgPackDeserializerBuilder<TJsonBuffer, TString>::TParser
makeMsgPackDeserializer(TJsonBuffer *buffer, TString &json,
                        uint8_t nestingLimit) {
  return MsgPackDeserializerBuilder<
      TJsonBuffer, TString>::makeMsgPackDeserializer(buffer, json,
                                                     nestingLimit);
}
}  // namespace Internals
}  // namespace ArduinoJson

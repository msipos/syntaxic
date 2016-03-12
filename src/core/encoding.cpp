#include "core/encoding.hpp"

#include <cstring>
#include <QTextCodec>

std::vector<char> utf8_to_encoding(const char* codec_name, const std::string& str) {
  const QTextCodec* codec = QTextCodec::codecForName(codec_name);
  if (codec == nullptr) {
    throw EncodingError("No such codec: " + std::string(codec_name));
  }
  const QByteArray barr = codec->fromUnicode(QString::fromStdString(str));
  std::vector<char> vec;
  vec.reserve(barr.size());
  for (int i = 0; i < barr.size(); i++) {
    vec.push_back(barr[i]);
  }
  return vec;
}

std::string encoding_to_utf8(const char* codec_name, const char* buf, unsigned int sz) {
  const QTextCodec* codec = QTextCodec::codecForName(codec_name);
  if (codec == nullptr) {
    throw EncodingError("No such codec: " + std::string(codec_name));
  }
  const QString qstr = codec->toUnicode(buf, (int) sz);
  return qstr.toStdString();
}

bool encoding_possible(const char* codec_name, const std::string& str) {
  const QTextCodec* codec = QTextCodec::codecForName(codec_name);
  if (codec == nullptr) {
    throw EncodingError("No such codec: " + std::string(codec_name));
  }
  return codec->canEncode(QString::fromStdString(str));
}

EncodingPair ENCODINGS[] = {
  { "UTF-8", "UTF-8" },
  { "UTF-16", "UTF-16" },
  { "UTF-16BE", "UTF-16BE" },
  { "UTF-16LE", "UTF-16LE" },
  { "UTF-32", "UTF-32" },
  { "UTF-32BE", "UTF-32BE" },
  { "UTF-32LE", "UTF-32LE" },
  { "Big5", "Big5 (Taiwanese)" },
  { "EUC-JP", "EUC-JP (Japanese)" },
  { "EUC-KR", "EUC-KR (Korean)" },
  { "GBK", "Chinese GB18030/GBK/GB2312" },
  { "ISO 8859-1", "ISO 8859-1 (Latin-1/Western European)" },
  { "ISO 8859-2", "ISO 8859-2 (Latin-2/Eastern European)" },
  { "ISO 8859-3", "ISO 8859-3 (Latin-3/South European)" },
  { "ISO 8859-4", "ISO 8859-4 (Latin-4/North European)" },
  { "ISO 8859-5", "ISO 8859-5 (Latin/Cyrillic)" },
  { "ISO 8859-6", "ISO 8859-6 (Latin/Arabic)" },
  { "ISO 8859-7", "ISO 8859-7 (Latin/Greek)" },
  { "ISO 8859-8", "ISO 8859-8 (Latin/Hebrew)" },
  { "ISO 8859-9", "ISO 8859-9 (Latin-5/Turkish)" },
  { "ISO 8859-10", "ISO 8859-10 (Latin-6/Nordic)" },
  { "ISO 8859-13", "ISO 8859-13 (Latin-7/Baltic)" },
  { "ISO 8859-14", "ISO 8859-14 (Latin-8/Celtic)" },
  { "ISO 8859-15", "ISO 8859-15 (Latin-9/Western European)" },
  { "ISO 8859-16", "ISO 8859-16 (Latin-10/South-Eastern European)" },
  { "Shift-JIS", "Shift-JIS (Japanese)" },
  { "Windows-1250", "Windows-1250 (Central-Eastern European)" },
  { "Windows-1251", "Windows-1251 (Cyrillic)" },
  { "Windows-1252", "Windows-1252 (Latin)" },
  { "Windows-1253", "Windows-1253 (Greek)" },
  { "Windows-1254", "Windows-1254 (Turkish)" },
  { "Windows-1255", "Windows-1255 (Hebrew)" },
  { "Windows-1256", "Windows-1256 (Arabic)" },
  { "Windows-1257", "Windows-1257 (Baltic)" },
  { "Windows-1258", "Windows-1258 (Vietnamese)" },
};

int NUM_ENCODINGS = 35;

const char* codec_extended_to_name(const char* extended) {
  for (int i = 0; i < NUM_ENCODINGS; i++) {
    if (strcmp(extended, ENCODINGS[i].extended) == 0) {
      return ENCODINGS[i].name;
    }
  }
  return ENCODINGS[0].name;
}

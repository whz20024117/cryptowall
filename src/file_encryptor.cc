#include "file_encryptor.h"

extern "C" {
#include <unistd.h>
}
#include <cstring>
#include <queue>

#include <cryptopp/osrng.h>
#include <cryptopp/modes.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>

#include "util.h"

using std::queue;
using std::vector;
using std::string;
using CryptoPP::byte;
using CryptoPP::AES;
using CryptoPP::CFB_Mode;
using CryptoPP::FileSource;
using CryptoPP::FileSink;
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::StreamTransformationFilter;
using string_util::Base64Encode;
using string_util::Base64Decode;


const string FileEncryptor::kNewExtension = ".fuxsc";

FileEncryptor::FileEncryptor() {
  AutoSeededRandomPool rnd;
  rnd.GenerateBlock(iv_, AES::DEFAULT_BLOCKSIZE);
  rnd.GenerateBlock(key_, AES::DEFAULT_KEYLENGTH);
}

FileEncryptor::FileEncryptor(const string& b64_iv, const string& b64_key) {
  string&& iv_str = Base64Decode(b64_iv);
  string&& key_str = Base64Decode(b64_key);
  std::memcpy(iv_, iv_str.c_str(), iv_str.size());
  std::memcpy(key_, key_str.c_str(), key_str.size());
}


vector<string> FileEncryptor::ListDirectory(string directory) const {
  // If `directory` doesn't end with a slash, then append a slash.
  directory += (directory.back() != '/') ? "/" : "";

  vector<string> files;
  queue<string> q;
  q.push(directory);

  while (!q.empty()) {
    DIR* dir;
    dirent* ent;

    if ((dir = opendir(q.front().c_str()))) {
      while ((ent = readdir(dir))) {
        // If the entry it is not "." or "..", and...
        // it is a directory then push it into the queue,
        // otherwise, add it to the vector.
        if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
          if (ent->d_type == DT_DIR) {
            q.push(directory + ent->d_name + '/');
          } else {
            files.push_back(q.front() + ent->d_name);
          }
        }
      }
      closedir(dir);
    }
    q.pop();
  }
  return files;
}

// The following object created with new do not require explicit destruction.
void FileEncryptor::Encrypt(const string& filename) const {
  CFB_Mode<AES>::Encryption e(key_, sizeof(key_), iv_);

  string new_name = filename + kNewExtension;
  FileSource f(filename.c_str(), true,
      new StreamTransformationFilter(e, new HexEncoder(new FileSink(new_name.c_str()))));

  remove(filename.c_str());
}

void FileEncryptor::Decrypt(const string& filename) const {
  // Make sure the length of the file to decrypt is long enough.
  // It has at least to be something like 'a.fuxsc', i.e., length >= 7.
  if (!FilenameEndsIn(filename, kNewExtension) || filename.size() <= kNewExtension.size()) {
    return;
  }

  CFB_Mode<AES>::Decryption d(key_, sizeof(key_), iv_);

  string old_name = GetOriginalFilename(filename);
  FileSource f(filename.c_str(), true,
      new HexDecoder(new StreamTransformationFilter(d, new FileSink(old_name.c_str()))));

  remove(filename.c_str());
}

string FileEncryptor::Export() const {
  return Base64Encode(iv_, sizeof(iv_)) + '\n' + Base64Encode(key_, sizeof(key_));
}


string FileEncryptor::GetOriginalFilename(string filename) {
  return filename.erase(filename.size() - kNewExtension.size());
}

bool FileEncryptor::FilenameEndsIn(const string& filename, const string& keyword) {
  return filename.size() >= keyword.size()
    && !filename.compare(filename.size() - keyword.size(), keyword.size(), keyword);
}

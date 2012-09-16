#include <nall/file.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <nall/varint.hpp>
#include <nall/emulation/super-famicom-usart.hpp>
using namespace nall;

extern "C" usartproc void usart_main(int argc, char **argv) {
  if(argc != 4 || cstring{argv[1]} != "-o" || cstring{argv[2]} == cstring{argv[3]}) return print(
    "usart v01\n"
    "usage: usart -o output input\n"
  );

  //assemble the source file with bass
  file::remove({nall::basename(argv[3]), ".rom"});
  system(string{"bass -overwrite -o ", nall::basename(argv[3]), ".rom ", argv[3]});
  auto upload = file::read({nall::basename(argv[3]), ".rom"});
  if(upload.size() == 0) return print("error: failed to assemble source file\n");
  file::remove({nall::basename(argv[3]), ".rom"});

  //handshake
  usart_write(0x22);
  usart_write(0x5a);

  //transfer destination
  usart_write(0x00);
  usart_write(0x20);
  usart_write(0x7e);

  //transfer length
  usart_write(upload.size() >>  0);
  usart_write(upload.size() >>  8);
  usart_write(upload.size() >> 16);

  //upload program and execute it
  for(auto &byte : upload) usart_write(byte);

  file fp;
  if(fp.open(argv[2], file::mode::write) == false) return print("error: failed to open output file\n");

  //handshake
  while(usart_read() != 0x22);
  while(usart_read() != 0x5a);

  while(usart_quit() == false) {
    uint8_t command = usart_read();

    //0x00: exit
    if(command == 0x00) return;

    //0x01: write
    if(command == 0x01) {
      uint24_t length = 0;
      length |= usart_read() <<  0;
      length |= usart_read() <<  8;
      length |= usart_read() << 16;
      while(usart_quit() == false) {
        fp.write(usart_read());
        if(--length == 0) break;
      }
      continue;
    }

    //0x02: print
    if(command == 0x02) {
      while(usart_quit() == false) {
        uint8_t data = usart_read();
        if(data == 0) break;
        if(data < 0x20 || data > 0x7e) data = '.';  //unprintable character
        fputc((char)data, stdout);
      }
      fputc('\n', stdout);
      continue;
    }

    //0x03: hex
    if(command == 0x03) {
      uint8_t rows = usart_read();
      while(usart_quit() == false) {
        uint8_t data[16];
        for(auto &byte : data) byte = usart_read();
        for(auto &byte : data) print(hex<2>(byte), " ");
        for(auto &byte : data) fputc(byte < 0x20 || byte > 0x7e ? '.' : byte, stdout);
        fputc('\n', stdout);
        if(--rows == 0) break;
      }
      continue;
    }

    return print("error: unrecognized command 0x", hex<2>(command), "\n");
  }

  print("error: interrupted\n");
}

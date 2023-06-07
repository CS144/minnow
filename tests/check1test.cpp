#include "byte_stream.hh"
#include "reassembler.hh"
int main()
{
    ByteStream bs{65000};
    Reassembler ra{};
    ra.insert(2, "c", false, bs.writer());
    ra.insert(1, "bcd", false, bs.writer());
  //  ra.insert(0, "abc", false, bs.writer());

}
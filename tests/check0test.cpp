#include <byte_stream.hh>
#include <iostream>
int main()
{
    ByteStream bs{2};
    bs.writer().push("cat");
    std::string out;
    read(bs.reader(),2,out);
    std::cout<<out<<'\n';
}
#include "RootEventSource.hh"


int main(int argc, char** argv)
{
    auto source = new RootEventSource(argv[1], -1, {});
    ArrayEvent event = (*source)[0];
}
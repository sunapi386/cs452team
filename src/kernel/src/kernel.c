/**
kernel.c
mmutest.c */
#include <bwio.h>
#include <ts7200.h>
int main( int argc, char* argv[] ) {
    int data, *junk;

    data = (int *) junk;
    bwsetfifo( COM2, OFF );
    //bwputstr( COM2, "Page table base: " );
    bwputr( COM2, data );
    //bwputstr( COM2, "\n" );
    return 0;
}

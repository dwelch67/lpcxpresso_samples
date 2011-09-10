
extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern void dummy ( unsigned int );

void dowait ( void )
{
    unsigned int ra;

    for(ra=0x20000;ra;ra--)
    {
        dummy(ra);
    }
}

void notmain ( void )
{
    PUT32(0x50008000,GET32(0x50008000)|0x80);
    while(1)
    {
        PUT32(0x50000200,0x00000000);
        dowait();
        PUT32(0x50000200,0xFFFFFFFF);
        dowait();
    }
}


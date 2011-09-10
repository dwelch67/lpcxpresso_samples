
//-----------------------------------------------------------------------------
// Copyright (C) David Welch, 2010
//-----------------------------------------------------------------------------

/*
Might want to use the flash_erase_all() function on a new board

The serial port is hardcoded in ser.c, baud rate is detected by the chip
so 38400, 19200 doesnt matter.

This is a work in progress and may or may not improve in its appearance

Derived from the same program in armmite_samples, hand tuned for the
chip on the board I have.

J command returns 1023410219 which means one of these two parts
LPC1343FHN33
LPC1343FBD48

The K command, read boot code version returns version 2.5
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ser.h"

unsigned char sdata[512];
unsigned char udata[512];
unsigned char rdata[5000];

FILE *fp;
unsigned int fdatalen;
unsigned int fdata[4000];

//-----------------------------------------------------------------------------
 unsigned char syncpatt[]=
{
    0x53, //[S]
    0x79, //[y]
    0x6E, //[n]
    0x63, //[c]
    0x68, //[h]
    0x72, //[r]
    0x6F, //[o]
    0x6E, //[n]
    0x69, //[i]
    0x7A, //[z]
    0x65, //[e]
    0x64, //[d]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char syncpatt_ok[]=
{
    0x53, //[S]
    0x79, //[y]
    0x6E, //[n]
    0x63, //[c]
    0x68, //[h]
    0x72, //[r]
    0x6F, //[o]
    0x6E, //[n]
    0x69, //[i]
    0x7A, //[z]
    0x65, //[e]
    0x64, //[d]
    0x0D,
    0x4F, //[O]
    0x4B, //[K]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char okpatt[]=
{
    0x4F, //[O]
    0x4B, //[K]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char crystalpatt[]=
{
    0x31, //[1]
    0x32, //[2]
    0x30, //[0]
    0x30, //[0]
    0x30, //[0]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char crystalpatt_ok[]=
{
    0x31, //[1]
    0x32, //[2]
    0x30, //[0]
    0x30, //[0]
    0x30, //[0]
    0x0D,
    0x4F, //[O]
    0x4B, //[K]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char echo_off[]=
{
    0x41, //[A]
    0x20, //[ ]
    0x30, //[0]
    0x0D,
    0x0A,
};
//-----------------------------------------------------------------------------
 unsigned char echo_off_ok[]=
{
    0x41, //[A]
    0x20, //[ ]
    0x30, //[0]
    0x0D,
    0x30, //[0]
    0x0D,
    //0x0A,
};
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int uuencode ( unsigned char *d, unsigned char *s, unsigned int len )
{
    unsigned int ra,rb,rc;

    //pad with zeros, s buffer must have space
    //printf("len %u\n",len);
    for(ra=0;ra<len;ra+=3) ;
    for(rb=len;rb<ra;rb++) s[rb]=0x00;
    rb=0;
    d[rb++]=0x20+len;
    len=ra;
    //printf("len %u\n",len);
    for(ra=0;ra<len;ra+=3)
    {
        d[rb]=(s[ra+0]>>2);
        d[rb]&=0x3F;
        d[rb]+=0x20;
        rb++;
        d[rb]=(s[ra+1]>>4)&0x0F;
        d[rb]|=s[ra+0]<<4;
        d[rb]&=0x3F;
        d[rb]+=0x20;
        rb++;
        d[rb]=(s[ra+2]>>6)&0x03;
        d[rb]|=s[ra+1]<<2;
        d[rb]&=0x3F;
        d[rb]+=0x20;
        rb++;
        d[rb]=s[ra+2];
        d[rb]&=0x3F;
        d[rb]+=0x20;
        rb++;
    }
    d[rb]=0x00;
    for(ra=1;ra<rb;ra++) if(d[ra]==0x20) d[ra]=0x60;
    return(rb);
}
//-----------------------------------------------------------------------------
unsigned int uudecode ( unsigned char *d, unsigned char *s, unsigned int len )
{
    unsigned int ra,rb,rc,rd,re;
    unsigned int dlen;

    if(s[0]<0x20) return(0);

    dlen=s[0]-0x20;

    //printf("len %u %u\n",len,dlen);
    for(ra=1;ra<len;ra++)
    {
        if(s[ra]==0x60) s[ra]=0x20;
        if(s[ra]<0x20) return(0);
        if(s[ra]>(0x20+0x3F)) return(0);
    }

    for(ra=1;ra<len;ra+=4) ;
    for(rb=len;rb<ra;rb++) s[rb]=0x00;
    len=ra;
    //printf("len %u\n",len);

    rb=0;
    for(ra=1;ra<len;ra+=4)
    {
        rd=s[ra+0]-32;
        rd<<=2; //6
        re=s[ra+1]-32;
        re>>=4; //2
        d[rb++]=rd|re;

        rd=s[ra+1]-32;
        rd<<=4; //4
        re=s[ra+2]-32;
        re>>=2; //4
        d[rb++]=rd|re;

        rd=s[ra+2]-32;
        rd<<=6; //2
        re=s[ra+3]-32;
        //re>>=0; //6
        d[rb++]=rd|re;
    }
//for(ra=0;ra<rb;ra++) printf("0x%02X,",d[ra]); printf("\n");
    return(dlen);
}
//-----------------------------------------------------------------------------
int sync_wait_for ( unsigned char *patt, unsigned int len )
{
    unsigned int ra,rb;
//    unsigned int rblast;

//printf("-- %u\n",len);

//    rblast=0;
    while(1)
    {
        rb=ser_copystring(rdata);
//if(rb!=rblast)
//{
    //for(ra=rblast;ra<rb;ra++) printf("0x%02X\n",rdata[ra]);
    //rblast=rb;
//}
        if(rb>=len)
        {
            for(ra=0;ra<len;ra++) if(patt[ra]!=rdata[ra]) break;
            if(ra<len)
            {
printf("fail\n");
for(ra=0;ra<len;ra++) printf("0x%02X 0x%02X\n",rdata[ra],patt[ra]);


                ser_close();
                return(1);
            }
            break;
        }
    }
    ser_dump(len);
    return(0);
}
//-----------------------------------------------------------------------------
int watch_for ( void  )
{
    unsigned int ra,rb;
    unsigned int rblast;

printf("xx\n");

    rblast=0;
    while(1)
    {
        rb=ser_copystring(rdata);
if(rb!=rblast)
{
    for(ra=rblast;ra<rb;ra++) printf("0x%02X\n",rdata[ra]);
    rblast=rb;
}
    }
    return(0);
}
//-----------------------------------------------------------------------------
unsigned int wait_for_packet ( void )
{
    unsigned int ra,rb;

    while(1)
    {
        rb=ser_copystring(rdata);
        if(rb)
        {
            if(rdata[0]!=0x0A)
            {
                for(ra=0;ra<rb;ra++)
                {
                    if(rdata[ra]==0x0A) break;
                    printf("discarding 0x%02X ",rdata[ra]);
                    if((rdata[ra]>=0x20)&&(rdata[ra]<127)) printf("[%c]",rdata[ra]);
                    printf("\n");
                    ser_dump(1);
                }
                continue;
            }
            for(ra=0;ra<rb;ra++) if(rdata[ra]==0x0D) break;
            if(ra<rb) break;
        }
    }
    ra++;
    ser_dump(ra);
    rdata[ra]=0;
    return(ra);
}
//-----------------------------------------------------------------------------
unsigned int get_return_code ( void )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb=wait_for_packet();
    if(rb<3) return(111);
    rb--;
    rc=0;
    for(ra=1;ra<rb;ra++)
    {
        if(rdata[ra]<0x30) return(112);
        if(rdata[ra]>0x39) return(113);
        rc*=10;
        rc+=rdata[ra]&0xF;
    }
    if(rc) printf("returned %u\n",rc);
    else { printf("."); fflush(stdout); }
    return(rc);
}
//-----------------------------------------------------------------------------
int flash_erase_all ( void )
{
    unsigned int ra;
    ra=0;
    sdata[ra++]='P';
    sdata[ra++]=' ';
    sdata[ra++]='0';
    sdata[ra++]=' ';
    sdata[ra++]='7';
    sdata[ra++]=0x0D;
    ser_senddata(sdata,ra);
    if(get_return_code()) return(1);

    ra=0;
    sdata[ra++]='E';
    sdata[ra++]=' ';
    sdata[ra++]='0';
    sdata[ra++]=' ';
    sdata[ra++]='7';
    sdata[ra++]=0x0D;
    ser_senddata(sdata,ra);
    if(get_return_code()) return(1);

    return(0);
}
//-----------------------------------------------------------------------------
int get_checksum ( unsigned int *data )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb=wait_for_packet();
    if(rb<3) return(1);
    rb--;
    rc=0;
    for(ra=1;ra<rb;ra++)
    {
        if(rdata[ra]<0x30) return(1);
        if(rdata[ra]>0x39) return(1);
        rc*=10;
        rc+=rdata[ra]&0xF;
    }
    printf("checksum %u\n",rc);
    *data=rc;
    return(0);
}
//-----------------------------------------------------------------------------
int read_data_word ( unsigned int addr, unsigned int *data )
{
    unsigned int ra,rb,rc,rd;

    sprintf((char *)sdata,"R %u 4\r",addr);
    ra=strlen((char *)sdata);
    ser_senddata(sdata,ra);
    if(get_return_code()) return(1);

    ra=wait_for_packet();
    if(ra<3) return(1);


    for(rb=0;rb<ra;rb++) printf("0x%02X ",rdata[rb]); printf("\n");

    rb=uudecode(udata,&rdata[1],ra-2);
    rc=0;
    rd=0;
    for(ra=0;ra<rb;ra++)
    {
        rd<<=8;
        rd|=udata[ra];
        rc+=udata[ra];
        printf("0x%02X %u\n",udata[ra],rc);
    }
    if(get_checksum(&ra)) return(1);
    if(ra!=rc) return(1);
    ser_senddata(okpatt,sizeof(okpatt));
    *data=rd;
    return(0);
}
//-----------------------------------------------------------------------------
int write_data_word ( unsigned int addr, unsigned int data )
{
    unsigned int ra,rb,rc,rd;

    ra=0;
    rd=data;

    //sdata[ra++]=(rd>>24)&0xFF; rd<<=8;
    //sdata[ra++]=(rd>>24)&0xFF; rd<<=8;
    //sdata[ra++]=(rd>>24)&0xFF; rd<<=8;
    //sdata[ra++]=(rd>>24)&0xFF; rd<<=8;

    sdata[ra++]=rd&0xFF; rd>>=8;
    sdata[ra++]=rd&0xFF; rd>>=8;
    sdata[ra++]=rd&0xFF; rd>>=8;
    sdata[ra++]=rd&0xFF; rd>>=8;

    rd=0; for(rb=0;rb<ra;rb++) rd+=sdata[rb];
    rb=uuencode(udata,sdata,ra);
    udata[rb++]=0x0D;
    udata[rb++]=0x0A;
    sprintf((char *)sdata,"W %u 4\r\n",addr);
    ra=strlen((char *)sdata);
    ser_senddata(sdata,ra);
    if(get_return_code()) return(1);
    ser_senddata(udata,rb);
    sprintf((char *)sdata,"%u\r\n",rd);
    ra=strlen((char *)sdata);
    ser_senddata(sdata,ra);
    ra=wait_for_packet();
    rb=0;
    if(ra!=4) rb++;
    if(rdata[1]!='O') rb++;
    if(rdata[2]!='K') rb++;
    if(rb)
    {
        for(rc=0;rc<ra;rc++) printf("[0x%02X]",rdata[rc]); printf("\n");
        printf("%s\n",rdata);
        return(1);
    }
    return(0);
}
//-----------------------------------------------------------------------------
int lpc_isp_synchronize ( void )
{
    //We send ?[CR][LF]
    sdata[0]='?';
    sdata[1]=0x0D;
    sdata[2]=0x0A;
    ser_senddata(sdata,3);
    //They reply with Syncronized[CR][LF]
    if(sync_wait_for(syncpatt,sizeof(syncpatt))) return(1);
    //We send Synchronized[CR][LF]
    ser_senddata(syncpatt,sizeof(syncpatt));
    //They echo our Synchronized[CR][LF]
    if(sync_wait_for(syncpatt_ok,sizeof(syncpatt_ok))) return(1);
    ////They send OK[CR][LF]
    //if(sync_wait_for(okpatt,sizeof(okpatt))) return(1);
    //We send crystal frequency
    ser_senddata(crystalpatt,sizeof(crystalpatt));
    //They echo the crystal frequency
    if(sync_wait_for(crystalpatt_ok,sizeof(crystalpatt_ok))) return(1);
    ////They send OK[CR][LF]
    //if(sync_wait_for(okpatt,sizeof(okpatt))) return(1);
    printf("synchronized\n");
    return(0);
}
//-----------------------------------------------------------------------------
int do_lpc_stuff ( int erase_all )
{
    unsigned int ra,rb,rc;
    unsigned int ramadd;
    unsigned int romadd;
    unsigned int romsec;

    if(lpc_isp_synchronize()) return(1);

    //ra=0;
    //sdata[ra++]='J';
    //sdata[ra++]=0x0D;
    //ser_senddata(sdata,ra);
    //watch_for();

    //ra=0;
    //sdata[ra++]='K';
    //sdata[ra++]=0x0D;
    //ser_senddata(sdata,ra);
    //watch_for();


    //Turn off echo
    ser_senddata(echo_off,sizeof(echo_off));
    if(sync_wait_for(echo_off_ok,sizeof(echo_off_ok))) return(1);
    //----
    //the ra-1 is so that everything from the chip is 0x0A...stuff...0x0D
    //----
    //Unlock
    ra=0;
    sdata[ra++]='U';
    sdata[ra++]=' ';
    sdata[ra++]='2';
    sdata[ra++]='3';
    sdata[ra++]='1';
    sdata[ra++]='3';
    sdata[ra++]='0';
    sdata[ra++]=0x0D;
    ser_senddata(sdata,ra);
    if(get_return_code()) return(1);

    ////write program to ram and run from ram
    //rb=0x40000200;
    //for(ra=0;ra<bin_length;ra++)
    //{
        //if(write_data_word(rb,bin_data[ra])) return(1);
        //rb+=4;
    //}
    //sprintf((char *)sdata,"G %u A\r",0x40000200);
    //ra=strlen((char *)sdata);
    //ser_senddata(sdata,ra);
    ////if(get_return_code()) return(1);

   if(erase_all)
   {
       printf("Erase All\n");
       if(flash_erase_all())
       {
           printf("flash_erase_all() failed\n");
           return(1);
       }
   }

    //write program to flash

    printf("Programming 0x%04X bytes to flash\n",fdatalen<<2);

    //patch vector table, first 0x20 bytes worth of words need to add to zero
    fdata[0x14>>2]=0x0;
    rc=0; for(ra=0;ra<0x20;ra+=4) rc+=fdata[ra>>2];
    rc=-rc;
    fdata[0x14>>2]=rc;

    printf("0x%08X\n",fdata[0]);


    romadd=0x00000000;
    //ramadd=0x40000200;
    for(ra=0;ra<fdatalen;ra+=64)
    {
        romsec=romadd>>12; //lpc1334 uses 4KB sectors
        if((romadd&0xFFF)==0) //lpc13134 uses 4KB sectors
        {
            printf("\nErasing 0x%04X\n",romadd);

            sprintf((char *)sdata,"P %u %u\r",romsec,romsec);
            ser_senddata(sdata,strlen((char *)sdata));
            if(get_return_code()) return(1);

            sprintf((char *)sdata,"E %u %u\r",romsec,romsec);
            ser_senddata(sdata,strlen((char *)sdata));
            if(get_return_code()) return(1);
            ramadd=0x10001000;
        }

        printf("\nProgramming 0x%04X\n",romadd);

        rc=ramadd;
        for(rb=0;rb<64;rb++) //256 bytes per copy
        {
            if(write_data_word(rc,fdata[ra+rb])) return(1);
            rc+=4;
        }

        sprintf((char *)sdata,"P %u %u\r",romsec,romsec);
        ser_senddata(sdata,strlen((char *)sdata));
        if(get_return_code()) return(1);

        sprintf((char *)sdata,"C %u %u 256\r",romadd,ramadd);
        ser_senddata(sdata,strlen((char *)sdata));
        if(get_return_code()) return(1);

        romadd+=256;
        ramadd+=256;
    }

    printf("\n====\n");
    while(1)
    {
        rb=ser_copystring(rdata);
        for(ra=0;ra<rb;ra++)
        {
            printf("0x%02X ",rdata[ra]);
            if((rdata[ra]>=0x20)&&(rdata[ra]<127)) printf("[%c]",rdata[ra]);
            printf("\n");
        }
        ser_dump(rb);
    }

    return(0);
}
//-----------------------------------------------------------------------------
int main ( int argc, char *argv[] )
{
    unsigned int ra,rb,rc,rd;
    int ea;

    if(argc<3)
    {
        fprintf(stderr,"proglpc /dev/ttySOMETHING program.bin\n");
        fdatalen=0;
    }
    else
    {
        fp=fopen(argv[2],"rb");
        if(fp==NULL)
        {
            fprintf(stderr,"Error opening file [%s]\n",argv[2]);
            return(1);
        }
        memset(fdata,0xFF,sizeof(fdata));
        fdatalen=fread(fdata,1,sizeof(fdata),fp);
        fclose(fp);
        if(fdatalen&3) fdatalen+=4;
        fdatalen>>=2;
    }
    if(argc>3) ea=1; else ea=0;

    if(ser_open(argv[1]))
    {
        printf("ser_open(%s) failed\n",argv[1]);
        return(1);
    }
    printf("port opened\n");
    do_lpc_stuff(ea);
    ser_close();

    return(0);
}
//-----------------------------------------------------------------------------
// Copyright (C) David Welch, 2010
//-----------------------------------------------------------------------------


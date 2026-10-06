/* sn_tool.c - Seewo XHS11-EA SN block tool for TWRP (static armv7)
 * SN block layout (4096B):
 *   0x00  magic 0x48392517 (u32 le)
 *   0x08  "snum"
 *   0x50  fixed 0f82d38a
 *   0x60  length (u32 le, usually 22)
 *   0x64  SN value (22B)
 *   0xC64 CRC32 = zlib.crc32(bytes[0:0xC64]) stored u32 LE
 *
 * Commands:
 *   sn_tool dump  <dev> <hexoff> <out>    read 4096B at partition offset -> file
 *   sn_tool set   <newSN> <tpl> <out>     build new block w/ correct CRC
 *   sn_tool write <dev> <hexoff> <in>     write 4096B file at partition offset
 *   sn_tool show  <file>                  dump block info + CRC verify
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define BLK 4096
#define SN_OFF 0x64
#define SN_LEN 22
#define CRC_OFF 0xC64
#define MAGIC 0x48392517u

static uint32_t crc_table[256];
static int tbl_ready=0;
static void make_table(void){
    for(uint32_t i=0;i<256;i++){
        uint32_t c=i;
        for(int k=0;k<8;k++) c=(c>>1)^(0xEDB88320u & -(c&1));
        crc_table[i]=c;
    }
    tbl_ready=1;
}
static uint32_t crc32_zlib(const uint8_t *d, size_t n){
    if(!tbl_ready) make_table();
    uint32_t c=0xFFFFFFFFu;
    for(size_t i=0;i<n;i++) c=crc_table[(c^d[i])&0xFF]^(c>>8);
    return c^0xFFFFFFFFu;
}
static void w32(uint8_t *p, uint32_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }
static uint32_t r32(const uint8_t *p){ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }

static int read_blk(const char *dev, uint32_t off, uint8_t *out){
    int fd=open(dev,O_RDONLY); if(fd<0){fprintf(stderr,"open %s: %m\n",dev);return -1;}
    if(lseek(fd,(off_t)off,SEEK_SET)<0){fprintf(stderr,"lseek %u: %m\n",off);close(fd);return -1;}
    ssize_t n=read(fd,out,BLK); close(fd);
    if(n!=BLK){fprintf(stderr,"read %zd (want %d)\n",n,BLK);return -1;}
    return 0;
}
static int write_blk(const char *dev, uint32_t off, const uint8_t *in){
    int fd=open(dev,O_WRONLY); if(fd<0){fprintf(stderr,"open %s: %m\n",dev);return -1;}
    if(lseek(fd,(off_t)off,SEEK_SET)<0){fprintf(stderr,"lseek %u: %m\n",off);close(fd);return -1;}
    ssize_t n=write(fd,in,BLK); close(fd);
    if(n!=BLK){fprintf(stderr,"write %zd (want %d)\n",n,BLK);return -1;}
    return 0;
}
/* flip: read SN from template, toggle char[7] (0<->F), recompute CRC.
 * Format expected: YLXHS11[0|F]...   (prefix check on SN[0:7]) */
static int flip_block(const uint8_t *tpl, uint8_t *out){
    memcpy(out,tpl,BLK);
    if(r32(out+0)!=MAGIC){fprintf(stderr,"bad magic\n");return -1;}
    char *sn=(char*)out+SN_OFF;
    if(memcmp(sn,"YLXHS11",7)!=0){
        fprintf(stderr,"SN prefix not YLXHS11 (sn=[%.22s])\n",sn); return -1;
    }
    char c=sn[7];
    if(c=='0') sn[7]='F';
    else if(c=='F') sn[7]='0';
    else { fprintf(stderr,"SN[7]='%c' not 0/F, refuse\n",c); return -1; }
    w32(out+CRC_OFF, crc32_zlib(out,CRC_OFF));
    return 0;
}
static int build(const char *sn, const uint8_t *tpl, uint8_t *out){
    memcpy(out,tpl,BLK);
    if(r32(out+0)!=MAGIC){fprintf(stderr,"bad magic 0x%08x (not a snum block?)\n",r32(out));return -1;}
    memset(out+SN_OFF,0,SN_LEN);
    size_t l=strlen(sn); if(l>SN_LEN) l=SN_LEN;
    memcpy(out+SN_OFF,sn,l);
    w32(out+0x60,SN_LEN);
    w32(out+CRC_OFF, crc32_zlib(out,CRC_OFF));
    return 0;
}
int main(int argc,char**argv){
    if(argc<2){fprintf(stderr,"usage: %s {dump|set|write|show} ...\n",argv[0]);return 2;}
    const char*cmd=argv[1];
    if(!strcmp(cmd,"show")){
        if(argc<3) return 2;
        uint8_t b[BLK]; FILE*f=fopen(argv[2],"rb"); if(!f){perror("open");return 1;}
        if(fread(b,1,BLK,f)!=BLK){fprintf(stderr,"short file\n");fclose(f);return 1;} fclose(f);
        uint32_t magic=r32(b+0), crc=r32(b+CRC_OFF), calc=crc32_zlib(b,CRC_OFF);
        printf("magic  0x%08x (%s)\n",magic, magic==MAGIC?"ok":"BAD");
        printf("len    %u\n",r32(b+0x60));
        printf("sn     [%.22s]\n",(char*)b+SN_OFF);
        printf("crc    stored 0x%08x calc 0x%08x %s\n",crc,calc, crc==calc?"PASS":"FAIL");
        return 0;
    }
    else if(!strcmp(cmd,"dump")){
        if(argc<5) return 2;
        uint8_t b[BLK]; uint32_t off=(uint32_t)strtoul(argv[3],0,0);
        if(read_blk(argv[2],off,b)) return 1;
        FILE*f=fopen(argv[4],"wb"); if(!f){perror("open");return 1;} fwrite(b,1,BLK,f); fclose(f);
        printf("dumped %d bytes @0x%x -> %s\n",BLK,off,argv[4]);
        return 0;
    }
    else if(!strcmp(cmd,"set")){
        if(argc<5) return 2;
        uint8_t tpl[BLK],out[BLK]; FILE*f=fopen(argv[3],"rb"); if(!f){perror("open");return 1;}
        if(fread(tpl,1,BLK,f)!=BLK){fprintf(stderr,"short template\n");fclose(f);return 1;} fclose(f);
        if(build(argv[2],tpl,out)) return 1;
        f=fopen(argv[4],"wb"); if(!f){perror("open");return 1;} fwrite(out,1,BLK,f); fclose(f);
        printf("set SN [%.22s] crc 0x%08x -> %s\n",argv[2],r32(out+CRC_OFF),argv[4]);
        return 0;
    }
    else if(!strcmp(cmd,"flip")){
        if(argc<4) return 2;
        uint8_t tpl[BLK],out[BLK]; FILE*f=fopen(argv[2],"rb"); if(!f){perror("open");return 1;}
        if(fread(tpl,1,BLK,f)!=BLK){fprintf(stderr,"short template\n");fclose(f);return 1;} fclose(f);
        if(flip_block(tpl,out)) return 1;
        f=fopen(argv[3],"wb"); if(!f){perror("open");return 1;} fwrite(out,1,BLK,f); fclose(f);
        char oldc=tpl[SN_OFF+7], newc=out[SN_OFF+7];
        printf("flip SN[7]: idx7 '%c'->'%c'  sn=[%.22s] crc 0x%08x -> %s\n",
               oldc, newc, (char*)out+SN_OFF, r32(out+CRC_OFF), argv[3]);
        return 0;
    }
    else if(!strcmp(cmd,"write")){
        if(argc<5) return 2;
        uint8_t b[BLK]; FILE*f=fopen(argv[4],"rb"); if(!f){perror("open");return 1;}
        if(fread(b,1,BLK,f)!=BLK){fprintf(stderr,"short file\n");fclose(f);return 1;} fclose(f);
        uint32_t off=(uint32_t)strtoul(argv[3],0,0);
        if(write_blk(argv[2],off,b)) return 1;
        printf("wrote %d bytes @0x%x -> %s\n",BLK,off,argv[2]);
        return 0;
    }
    fprintf(stderr,"unknown cmd %s\n",cmd); return 2;
}

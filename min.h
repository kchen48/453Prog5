/* Kelly Chen (kchen48)
 * Richard Ding (riding)
 *
 * Header file for Asgn 5 */

struct superblock {
   uint32_t ninodes; /* number of inodes in this filesystem */
   uint16_t pad1; /* make things line up properly */
   int16_t i_blocks; /* # of blocks used by inode bit map */
   int16_t z_blocks; /* number of blocks used by zone bit map */
   uint16_t firstdata; /* number of first data zone */
   int16_t log_zone_size; /* log2 of blocks per zone */
   int16_t pad2; /* make things line up again */
   uint32_t max_file; /* maximum file size */
   uint32_t zones; /* number of zones on disk */
   int16_t magic; /* magic number */
   int16_t pad3; /* make things line up again */
   uint16_t blocksize; /* block size in bytes */
   uint8_t subversion; /* filesystem sub-version */
};

#define DIRECT_ZONES 7
#define LENFILENAMES 60

struct inode {
   uint16_t mode;
   uint16_t links;
   uint16_t uid;
   uint16_t gid;
   uint32_t size;
   int32_t atime;
   int32_t mtime;
   int32_t ctime;
   uint32_t zone[DIRECT_ZONES];
   uint32_t indirect;
   uint32_t two_indirect;
   uint32_t unused;
};

#ifndef DIRSIZ
#define DIRSIZ 60
#endif

struct fileent{
   uint32_t ino;
   char name[DIRSIZ];
};

#define PTABLE_OFFSET 0x1BE
#define PMAGIC510 0x55
#define PMAGIC511 0xAA
#define MINIXPART 0x81

#define MIN_MAGIC 0x4d5a /* minix magic number */
#define MIN_MAGIC_REV 0x5a4d /* minix magic number reversed */
#define MIN_MAGIC_OLD 0x2468 /* v2 minix magic number */
#define MIN_MAGIC_REV_OLD 0x6824 /* v2 magic number reversed */

#define LENPERMS 11
#define LENBLOCKS 1024
#define LENSECTORS 512
#define SIGBYTE510 510
#define SIGBYTE511 511

#define MIN_ISREG(m) (((m)&0170000)==0100000)
#define MIN_ISDIR(m) (((m)&0170000)==0040000)
#define MIN_IRUSR 0400
#define MIN_IWUSR 0200
#define MIN_IXUSR 0100
#define MIN_IRGRP 0040
#define MIN_IWGRP 0020
#define MIN_IXGRP 0010
#define MIN_IROTH 0004
#define MIN_IWOTH 0002
#define MIN_IXOTH 0001

struct f{
   struct inode node;
   struct fileent ent;
   uint8_t *cont;
   struct fileent *ents;
   int numEnts;
   char *path;
};

struct info {
   char *image;
   char *src;
   char *dstpath;
   int part;
   int sub;
   int verbose;
   FILE *f;
   long place;
   long zonesize;
   struct superblock superBlock;
};

struct pt {
   uint8_t bootind;
   uint8_t start_head;
   uint8_t start_sec;
   uint8_t start_cyl;
   uint8_t type;
   uint8_t end_head;
   uint8_t end_sec;
   uint8_t end_cyl;
   uint32_t lFirst;
   uint32_t size;
};

int checkFlag(char*, int, char**);
void partAndSubpart(struct info*);
void partition(struct info*);
void subpartition(struct info*);
void superBlock(struct info*);
void findInode(struct info*, struct inode*, uint32_t);
uint32_t getInodePath(struct info*, char*);
uint32_t getInodeReg(struct info*, char*, uint32_t);
struct f* findFilePath(struct info*, char*);
struct f* openFile(struct info*, uint32_t);
void loadFile(struct info*, struct f*);
void loadDirectory(struct info*, struct f*);
void openImg(struct info*);
void freeFile(struct f*);
void printPermissions(uint16_t);
void printInode(struct inode*);
void printSuperBlock(struct superblock*);
void printPT(struct pt*);
void printEnts(struct info*, struct fileent*);
void printFile(struct info*);
void writeOut(struct info*);
void* checked_malloc(size_t);

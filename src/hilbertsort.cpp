#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

#include "pointset.h"
#include "timing.h"
#include "octree.h"

static constexpr uint64_t B[] = {
  0xFFFF00000000FFFF, 
  0x00FF0000FF0000FF, 
  0xF00F00F00F00F00F, 
  0x30C30C30C30C30C3, 
  0x9249249249249249,
};
static constexpr uint64_t S[] = {32, 16, 8, 4, 2};
    
uint64_t morton3d( uint64_t x, uint64_t y, uint64_t z ) {   
  // pack 3 32-bit indices into a 96-bit Morton code
  // except that the result is truncated to 64-bit.
  for (uint64_t i=0; i<5; i++) {
    x = (x | (x << S[i])) & B[i];
    y = (y | (y << S[i])) & B[i];
    z = (z | (z << S[i])) & B[i];
  }
  return x | (y<<1) | (z<<2);
}

uint64_t hilbert3d( const point & p ) {
  uint64_t val = morton3d( p.x,p.y,p.z );
  uint64_t start = 0;
  uint64_t end = 1; // can be 1,2,4
  uint64_t ret = 0;
  for (int64_t j=19; j>=0; j--) {
    uint64_t rg = ((val>>(3*j))&7) ^ start;
    uint64_t travel_shift = (0x30210 >> (start ^ end)*4)&3;
    uint64_t i = (((rg << 3) | rg) >> travel_shift ) & 7;
    i = (0x54672310 >> i*4) & 7;
    ret = (ret<<3) | i;
    uint64_t si = (0x64422000 >> i*4 ) & 7; // next lower even number, or 0
    uint64_t ei = (0x77755331 >> i*4 ) & 7; // next higher odd number, or 7
    uint64_t sg = ( si ^ (si>>1) ) << travel_shift;
    uint64_t eg = ( ei ^ (ei>>1) ) << travel_shift;
    end   = ( ( eg | ( eg >> 3 ) ) & 7 ) ^ start;
    start = ( ( sg | ( sg >> 3 ) ) & 7 ) ^ start;
  }
  return ret;
}
    
bool hilbert3d_compare( const point & p1,const point & p2 ) {
  uint64_t val1 = morton3d( p1.x,p1.y,p1.z );
  uint64_t val2 = morton3d( p2.x,p2.y,p2.z );
  uint64_t start = 0;
  uint64_t end = 1; // can be 1,2,4
  for (int64_t j=19; j>=0; j--) {
    uint64_t travel_shift = (0x30210 >> (start ^ end)*4)&3;
    uint64_t rg1 = ((val1>>(3*j))&7) ^ start;
    uint64_t rg2 = ((val2>>(3*j))&7) ^ start;
    uint64_t i1 = (((rg1 << 3) | rg1) >> travel_shift ) & 7;
    uint64_t i2 = (((rg2 << 3) | rg2) >> travel_shift ) & 7;
    i1 = (0x54672310 >> i1*4) & 7;
    i2 = (0x54672310 >> i2*4) & 7;
    if (i1<i2) return true;
    if (i1>i2) return false;
    uint64_t si = (0x64422000 >> i1*4 ) & 7; // next lower even number, or 0
    uint64_t ei = (0x77755331 >> i1*4 ) & 7; // next higher odd number, or 7
    uint64_t sg = ( si ^ (si>>1) ) << travel_shift;
    uint64_t eg = ( ei ^ (ei>>1) ) << travel_shift;
    end   = ( ( eg | ( eg >> 3 ) ) & 7 ) ^ start;
    start = ( ( sg | ( sg >> 3 ) ) & 7 ) ^ start;
  }
  return false;
}

int main(int argc, char ** argv){
  Timer t;
  if (argc != 2 && argc != 4) {
    fprintf(stderr,"Please specify the file to convert (without '.vxl') and optionally repeat mask & depth.\n");
    exit(2);
  }
  
  // Determine repeat arguments
  int repeat_mask=7;
  int repeat_depth=0;
  if (argc == 4) {
    char * endptr = NULL;
    repeat_mask  = strtol(argv[2], &endptr, 10);
    if (errno) {perror("Could not parse mask"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(repeat_mask>=0 && repeat_mask<8);
    repeat_depth = strtol(argv[3], &endptr, 10);
    if (errno) {perror("Could not parse depth"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(repeat_depth>=0 && repeat_depth<16);
    int dirs = __builtin_popcount(7^repeat_mask);
    printf("[%10.0f] Result cloned %d times at %d layers in %s%s%s direction(s).\n", t.elapsed(), 1<<dirs*repeat_depth, repeat_depth, repeat_mask&4?"":"X", repeat_mask&2?"":"Y", repeat_mask&1?"":"Z");
  }

  // Determine the file names.
  char * name = argv[1];
  int length=strlen(name);
  char infile[length+9];
  char outfile[length+9];
  sprintf(infile, "vxl/%s.vxl", name);
  sprintf(outfile, "vxl/%s.oct", name);
  
  printf("[%10.0f] Opening '%s'.\n", t.elapsed(), infile);
  pointset p(infile, true);
  
  printf("[%10.0f] Checking if %d points are sorted.\n", t.elapsed(), p.length);
  int64_t old = 0;
  for (uint64_t i=0; i<p.length; i++) {
    if (i && (i&0x3fffff)==0) {
      printf("[%10.0f] Checking ... %6.2f%%.\n", t.elapsed(), i*100.0/p.length);
    }
    int64_t cur = hilbert3d(p.list[i]);
    if (old>cur) {
      printf("[%10.0f] Point %lu should precede previous point.\n", t.elapsed(), i);
      printf("[%10.0f] Sorting points.\n", t.elapsed());
      std::stable_sort(p.list, p.list+p.length, hilbert3d_compare);
      break;
    }
    old = cur;
  }
  
  printf("[%10.0f] Counting nodes per layer.\n", t.elapsed());
  uint64_t nodecount[20];
  int64_t maxnode=0;
  for (int j=0; j<20; j++) nodecount[j]=0;
  old = -1;
  for (uint64_t i=0; i<p.length; i++) {
    if (i && (i&0x3fffff)==0) {
      printf("[%10.0f] Counting ... %6.2f%%.\n", t.elapsed(), i*100.0/p.length);
    }
    point q = p.list[i];
    int64_t cur = morton3d(q.x, q.y, q.z);
    for (int j=0; j<20; j++) {
      if ((cur>>j*3)!=(old>>j*3)) {
        nodecount[19-j]++;
      }
    }
    old = cur;
    if (maxnode<cur)
      maxnode=cur;
  }
  printf("[%10.0f] Counting layers (maxnode=0x%lx).\n", t.elapsed(), maxnode);
  int layers=0;
  while(maxnode>>layers*3) layers++;
  printf("[%10.0f] Found 1 leaf layer + %d data layers + %d repetition layers.\n", t.elapsed(), layers, repeat_depth);
  int nonlayers=19-layers-repeat_depth;
  assert(nonlayers>=0);
  
  uint64_t nodesum = 0;
  for (int i=nonlayers; i<20; i++) {
    if (i<19) {
      printf("[%10.0f] At layer %2d: %8lu nodes.\n", t.elapsed(), i-nonlayers, nodecount[i]);
      nodesum += nodecount[i];
    } else {
      printf("[%10.0f] At layer %2d: %8lu leaves.\n", t.elapsed(), i-nonlayers, nodecount[i]);
    }
  }
  printf("[%10.0f] Creating octree file with %lu nodes of %luB each (%luMiB).\n", t.elapsed(), nodesum, sizeof(octree), nodesum*sizeof(octree)>>20);
  
  printf("[%10.0f] Done.\n", t.elapsed());

}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 


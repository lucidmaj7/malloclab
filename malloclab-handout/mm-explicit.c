/*
 * mm-explicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 200802201 
 * @name : wonhee jeong
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define HDRSIZE 4
#define FTRSIZE 4
#define WSIZE 4
 #define DSIZE 8
 #define CHUNKSIZE (1<<12)
 #define OVERHEAD 8

 #define MAX(x,y)	((x)>(y) ? (x):(y))
 #define MIN(x,y)	((x)<(y)) ? (x):(y))

 #define PACK(size, alloc) ((unsigned) ((size)| (alloc)))
#define GET(p) 	(*(unsigned *) (p))
#define PUT(p,val) (*(unsigned*)(p)= (unsigned)(val))
 #define GET8(p) (*(unsigned long *) (p))
 #define PUT8(p,val) (*(unsigned long*)(p)= (unsigned long)(val))


 #define GET_SIZE(p) (GET(p) & ~0x7)
 #define GET_ALLOC(p) (GET(p)& 0x1)

#define HDRP(bp) ((char*)(bp)-WSIZE)
 #define FTRP(bp) ((char*)(bp)+ GET_SIZE(HDRP(bp))-DSIZE)

 #define NEXT_BLKP(bp) ((char*)(bp)+ GET_SIZE(HDRP(bp)))
 #define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))
 #define NEXT_FREEP(bp) ((char*)(bp))
 #define PREV_FREEP(bp) ((char*)(bp)+WSIZE)

 #define NEXT_FREE_BLKP(bp) ((char* ) GET8((char*)(bp)))
 #define PREV_FREE_BLKP(bp) ((char*)GET8((char*)(bp)+WSIZE))


inline void *extend_heap(size_t words);
static void* coalesce(void* bp);
static void place(void* bp, size_t asize);
static void* find_fit(size_t asize);



static char * heap_start;
static char * epilogue;
static char * freeblkp;
static void * nextfitp=0;
static void addFreeBlock(void* bp);

static void deleteFreeeBlock(void* bp);




/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)



static void* find_fit(size_t asize){



	char* bp;
	
	for(bp = freeblkp; bp!=NULL ;  bp = NEXT_FREE_BLKP(bp))
	{
 	
		if( (asize<=GET_SIZE(HDRP(bp))))
		{
			//printf("%x\n",NEXT_FREE_BLKP(freeblkp));
			return bp;
		}

	}


	return NULL;
}
/*
 * Initialize: return -1 on error, 0 on success.
 */


int mm_init(void) {
	char* h_ptr;
	if((h_ptr= mem_sbrk(DSIZE+4*HDRSIZE))==NULL)
		return -1;
	heap_start=h_ptr;

	PUT(h_ptr, NULL);  //next
	PUT(h_ptr+ WSIZE, NULL); //prev
	PUT(h_ptr+DSIZE, 0); //payload? 

	// prologue
	PUT(h_ptr+DSIZE+HDRSIZE, PACK(OVERHEAD,1)); // prologue hdr
	PUT(h_ptr+DSIZE+HDRSIZE+FTRSIZE, PACK(OVERHEAD,1)); // prologue ftr
	PUT(h_ptr+DSIZE+2*HDRSIZE +FTRSIZE,PACK(0,1)); //epilogue

	h_ptr+=DSIZE+DSIZE; //prologue hdr, ftr 사이에 위치. 
	freeblkp=NULL;

	//nextfitp=NULL;


	epilogue= h_ptr+HDRSIZE;
	if(extend_heap(80/WSIZE)==NULL)
		return -1;

    return 0;
}

static void deleteFreeeBlock(void* bp)
{

	if(NEXT_FREE_BLKP(bp)==NULL && PREV_FREE_BLKP(bp)==NULL)  // 단독. 
	{
		freeblkp=NULL;
	}

	else if(NEXT_FREE_BLKP(bp)!=NULL && PREV_FREE_BLKP(bp)!=NULL) //중간 
	{
		PUT8(NEXT_FREEP(PREV_FREE_BLKP(bp)), NEXT_FREE_BLKP(bp));

		PUT8( PREV_FREEP( NEXT_FREE_BLKP(bp)), PREV_FREE_BLKP(bp));


	}
	else if(NEXT_FREE_BLKP(bp)==NULL && PREV_FREE_BLKP(bp)!=NULL) //리스트의 끝. 
	{
		 PUT8(NEXT_FREEP(PREV_FREE_BLKP(bp)),NULL);

	}
	else if(NEXT_FREE_BLKP(bp)!=NULL && PREV_FREE_BLKP(bp)==NULL) //리스트의 처음 
	{
		freeblkp=NEXT_FREE_BLKP(bp);
		PUT8( PREV_FREEP( NEXT_FREE_BLKP(bp)) , NULL);
		
	}



}

static void addFreeBlock(void* bp)
{
	char *tempp;

	if(freeblkp==NULL)
	{
		freeblkp=bp;
		nextfitp=freeblkp	;	
		PUT8(NEXT_FREEP(freeblkp),NULL);

		PUT8(PREV_FREEP(freeblkp),NULL);
	}else
	{
		
		PUT8(NEXT_FREEP(bp),freeblkp);
		PUT8(PREV_FREEP(bp),NULL);
		PUT8(PREV_FREEP(freeblkp),bp);

		freeblkp=bp;
		
	}


}

static void* coalesce(void* bp)
{

	
 	//	printf("coalesce \n");
	size_t prev_alloc= GET_ALLOC(FTRP(PREV_BLKP(bp)));
	//printf("size_t prev_alloc= GET_ALLOC(FTRP(PREV_BLKP(bp))); \n");
	size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));

	size_t size= GET_SIZE(HDRP(bp));

	char *tempp;

	if(prev_alloc && next_alloc)
	{
			
		addFreeBlock(bp);
	}

	else if(prev_alloc && !next_alloc)
	{
		

//		if(NEXT_FREE_BLKP(NEXT_BLKP(bp))!=NULL)
		

		size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));

		deleteFreeeBlock(NEXT_BLKP(bp));
		
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp), PACK(size,0));
		
		

		addFreeBlock(bp);




	}
	else if(!prev_alloc && next_alloc){



		size+=GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp),PACK(size,0));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		
		deleteFreeeBlock(PREV_BLKP(bp));

		
		addFreeBlock(PREV_BLKP(bp));

		bp=freeblkp;
		

	
	}
	else
	{
		size+=GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
	
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));

		deleteFreeeBlock(NEXT_BLKP(bp));
		deleteFreeeBlock(PREV_BLKP(bp));
		bp=PREV_BLKP(bp);
		addFreeBlock(bp);

		bp=freeblkp;




	}
	////printf("calesce end\n");
//	if ((nextfitp > bp) && (nextfitp  < NEXT_BLKP(bp ))) 
//		nextfitp=bp;







	return bp;

//	return NULL;
}





static void place(void* bp, size_t asize)
{

	size_t csize= GET_SIZE(HDRP(bp));
	char* tempp;
	if((csize-asize) >= (2*DSIZE))
	{
	
		PUT(HDRP(bp), PACK(asize,1));
		PUT(FTRP(bp), PACK(asize, 1));

	//	tempp=bp;


		deleteFreeeBlock(bp);

		bp=NEXT_BLKP(bp);
		PUT(HDRP(bp),PACK(csize-asize,0));
		PUT(FTRP(bp),PACK(csize-asize,0));

		addFreeBlock(bp);


	
	}
	else
	{

		PUT(HDRP(bp),PACK(csize,1));
		PUT(FTRP(bp),PACK(csize,1));

		deleteFreeeBlock(bp);





	}

//	printf("%x\n",freeblkp);


	//return NULL;

}
inline void *extend_heap(size_t words)
{

	unsigned *old_epilogue;
	char* bp;
	unsigned size;

	size=(words%2 ) ? (words+1)*WSIZE: words*WSIZE;

	if((long)(bp=mem_sbrk(size))<0)
		return NULL;

	old_epilogue= epilogue;
	epilogue= bp+size- HDRSIZE;

	PUT(HDRP(bp), PACK(size,0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(epilogue, PACK(0,1));
	


	return coalesce(bp);

}

/*
 * malloc
 */
void *malloc (size_t size) {

	char* bp;
	unsigned asize;
	unsigned extendsize;

	if(size==0)
		return NULL;

	if(size <= DSIZE)
		asize=2*DSIZE;
	else
		asize=DSIZE* ((size+(DSIZE)+(DSIZE-1))/DSIZE);

	if((bp=find_fit(asize))!=NULL)
	{
//printf("asize %d\n",asize);
		place(bp, asize);

		//
			//printf("asize %d,%x\n",asize,bp);
		return bp;

	}
	//printf("asize %d,%x\n",asize,bp);
	extendsize= MAX(asize,80);

	if((bp=extend_heap(extendsize/WSIZE))==NULL)
		return NULL;

	place(bp, asize);

	return bp;


    //return NULL;
}

/*
 * free
 */
void free (void *ptr) {
    if(!ptr) return;

     size_t size=GET_SIZE(HDRP(ptr));



    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
   // printf("mm_init(void\n");
	coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
        size_t oldsize;
  void *newptr;

  /* If size == 0 then this is just free, and we return NULL. */
  if(size == 0) {
    free(oldptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if(oldptr == NULL) {
    return malloc(size);
  }

  newptr = malloc(size);

  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = GET_SIZE(HDRP(oldptr));
  if(size < oldsize) oldsize = size;
  memcpy(newptr, oldptr, oldsize);

  /* Free the old block. */
  free(oldptr);

  return newptr;

}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    return NULL;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}

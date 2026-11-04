# include <stdio.h>
# include "pf.h"
# include "am.h"
#include <string.h>

extern void free(void *ptr);

/* Creates a secondary idex file called fileName.indexNo */
AM_CreateIndex(fileName,indexNo,attrType,attrLength)
char *fileName;/* Name of indexed file */
int indexNo;/*number of this index for file */
char attrType;/* 'c' for char ,'i' for int ,'f' for float */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */


{
	char *pageBuf; /* buffer for holding a page */
	char indexfName[AM_MAX_FNAME_LENGTH]; /* String to store the indexed
					 files name with extension           */
	int pageNum; /* page number of the root page(also the first page) */
	int fileDesc; /* file Descriptor */
	int errVal;
	int maxKeys;/* Maximum keys that can be held on one internal page */
	AM_LEAFHEADER head,*header;

	/* Check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                 }
        
	if ((attrLength < 1) || (attrLength > 255))
		{
		 AM_Errno = AME_INVALIDATTRLENGTH;
		 return(AME_INVALIDATTRLENGTH);
                }
	
	if (attrLength != 4)
		if (attrType !='c')
			{
			 AM_Errno = AME_INVALIDATTRLENGTH;
			 return(AME_INVALIDATTRLENGTH);
                        }
	
	header = &head;
	
	/* Get the filename with extension and create a paged file by that name*/
	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_CreateFile(indexfName);
	AM_Check;

	/* open the new file */
	fileDesc = PF_OpenFile(indexfName);
	if (fileDesc < 0) 
	  {
	   AM_Errno = AME_PF;
	   return(AME_PF);
          }

	/* allocate a new page for the root */
	errVal = PF_AllocPage(fileDesc,&pageNum,&pageBuf);
	AM_Check;
	
	/* initialise the header */
	header->pageType = 'l';
	header->nextLeafPage = AM_NULL_PAGE;
	header->recIdPtr = PF_PAGE_SIZE;
	header->keyPtr = AM_sl;
	header->freeListPtr = AM_NULL;
	header->numinfreeList = 0;
	header->attrLength = attrLength;
	header->numKeys = 0;
	/* the maximum keys in an internal node- has to be even always*/
	maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);
	if (( maxKeys % 2) != 0) 
		header->maxKeys = maxKeys - 1;
	else 
		header->maxKeys = maxKeys;
	/* copy the header onto the page */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	AM_Check;
	
	/* Close the file */
	errVal = PF_CloseFile(fileDesc);
	AM_Check;
	
	/* initialise the root page and the leftmost page numbers */
	AM_RootPageNum = pageNum;
	return(AME_OK);
}


/* Destroys the index fileName.indexNo */
AM_DestroyIndex(fileName,indexNo)
char *fileName;/* name of indexed file */
int indexNo; /* number of this index for file */

{
	char indexfName[AM_MAX_FNAME_LENGTH];
	int errVal;

	sprintf(indexfName,"%s.%d",fileName,indexNo);
	errVal = PF_DestroyFile(indexfName);
	AM_Check;
	return(AME_OK);
}


/* Deletes the recId from the list for value and deletes value if list
becomes empty */
AM_DeleteEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'c' , 'i' or 'f' */
int attrLength; /* 4 for 'i' or 'f' , 1-255 for 'c' */
char *value;/* Value of key whose corr recId is to be deleted */
int recId; /* id of the record to delete */

{
	char *pageBuf;/* buffer to hold the page */
	int pageNum; /* page Number of the page in buffer */
	int index;/* index where key is present */
	int status; /* whether key is in tree or not */
	short nextRec;/* contains the next record on the list */
	short oldhead; /* contains the old head of the list */
	short temp; 
	char *currRecPtr;/* pointer to the current record in the list */
	AM_LEAFHEADER head,*header;/* header of the page */
	int recSize; /* length of key,ptr pair for a leaf */
	int tempRec; /* holds the recId of the current record */
	int errVal; /* holds the return value of functions called within 
				                            this function */
	int i; /* loop index */


	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                }

	if (value == NULL) 
		{
		 AM_Errno = AME_INVALIDVALUE;
		 return(AME_INVALIDVALUE);
                }

	if (fileDesc < 0) 
		{
		 AM_Errno = AME_FD;
		 return(AME_FD);
                }

	/* initialise the header */
	header = &head;
	
	/* find the pagenumber and the index of the key to be deleted if it is
	there */
	status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);
	
	/* check if return value is an error */
	if (status < 0) 
		{
		 AM_Errno = status;
		 return(status);
                }
	
	/* The key is not in the tree */
	if (status == AM_NOT_FOUND) 
		{
		 AM_Errno = AME_NOTFOUND;
		 return(AME_NOTFOUND);
                }
	
	bcopy(pageBuf,header,AM_sl);
	recSize = attrLength + AM_ss;
	currRecPtr = pageBuf + AM_sl + (index - 1)*recSize + attrLength;
	bcopy(currRecPtr,&nextRec,AM_ss);
	
	/* search the list for recId */
	while(nextRec != 0)
	{
		bcopy(pageBuf + nextRec,&tempRec,AM_si);
		
		/* found the recId to be deleted */
		if (recId == tempRec)
		{
			/* Delete recId */
			bcopy(pageBuf + nextRec + AM_si,currRecPtr,AM_ss);
			header->numinfreeList++;
			oldhead = header->freeListPtr;
			header->freeListPtr = nextRec;
			bcopy(&oldhead,pageBuf + nextRec + AM_si,AM_ss);
			break;
		}
		else 
	        {
			/* go over to the next item on the list */
			currRecPtr = pageBuf + nextRec + AM_si;
			bcopy(currRecPtr,&nextRec,AM_ss);
		}
	}
	
	/* if end of list reached then key not in tree */
	if (nextRec == AM_NULL)
		{
		 AM_Errno = AME_NOTFOUND;
		 return(AME_NOTFOUND);
                }
	
	/* check if list is empty */
	bcopy(pageBuf + AM_sl + (index - 1)*recSize + attrLength,&temp,AM_ss);
	if (temp == 0)
	{
		/* list is empty , so delete key from the list */
		for(i = index; i < (header->numKeys);i++)
			bcopy(pageBuf + AM_sl + i*recSize,pageBuf + AM_sl + 
				(i-1)*recSize,recSize);
		header->numKeys--;
		header->keyPtr = header->keyPtr - recSize;
	}
	
	/* copy the header onto the buffer */
	bcopy(header,pageBuf,AM_sl);
	
	errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
	
	/* empty the stack so that it is set for next amlayer call */
	AM_EmptyStack();
	  {
	   AM_Errno = AME_OK;
	   return(AME_OK);
          }
}








/* Inserts a value,recId pair into the tree */
AM_InsertEntry(fileDesc,attrType,attrLength,value,recId)
int fileDesc; /* file Descriptor */
char attrType; /* 'i' or 'c' or 'f' */
int attrLength; /* 4 for 'i' or 'f', 1-255 for 'c' */
char *value; /* value to be inserted */ 
int recId; /* recId to be inserted */

{
	char *pageBuf; /* buffer to hold page */
	int pageNum; /* page number of the page in buffer */
	int index; /* index where key can be found or can be inserted */
	int status; /* whether key is old or new */
	int inserted; /* Whether key has been inserted into the leaf or 
	                      splitting is needed */
	int addtoparent; /* Whether key has to be added to the parent */ 
	int errVal; /* return value of functions within this function */
	char key[AM_MAXATTRLENGTH]; /* holds the attribute to be passed 
						  back to the parent */

	
	/* check the parameters */
	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
		{
		 AM_Errno = AME_INVALIDATTRTYPE;
		 return(AME_INVALIDATTRTYPE);
                }

	if (value == NULL) 
		{
		 AM_Errno = AME_INVALIDVALUE;
		 return(AME_INVALIDVALUE);
                }

	if (fileDesc < 0) 
		{
		 AM_Errno = AME_FD;
		 return(AME_FD);
                }
	
	
	/* Search the leaf for the key */
	status = AM_Search(fileDesc,attrType,attrLength,value,&pageNum,
			   &pageBuf,&index);


	
	/* check if there is an error */
	if (status < 0) 
	{ 
		AM_EmptyStack();
		AM_Errno = status;
		return(status);
	}
	
	/* Insert into leaf the key,recId pair */
	inserted = AM_InsertintoLeaf(pageBuf,attrLength,value,recId,index,
				     status);

	/* if key has been inserted then done */
	if (inserted == TRUE) 
	{
		errVal = PF_UnfixPage(fileDesc,pageNum,TRUE);
		AM_Check;
		AM_EmptyStack();
		return(AME_OK);
	}
	
	/* check if there is any error */
	if (inserted < 0) 
	{
		AM_EmptyStack();
		AM_Errno = inserted;
		return(inserted);
	}
	
	/* if not inserted then have to split */
	if (inserted == FALSE)
	{
		/* Split the leaf page */
		addtoparent = AM_SplitLeaf(fileDesc,pageBuf,&pageNum,
			     attrLength,recId,value, status,index,key);
		
		/* check for errors */
		if (addtoparent < 0) 
		{
			AM_EmptyStack();
			{
			 AM_Errno = addtoparent;
			 return(addtoparent);
                        }
		}
		
		/* if key has to be added to the parent */
		if (addtoparent == TRUE)
		{
			errVal = AM_AddtoParent(fileDesc,pageNum,key,attrLength);
			if (errVal < 0)
			{
				AM_EmptyStack();
				AM_Errno = errVal;
				return(errVal);
			}
		}
	}
	AM_EmptyStack();
	return(AME_OK);
}


/* error messages */
static char *AMerrormsg[] = {
"No error",
"Invalid Attribute Length",
"Key Not Found in Tree",
"PF error",
"Internal error - contact database manager immediately",
"Invalid scan Descriptor",
"Invalid operator to OpenIndexScan",
"Scan Over",
"Scan Table is full",
"Invalid Attribute Type",
"Invalid file Descriptor",
"Invalid value to Delete or Insert Entry"
};


AM_PrintError(s)
char *s;

{
   fprintf(stderr,"%s",s);
   fprintf(stderr,"%s",AMerrormsg[-1*AM_Errno]);
   if (AM_Errno == AME_PF)
      /* print the PF error message */
      PF_PrintError(" ");
   else 
     fprintf(stderr,"\n");
}

/* Initialize a leaf page for bulk loading */
void AM_BulkInitLeaf(char *page, int attrLength) {
    AM_LEAFHEADER header;
    int maxKeys;

    maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);

    if(maxKeys%2 != 0) {
        maxKeys--;
    }

    header.pageType = 'l';
    header.nextLeafPage = AM_NULL_PAGE;
    header.recIdPtr = PF_PAGE_SIZE;
    header.keyPtr = AM_sl;
    header.freeListPtr = AM_NULL;
    header.numinfreeList = 0;
    header.attrLength = attrLength;
    header.numKeys = 0;
    header.maxKeys = maxKeys;

    memcpy(page, &header, AM_sl);
}

/* Insert a sorted key directly into a leaf page */
int AM_BulkInsertLeaf(char *page, int attrLength,
                      char *key, int recId) {
    AM_LEAFHEADER header;
    int index;
    int status;
    int result;
    char *lastKey;

    memcpy(&header, page, AM_sl);

    index = header.numKeys + 1;
    status = AM_NOT_FOUND;

    // Check if the key already exists
    if(header.numKeys > 0) {
        lastKey = page + AM_sl +
                  (header.numKeys-1)*(attrLength + AM_ss);

        if(memcmp(lastKey, key, attrLength) == 0) {
            index = header.numKeys;
            status = AM_FOUND;
        } else if(memcmp(lastKey, key, attrLength) > 0) {
            return -1;
        }
    }

    // Insert directly without searching the B+ tree
    result = AM_InsertintoLeaf(page, attrLength,
                               key, recId, index, status);

    return result;
}

/* Connect one leaf page to the next */
int AM_BulkLinkLeaf(int fileDesc, int pageNum, int nextPageNum) {
    char *page;
    AM_LEAFHEADER header;
    int result;

    result = PF_GetThisPage(fileDesc, pageNum, &page);

    if(result != PFE_OK) {
        return -1;
    }

    memcpy(&header, page, AM_sl);

    header.nextLeafPage = nextPageNum;

    memcpy(page, &header, AM_sl);

    result = PF_UnfixPage(fileDesc, pageNum, TRUE);

    if(result != PFE_OK) {
        return -1;
    }

    return 0;
}

/* Build leaf pages using sorted roll numbers */
int AM_BulkBuildLeaves(int fileDesc, int attrLength,
                       char *keys, int *recIds, int count,
                       int *leafPages, char *firstKeys) {
    char *page;
    char *newPage;
    int pageNum;
    int newPageNum;
    int leafCount = 0;
    int result;
	int i;

    if(count <= 0) {
        return -1;
    }

    // Allocate the first leaf
    result = PF_AllocPage(fileDesc, &pageNum, &page);

    if(result != PFE_OK) {
        return -1;
    }

    AM_BulkInitLeaf(page, attrLength);

    leafPages[0] = pageNum;
    memcpy(firstKeys, keys, attrLength);
    leafCount++;

    for(i=0; i<count; i++) {
        result = AM_BulkInsertLeaf(page, attrLength,
                                   keys + i*attrLength, recIds[i]);

        if(result == TRUE) {
            continue;
        }

        if(result != FALSE) {
            PF_UnfixPage(fileDesc, pageNum, TRUE);
            return -1;
        }

        // Current leaf is full
        result = PF_UnfixPage(fileDesc, pageNum, TRUE);

        if(result != PFE_OK) {
            return -1;
        }

        // Allocate the next leaf
        result = PF_AllocPage(fileDesc, &newPageNum, &newPage);

        if(result != PFE_OK) {
            return -1;
        }

        AM_BulkInitLeaf(newPage, attrLength);

        // Save the new leaf
        result = PF_UnfixPage(fileDesc, newPageNum, TRUE);

        if(result != PFE_OK) {
            return -1;
        }

        // Connect the previous leaf to the new leaf
        result = AM_BulkLinkLeaf(fileDesc, pageNum, newPageNum);

        if(result != 0) {
            return -1;
        }

        pageNum = newPageNum;

        // Open the new leaf for insertion
        result = PF_GetThisPage(fileDesc, pageNum, &page);

        if(result != PFE_OK) {
            return -1;
        }

        leafPages[leafCount] = pageNum;

        memcpy(firstKeys + leafCount*attrLength,
               keys + i*attrLength, attrLength);

        leafCount++;

        // Insert the key that could not fit
        result = AM_BulkInsertLeaf(page, attrLength,
                                   keys + i*attrLength, recIds[i]);

        if(result != TRUE) {
            PF_UnfixPage(fileDesc, pageNum, TRUE);
            return -1;
        }
    }

    result = PF_UnfixPage(fileDesc, pageNum, TRUE);

    if(result != PFE_OK) {
        return -1;
    }

    return leafCount;
}
/* Fill an internal node using sorted child pages */
int AM_BulkFillInternal(char *page, int attrLength,
                        int *childPages, char *childKeys,
                        int childCount) {
    AM_INTHEADER header;
    int maxKeys;
    int pos;
    int i;

    maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);

    if(maxKeys%2 != 0) {
        maxKeys--;
    }

    if(childCount < 2 || childCount > maxKeys+1) {
        return -1;
    }

    header.pageType = 'i';
    header.numKeys = childCount-1;
    header.maxKeys = maxKeys;
    header.attrLength = attrLength;

    memcpy(page, &header, AM_sint);

    pos = AM_sint;

    // Store the leftmost child
    memcpy(page + pos, &childPages[0], AM_si);
    pos += AM_si;

    // Store separator keys and remaining children
    for(i=1; i<childCount; i++) {
        memcpy(page + pos, childKeys + i*attrLength, attrLength);
        pos += attrLength;

        memcpy(page + pos, &childPages[i], AM_si);
        pos += AM_si;
    }

    return 0;
}

/* Build one level of internal nodes */
int AM_BulkBuildLevel(int fileDesc, int attrLength,
                      int *childPages, char *childKeys, int childCount,
                      int *parentPages, char *parentKeys) {
    int maxKeys;
    int maxChildren;
    int parentCount;
    int groupSize;
    int extra;
    int start = 0;
    int pageNum;
    int result;
    int i;
    char *page;

    maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);

    if(maxKeys%2 != 0) {
        maxKeys--;
    }

    maxChildren = maxKeys + 1;

    // Calculate number of parent pages needed
    parentCount = (childCount + maxChildren-1)/maxChildren;

    groupSize = childCount/parentCount;
    extra = childCount%parentCount;

    for(i=0; i<parentCount; i++) {
        int currentSize = groupSize;

        if(i < extra) {
            currentSize++;
        }

        // Allocate a new internal page
        result = PF_AllocPage(fileDesc, &pageNum, &page);

        if(result != PFE_OK) {
            return -1;
        }

        // Fill the internal page
        result = AM_BulkFillInternal(page, attrLength,
                                     childPages + start,
                                     childKeys + start*attrLength,
                                     currentSize);

        if(result != 0) {
            PF_UnfixPage(fileDesc, pageNum, FALSE);
            return -1;
        }

        // Store parent page number
        parentPages[i] = pageNum;

        // Store the smallest key in this subtree
        memcpy(parentKeys + i*attrLength,
               childKeys + start*attrLength, attrLength);

        result = PF_UnfixPage(fileDesc, pageNum, TRUE);

        if(result != PFE_OK) {
            return -1;
        }

        start += currentSize;
    }

    return parentCount;
}

/* Build a B+ tree index using sorted records */
int AM_BulkLoad(int fileDesc, char attrType, int attrLength,
                char *keys, int *recIds, int count) {
    int *childPages;
    int *parentPages;
    char *childKeys;
    char *parentKeys;
    char *page;
    char *rootPage;

    int pageNum;
    int childCount;
    int parentCount;
    int rootNum;
    int result;
    int i;
    int status = -1;

    AM_LEAFHEADER header;

    if(fileDesc < 0 || attrType != 'c' || attrLength < 1 ||
       attrLength > 255 || count <= 0 ||
       keys == NULL || recIds == NULL) {
        return -1;
    }

    // Check that the index is empty
    result = PF_GetThisPage(fileDesc, 0, &page);

    if(result != PFE_OK) {
        return -1;
    }

    memcpy(&header, page, AM_sl);

    result = PF_UnfixPage(fileDesc, 0, FALSE);

    if(result != PFE_OK) {
        return -1;
    }

    if(header.pageType != 'l' || header.numKeys != 0) {
        return -1;
    }

    childPages = malloc(count*sizeof(int));
    parentPages = malloc(count*sizeof(int));
    childKeys = malloc(count*attrLength);
    parentKeys = malloc(count*attrLength);

    if(childPages == NULL || parentPages == NULL ||
       childKeys == NULL || parentKeys == NULL) {
        goto cleanup;
    }

    // Reserve page 1
    result = PF_AllocPage(fileDesc, &pageNum, &page);

    if(result != PFE_OK) {
        goto cleanup;
    }

    result = PF_UnfixPage(fileDesc, pageNum, FALSE);

    if(result != PFE_OK || pageNum != 1) {
        goto cleanup;
    }

    // Build leaf pages starting from page 2
    childCount = AM_BulkBuildLeaves(fileDesc, attrLength,
                                    keys, recIds, count,
                                    childPages, childKeys);

    if(childCount <= 0) {
        goto cleanup;
    }

    // Build internal levels bottom-up
    while(childCount > 1) {
        parentCount = AM_BulkBuildLevel(fileDesc, attrLength,
                                        childPages, childKeys,
                                        childCount,
                                        parentPages, parentKeys);

        if(parentCount <= 0) {
            goto cleanup;
        }

        childCount = parentCount;

        for(i=0; i<childCount; i++) {
            childPages[i] = parentPages[i];

            memcpy(childKeys + i*attrLength,
                   parentKeys + i*attrLength,
                   attrLength);
        }
    }

    // Copy the final root into page 0
    rootNum = childPages[0];

    result = PF_GetThisPage(fileDesc, rootNum, &rootPage);

    if(result != PFE_OK) {
        goto cleanup;
    }

    result = PF_GetThisPage(fileDesc, 0, &page);

    if(result != PFE_OK) {
        PF_UnfixPage(fileDesc, rootNum, FALSE);
        goto cleanup;
    }

    memcpy(page, rootPage, PF_PAGE_SIZE);

    result = PF_UnfixPage(fileDesc, rootNum, FALSE);

    if(result != PFE_OK) {
        PF_UnfixPage(fileDesc, 0, TRUE);
        goto cleanup;
    }

    result = PF_UnfixPage(fileDesc, 0, TRUE);

    if(result != PFE_OK) {
        goto cleanup;
    }

    AM_RootPageNum = 0;

    status = 0;

cleanup:
    free(childPages);
    free(parentPages);
    free(childKeys);
    free(parentKeys);

    return status;
}
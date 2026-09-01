#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int nextBlockNum = ATTRCAT_BLOCK;
    while (nextBlockNum != -1) {
      unsigned char tempBuffer[BLOCK_SIZE];
      disk_run.readBlock(tempBuffer, nextBlockNum);

      int prevBlockNum = nextBlockNum;
      attrCatBuffer = RecBuffer(nextBlockNum);
      attrCatBuffer.getHeader(&attrCatHeader);
      nextBlockNum = attrCatHeader.rblock;

      for (int j = 0; j < attrCatHeader.numEntries; j++) {

        // declare attrCatRecord and load the attribute catalog entry into it
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
        
        // if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
        //   if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0) {
        //     memcpy(&attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch", 6);
        //     memcpy(tempBuffer + HEADER_SIZE + attrCatHeader.numSlots + (j * ATTRCAT_NO_ATTRS * ATTR_SIZE), attrCatRecord, ATTRCAT_NO_ATTRS * ATTR_SIZE);
        //     disk_run.writeBlock(tempBuffer, prevBlockNum);
        //   }
        // }
        
        attrCatBuffer.getRecord(attrCatRecord, j);
        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }

      }
    }
    
    printf("\n");
  }

  return 0;
}
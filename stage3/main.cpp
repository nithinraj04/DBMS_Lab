#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  for(int relId = 0; relId <= 1; relId++) { // loop through RELCAT_RELID and ATTRCAT_RELID
    RelCatEntry relCatEntry;

    if (RelCacheTable::getRelCatEntry(relId, &relCatEntry) != SUCCESS) {
      std::cerr << "Error retrieving relation catalog entry for relId " << relId << std::endl;
      return 1; // return error if relation catalog entry cannot be retrieved
    }

    std::cout << "Relation: " << relCatEntry.relName << std::endl;

    for (int j = 0; j < relCatEntry.numAttrs; j++) {
      AttrCatEntry attrCatEntry;
      if (AttrCacheTable::getAttrCatEntry(relId, j, &attrCatEntry) != SUCCESS) {
        std::cerr << "Error retrieving attribute catalog entry for relId " << relId << " and attrOffset " << j << std::endl;
        return 1;
      }

      if(attrCatEntry.attrType == NUMBER) {
        printf("  %s: NUM\n", attrCatEntry.attrName);
      }
      else {
        printf("  %s: STR\n", attrCatEntry.attrName);
      }
    }
  }

  for(int i = 0; i < MAX_OPEN; i++) {
    RelCatEntry relCatEntry;

    if (RelCacheTable::getRelCatEntry(i, &relCatEntry) == SUCCESS) {
      if(strcmp(relCatEntry.relName, "Students") == 0) {
        std::cout << "Relation: " << relCatEntry.relName << std::endl;
      }

      for (int j = 0; j < relCatEntry.numAttrs; j++) {
        AttrCatEntry attrCatEntry;
        if (AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry) == SUCCESS) {
          if(attrCatEntry.attrType == NUMBER) {
            printf("  %s: NUM\n", attrCatEntry.attrName);
          }
          else {
            printf("  %s: STR\n", attrCatEntry.attrName);
          }
        }
      }
    }
  }

  return 0;
}
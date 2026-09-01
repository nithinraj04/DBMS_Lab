#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>

OpenRelTable::OpenRelTable() {

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
    // (we need to populate relation cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    
    struct RelCacheEntry relCacheEntry;

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // allocate this on the heap because we want it to persist outside this function
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    // allocate this on the heap because we want it to persist outside this function
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

    /************ Setting up Attribute cache entries ************/
    // (we need to populate attribute cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *prev = nullptr;

    for (int i = 0; i < ATTRCAT_NO_ATTRS; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry *newEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = i;
        newEntry->next = nullptr;
        if (prev != nullptr) {
            prev->next = newEntry;
        }
        else {
            head = newEntry; // first entry becomes the head of the linked list
        }
        prev = newEntry;
    }

    // iterate through all the attributes of the relation catalog and create a linked
    // list of AttrCacheEntry (slots 0 to 5)
    // for each of the entries, set
    //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
    //    attrCacheEntry.recId.slot = i   (0 to 5)
    //    and attrCacheEntry.next appropriately
    // NOTE: allocate each entry dynamically using malloc

    // set the next field in the last entry to nullptr

    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    head = nullptr;
    prev = nullptr;
    for (int i = 6; i < 6 + ATTRCAT_NO_ATTRS; i++)
    {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry *newEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = i;
        newEntry->next = nullptr;
        if (prev != nullptr) {
            prev->next = newEntry;
        }
        else {
            head = newEntry; // first entry becomes the head of the linked list
        }
        prev = newEntry;
    }

    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    // set up the attributes of the attribute cache similarly.
    // read slots 6-11 from attrCatBlock and initialise recId appropriately

    // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]


    // assignment
    HeadInfo headInfo;
    relCatBlock.getHeader(&headInfo);
    for (int i = 0; i < headInfo.numEntries; i++) {
        relCatBlock.getRecord(relCatRecord, i);
        if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
            RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
            relCacheEntry.recId.block = RELCAT_BLOCK;
            relCacheEntry.recId.slot = i;
            RelCacheTable::relCache[i] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
            *(RelCacheTable::relCache[i]) = relCacheEntry;

            // setup attribute catalog entires in cache
            int rblock = ATTRCAT_BLOCK;
            head = nullptr;
            prev = nullptr;
            do{
                RecBuffer attrCatBlock(rblock);
                attrCatBlock.getHeader(&headInfo);
                rblock = headInfo.rblock;
                for(int j = 0; j < headInfo.numEntries; j++) {
                    attrCatBlock.getRecord(attrCatRecord, j);
                    if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
                        AttrCacheEntry *newEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
                        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
                        newEntry->recId.block = ATTRCAT_BLOCK;
                        newEntry->recId.slot = j;
                        newEntry->next = nullptr;
                        if(prev != nullptr) {
                            prev->next = newEntry;
                        } else {
                            head = newEntry; // first entry becomes the head of the linked list
                        }
                        prev = newEntry;
                    }
                }
            } while(rblock != -1);

            AttrCacheTable::attrCache[i] = head;

            break;
        }
    }
}

OpenRelTable::~OpenRelTable()
{
    // free all the memory that you allocated in the constructor
    free(RelCacheTable::relCache[RELCAT_RELID]);
    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    free(AttrCacheTable::attrCache[RELCAT_RELID]);
    free(AttrCacheTable::attrCache[ATTRCAT_RELID]);
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    for (int i = 0; i < MAX_OPEN; i++) {
        if (RelCacheTable::relCache[i] != nullptr) {
            if (strcmp(RelCacheTable::relCache[i]->relCatEntry.relName, relName) == 0) {
                return i;
            }
        }
    }
    return E_RELNOTOPEN;
}
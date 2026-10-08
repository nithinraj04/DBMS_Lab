#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>

char REL_NAME[ATTR_SIZE] = "RelName";

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
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


   // setup tableMetaInfo entries for the catalogs
   tableMetaInfo[RELCAT_RELID].free = false;
   strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
   tableMetaInfo[ATTRCAT_RELID].free = false;
   strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable()
{
    // close the tables
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
            OpenRelTable::closeRel(i);
        }
    }

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

int OpenRelTable::getFreeOpenRelTableEntry() {
    for (int i = 2; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    int relId = getRelId(relName);
    if (relId != E_RELNOTOPEN) {
        return relId;
    }

    int freeEntry = getFreeOpenRelTableEntry();
    if (freeEntry == E_CACHEFULL) {
        return E_CACHEFULL;
    }
    
    relId = freeEntry;
    Attribute relNameAttrVal;
    strcpy(relNameAttrVal.sVal, relName);

    RelCacheTable::resetSearchIndex(RELCAT_RELID); // reset search index before searching (always ig)
    RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, REL_NAME, relNameAttrVal, EQ);

    if(relCatRecId.block == -1 && relCatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    // populate relCache
    RecBuffer relCatBlock(relCatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    struct RelCacheEntry relCacheEntry;

    relCatBlock.getRecord(relCatRecord, relCatRecId.slot);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId = relCatRecId;

    RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[relId]) = relCacheEntry;

    // populate attrCache
    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *prev = nullptr;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrCatRecId;
    struct AttrCacheEntry attrCacheEntry;

    while (true) {
        attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, REL_NAME, relNameAttrVal, EQ);
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1) {
            break;
        }
        // Process the found attribute record
        RecBuffer attrCatBlock(attrCatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId = attrCatRecId;

        if (prev != nullptr) {
            prev->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            *(prev->next) = attrCacheEntry;
            prev = prev->next;
            prev->next = nullptr; // ensure the next pointer of the last entry is null
        } else {
            head = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            *head = attrCacheEntry;
            head->next = nullptr; // ensure the next pointer of the first entry is null
            prev = head;
        }
    }

    AttrCacheTable::attrCache[relId] = head;

    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }

    if(RelCacheTable::relCache[relId]->dirty) {
        RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCacheEntry->relCatEntry, relCatRecord);
        
        RecBuffer relCatBlock(relCacheEntry->recId.block);
        relCatBlock.setRecord(relCatRecord, relCacheEntry->recId.slot);
    }

    // free the memory allocated in the relation and attribute caches which was
    // allocated in the OpenRelTable::openRel() function

    // update `tableMetaInfo` to set `relId` as a free slot
    // update `relCache` and `attrCache` to set the entry at `relId` to nullptr

    free(RelCacheTable::relCache[relId]);
    AttrCacheEntry *current = AttrCacheTable::attrCache[relId];
    while (current != nullptr) {
        AttrCacheEntry *next = current->next;
        free(current);
        current = next;
    }
    tableMetaInfo[relId].free = true;
    memset(tableMetaInfo[relId].relName, '\0', ATTR_SIZE);
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;
}


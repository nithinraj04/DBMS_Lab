#include "BlockAccess.h"
#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(
    int relId, 
    char attrName[ATTR_SIZE], 
    union Attribute attrVal, 
    int op
) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // let block and slot denote the record id of the record being currently checked
    int block = -1;
    int slot = -1;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;  // start from the first slot of the block

        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        // slot = search index's slot + 1

        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer recBuffer(block);
        
        HeadInfo header;
        recBuffer.getHeader(&header);

        Attribute record[header.numAttrs];
        recBuffer.getRecord(record, slot);

        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);

        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if (slot >= header.numSlots)
        {
            // update block = right block of block
            // update slot = 0
            block = header.rblock;
            slot = 0;
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        /* use the attribute offset to get the value of the attribute from
           current record */
        
        int attrOffset;
        AttrCatEntry attrCatEntry;
        if (AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry) != SUCCESS) {
            std::cout << "Attribute " << attrName << " does not exist in relation with relId " << relId << std::endl;
            return RecId{-1, -1};
        }
        attrOffset = attrCatEntry.offset;

        Attribute recordAttrVal = record[attrOffset];

        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        cmpVal = compareAttrs(recordAttrVal, attrVal, attrCatEntry.attrType);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId searchIndex = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);
            return searchIndex;  // return the record id of the record that satisfies the given condition
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    // check if any relation with newRelName already exists 
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelNameAttr;
    strcpy(newRelNameAttr.sVal, newRelName);
    RecId recId;
    recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelNameAttr, EQ);

    if (recId.block != -1 && recId.slot != -1) {
        return E_RELEXIST;  // new relation name already exists
    }

    
    // check if a relation with oldRelName exists
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelNameAttr;
    strcpy(oldRelNameAttr.sVal, oldRelName);
    recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelNameAttr, EQ);

    if (recId.block == -1 && recId.slot == -1) {
        return E_RELNOTEXIST;  // old relation name does not exist
    }

    // update relation catalog entry
    RecBuffer recBuffer(recId.block);
    Attribute relCatEntry[RELCAT_NO_ATTRS];
    recBuffer.getRecord(relCatEntry, recId.slot);
    strcpy(relCatEntry[RELCAT_REL_NAME_INDEX].sVal, newRelName);
    recBuffer.setRecord(relCatEntry, recId.slot);

    // update attribute catalog entries
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    Attribute attrCatEntry[ATTRCAT_NO_ATTRS];
    while (true) {
        recId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelNameAttr, EQ);
        if (recId.block == -1 && recId.slot == -1) {
            break;  // no more attribute catalog entries for oldRelName
        }
        // update the attribute catalog entry
        RecBuffer attrBuffer(recId.block);
        attrBuffer.getRecord(attrCatEntry, recId.slot);
        strcpy(attrCatEntry[ATTRCAT_REL_NAME_INDEX].sVal, newRelName);
        attrBuffer.setRecord(attrCatEntry, recId.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE]) {
    // check if a relation with relName exists
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    RecId recId;
    recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    if (recId.block == -1 && recId.slot == -1) {
        return E_RELNOTEXIST;  // relation does not exist
    }

    // check if attribute with oldAttrName exists for this relation
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    Attribute oldAttrNameAttr;
    strcpy(oldAttrNameAttr.sVal, oldAttrName);
    Attribute newAttrNameAttr;
    strcpy(newAttrNameAttr.sVal, newAttrName);
    RecId attrRecId;
    while (true) {
        recId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if (recId.block == -1 && recId.slot == -1) {
            break;
        }

        RecBuffer attrBuffer(recId.block);
        Attribute attrCatEntry[ATTRCAT_NO_ATTRS];
        attrBuffer.getRecord(attrCatEntry, recId.slot);

        if (strcasecmp(attrCatEntry[ATTRCAT_ATTR_NAME_INDEX].sVal, oldAttrName) == 0) {
            attrRecId = recId;
            break;
        }

        if (strcasecmp(attrCatEntry[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName) == 0) {
            return E_ATTREXIST;  // new attribute name already exists
        }
    }

    if (attrRecId.block == -1 && attrRecId.slot == -1) {
        return E_ATTRNOTEXIST;  // old attribute name does not exist
    }

    RecBuffer oldAttrBuffer(attrRecId.block);
    Attribute oldAttrCatEntry[ATTRCAT_NO_ATTRS];
    oldAttrBuffer.getRecord(oldAttrCatEntry, attrRecId.slot);
    strcpy(oldAttrCatEntry[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName);
    oldAttrBuffer.setRecord(oldAttrCatEntry, attrRecId.slot);

    return SUCCESS;
}
    
int BlockAccess::insert(int relId, union Attribute *record) {
    // get the relation catalog entry for the relation with relId
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    RecId recId = {-1, -1}; // store where new rec will be inserted

    int numOfSlotsPerBlock = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1; 

    while (blockNum != -1) {
        RecBuffer recBuffer(blockNum);

        HeadInfo header;
        recBuffer.getHeader(&header);

        unsigned char slotMap[numOfSlotsPerBlock];
        recBuffer.getSlotMap(slotMap);

        for (int slot = 0; slot < numOfSlotsPerBlock; ++slot) {
            if (slotMap[slot] == SLOT_UNOCCUPIED) {
                recId.block = blockNum;
                recId.slot = slot;
                break;
            }
        }

        if (recId.block != -1 && recId.slot != -1) {
            break; // found a free slot
        }

        prevBlockNum = blockNum;
        blockNum = header.rblock; // Move to the next block
    }

    if(recId.block == -1 && recId.slot == -1) {
        if (strcmp(relCatEntry.relName, RELCAT_RELNAME) == 0) {
            return E_MAXRELATIONS;  // Maximum number of relations already present
        }

        // use constructor of RecBuffer with no args to get a new block
        RecBuffer newRecBuffer;
        int ret = newRecBuffer.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL; 
        }
        
        recId.block = ret;
        recId.slot = 0;

        // Set header for the new block
        HeadInfo newHeader;
        newHeader.blockType = REC;
        newHeader.pblock = -1;
        newHeader.lblock = prevBlockNum;
        newHeader.rblock = -1;
        newHeader.numEntries = 0;
        newHeader.numAttrs = numOfAttributes;
        newHeader.numSlots = numOfSlotsPerBlock;
        newRecBuffer.setHeader(&newHeader);

        // set the slot map
        unsigned char newSlotMap[numOfSlotsPerBlock];
        memset(newSlotMap, SLOT_UNOCCUPIED, numOfSlotsPerBlock);
        newRecBuffer.setSlotMap(newSlotMap);

        // set the right block of the previous block to this new block
        if (prevBlockNum != -1) {
            RecBuffer prevRecBuffer(prevBlockNum);
            HeadInfo prevHeader;
            prevRecBuffer.getHeader(&prevHeader);
            prevHeader.rblock = recId.block;
            prevRecBuffer.setHeader(&prevHeader);
        }
        else {
            // This is the first block for this relation, update the relation catalog entry
            relCatEntry.firstBlk = recId.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in relation catalog entry
        relCatEntry.lastBlk = recId.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    RecBuffer recBuffer(recId.block);
    recBuffer.setRecord(record, recId.slot);

    // update the slot map to mark the slot as occupied
    unsigned char slotMap[numOfSlotsPerBlock];
    recBuffer.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    // update the number of entries in the block header
    HeadInfo header;
    recBuffer.getHeader(&header);
    header.numEntries++;
    recBuffer.setHeader(&header);

    // update the number of records in the relation catalog entry
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}


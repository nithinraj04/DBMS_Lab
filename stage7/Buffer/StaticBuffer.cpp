#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].timeStamp = -1;
        metainfo[i].blockNum = -1;
    }
}

StaticBuffer::~StaticBuffer() {
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if(!metainfo[i].free && metainfo[i].dirty) {
            // write the block back to disk
            // (use Disk::writeBlock() function)
            Disk::writeBlock(blocks[i], metainfo[i].blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    // increment timestamp of all blocks in buffer
    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(!metainfo[i].free) {
            metainfo[i].timeStamp++;
        }
    }

    int allocatedBuffer = -1;
    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(metainfo[i].free) {
            allocatedBuffer = i;
            metainfo[i].free = false;
            metainfo[i].blockNum = blockNum;
            break;
        }
    }

    if (allocatedBuffer == -1) {
        // buffer is full, need to evict a block
        int maxTimeStamp = -1;
        for(int i = 0; i < BUFFER_CAPACITY; i++) {
            if(metainfo[i].timeStamp > maxTimeStamp) {
                maxTimeStamp = metainfo[i].timeStamp;
                allocatedBuffer = i;
            }
        }

        // if the block to be evicted is dirty, write it back to disk
        if(metainfo[allocatedBuffer].dirty) {
            Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
        }
    }

    // update the metadata for the newly allocated buffer
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum) {
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(metainfo[i].blockNum == blockNum) {
            return i;
        }
    }
    
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum) {
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(metainfo[i].blockNum == blockNum) {
            metainfo[i].dirty = true;
            return SUCCESS;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

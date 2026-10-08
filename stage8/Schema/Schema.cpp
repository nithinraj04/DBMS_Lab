#include "Schema.h"
#include <cmath>
#include <cstring>
#include <iostream>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (
    strcasecmp(relName, RELCAT_RELNAME) == 0 || strcasecmp(relName, ATTRCAT_RELNAME) == 0
  ) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
  if (
    strcasecmp(oldRelName, RELCAT_RELNAME) == 0 || strcasecmp(oldRelName, ATTRCAT_RELNAME) == 0
  ) {
    return E_NOTPERMITTED;
  }

  if (OpenRelTable::getRelId(oldRelName) != E_RELNOTOPEN) {
    return E_RELOPEN;  // can't rename if relation is open
  }

  return BlockAccess::renameRelation(oldRelName, newRelName);
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE]) {
  if (
    strcasecmp(relName, RELCAT_RELNAME) == 0 || strcasecmp(relName, ATTRCAT_RELNAME) == 0
  ) {
    return E_NOTPERMITTED;
  }

  if (OpenRelTable::getRelId(relName) != E_RELNOTOPEN) {
    return E_RELOPEN;  // can't rename if relation is open
  }

  return BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
}

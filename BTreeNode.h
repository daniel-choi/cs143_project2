/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 5/28/2008
 */

#ifndef BTNODE_H
#define BTNODE_H

#include "RecordFile.h"
#include "PageFile.h"
#include "math.h"
/**
 * BTLeafNode: The class representing a B+tree leaf node.
 */
const int g_leafEntrySize = sizeof(int) + sizeof(RecordId);
const int g_maxKeyCount = 85;
class BTLeafNode {
public:
    BTLeafNode () : keyCount(0) {}
    RC insert(int key, const RecordId& rid);
    RC insertAndSplit(int key, const RecordId& rid, BTLeafNode& sibling, int& siblingKey);
    RC locate(int searchKey, int& eid);
    RC readEntry(int eid, int& key, RecordId& rid);
    PageId getNextNodePtr();
    RC setNextNodePtr(PageId pid);
    int getKeyCount();
    RC read(PageId pid, const PageFile& pf);
    RC write(PageId pid, PageFile& pf);
    
private:
    /**
     * The main memory buffer for loading the content of the disk page 
     * that contains the node.
     */
    char buffer[PageFile::PAGE_SIZE];
    int keyCount;
    int insertToBuffer(const int key, const RecordId rid, const int eid);
    bool checkFull();
    int shift(const int eid);
}; 


/**
 * BTNonLeafNode: The class representing a B+tree nonleaf node.
 */
class BTNonLeafNode {
public:
    /**
     * Insert a (key, pid) pair to the node.
     * Remember that all keys inside a B+tree node should be kept sorted.
     * @param key[IN] the key to insert
     * @param pid[IN] the PageId to insert
     * @return 0 if successful. Return an error code if the node is full.
     */
    RC insert(int key, PageId pid);
    
    /**
     * Insert the (key, pid) pair to the node
     * and split the node half and half with sibling.
     * The sibling node MUST be empty when this function is called.
     * The middle key after the split is returned in midKey.
     * Remember that all keys inside a B+tree node should be kept sorted.
     * @param key[IN] the key to insert
     * @param pid[IN] the PageId to insert
     * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
     * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey);
    
    /**
     * Given the searchKey, find the child-node pointer to follow and
     * output it in pid.
     * Remember that the keys inside a B+tree node are sorted.
     * @param searchKey[IN] the searchKey that is being looked up.
     * @param pid[OUT] the pointer to the child node to follow.
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC locateChildPtr(int searchKey, PageId& pid);
    
    /**
     * Initialize the root node with (pid1, key, pid2).
     * @param pid1[IN] the first PageId to insert
     * @param key[IN] the key that should be inserted between the two PageIds
     * @param pid2[IN] the PageId to insert behind the key
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC initializeRoot(PageId pid1, int key, PageId pid2);
    
    /**
     * Return the number of keys stored in the node.
     * @return the number of keys in the node
     */
    int getKeyCount();
    
    /**
     * Read the content of the node from the page pid in the PageFile pf.
     * @param pid[IN] the PageId to read
     * @param pf[IN] PageFile to read from
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC read(PageId pid, const PageFile& pf);
    
    /**
     * Write the content of the node to the page pid in the PageFile pf.
     * @param pid[IN] the PageId to write to
     * @param pf[IN] PageFile to write to
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC write(PageId pid, PageFile& pf);
    
private:
    /**
     * The main memory buffer for loading the content of the disk page 
     * that contains the node.
     */
    char buffer[PageFile::PAGE_SIZE];
}; 

#endif /* BTNODE_H */

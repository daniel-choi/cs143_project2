/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight=0;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
	RC rc;
	if ((rc = pf.open(indexname, mode)) < 0) {
    	return rc;
 	}
	return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
	RC rc;
	if ((rc = pf.close()) < 0) {
    	return rc;
 	}
    return 0;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
	if(treeHeight == 0) {
		createRoot(key, rid);
		treeHeight++;
	}
	PageId siblingPid;
	int newRootKey;
	if((newRootKey = insertionHelper(key, rid, 1, rootPid, siblingPid)) != 0){
		//New root Node
		BTNonLeafNode *newRoot = new BTNonLeafNode;
		newRoot -> initializeRoot(rootPid, newRootKey, siblingPid);
		rootPid = pf.endPid();
		newRoot -> write(rootPid, pf);
		treeHeight++;
	}

    return 0;
}


RC BTreeIndex::createRoot(const int key, const RecordId &rid)
{
	BTNonLeafNode *root = new BTNonLeafNode;
	rootPid = pf.endPid();
	BTLeafNode *ln1 = new BTLeafNode;
	BTLeafNode *ln2 = new BTLeafNode;
	ln2->insert(key, rid);
	root->initializeRoot(1, key, 2);
	ln1->setNextNodePtr(2);
	root->write(rootPid, pf);
	ln1->write(1, pf);
	ln2->write(2, pf);
}

int BTreeIndex::insertionHelper(const int key, const RecordId &rid, int n, PageId pid, PageId &siblingPid)
{
	// Not eqaul to tree height meaning we are in the NonLeafNode.
	if(n != treeHeight) {
		PageId tempPid;
		BTNonLeafNode *tempNode = new BTNonLeafNode;
		tempNode -> read(pid, pf);
		tempNode -> locateChildPtr(key, tempPid);
		int siblingKey;
		if(siblingKey = insertionHelper(key, rid, n+1, tempPid, siblingPid) != 0)
		{
			if((tempNode -> insert(siblingKey, siblingPid)) < 0) {
				int midKey;
				BTNonLeafNode *siblingNode = new BTNonLeafNode;
				tempNode -> insertAndSplit (siblingKey, siblingPid, *siblingNode, midKey);
				siblingPid = pf.endPid();
				siblingNode -> write(siblingPid, pf);

				return midKey;
			}
			else
				return 0;
		}

	}
	// Eqaul to tree height meaning we are in the LeafNode. Insert key and rid in the leafNode and check if it overflows.
	// If it overflows, return siblingKey to insert in the parentNode.
	// If not return 0.
	else if (n == treeHeight){
		BTLeafNode *tempNode = new BTLeafNode;
		tempNode -> read(pid, pf);
		if((tempNode -> insert(key, rid)) < 0) {
			BTLeafNode *siblingNode = new BTLeafNode;
			int siblingKey;
			tempNode -> insertAndSplit (key, rid, *siblingNode, siblingKey);
			siblingPid = pf.endPid();
			tempNode -> setNextNodePtr(siblingPid);
			siblingNode -> write(siblingPid, pf);

			return siblingKey;
		}
		else
			return 0;
	}




}

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
	RC rc;
	BTLeafNode *leafNode = new BTLeafNode;
	leafNode -> read(cursor.pid, pf);
	if(rc = (leafNode -> readEntry(cursor.eid, key, rid)) < 0)
		return rc;
	IndexCursor nextCursor;
    return 0;
}

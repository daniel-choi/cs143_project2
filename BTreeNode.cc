#include "BTreeNode.h"

using namespace std;





////////////////////////////////////////////////////////////////////////////////
//                            BTLeafNode Implementation                       //
////////////////////////////////////////////////////////////////////////////////

/*
 * LeafNode Buffer
 * Equivalent to int buffer[256];
 _______________________________________________________________
 |  0   |  1   |   2  |   3  |   4  |   5  |   6  | ...  | 255  |
 | key  | pid  | sid  |  key | pid  |  sid | key  | ...  | pid  |
 |______|______|______|______|______|______|______|______|______|

*/

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
  RC  rc;
  if ((rc = pf.read(pid, buffer)) < 0) {
    return rc;
  }

  return 0; }

  /*
   * Write the content of the node to the page pid in the PageFile pf.
   * @param pid[IN] the PageId to write to
   * @param pf[IN] PageFile to write to
   * @return 0 if successful. Return an error code if there is an error.
   */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{ 
  RC rc;
  if ((rc = pf.write(pid, buffer)) < 0)
    return rc;

  return 0; 
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
  return keyCount; 
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{

  RC rc;
  if(checkFull())
    return RC_NODE_FULL;
  // Set eid to last buffer
  int eidCandidate = keyCount;
  for (int i = 0; i < keyCount; i++) {
    int readKey;
    RecordId readRid;
    // Read entry i
    if(rc = readEntry(i, readKey, readRid) < 0)
      return rc;
    // If read entry key is greater than the i's key, new entry needs to put before i entry.
    if(readKey >= key) {
      eidCandidate = i;
      break;
    }
  }
  // If new key is the biggest, put at the end.
  // If not shift all the entires by one and put it in the eidCandidate
  if(eidCandidate != keyCount)
    shift(eidCandidate);
  if(rc = insertToBuffer(key, rid, eidCandidate) < 0)
    return rc;

  // Update key count
  keyCount ++;

  return 0; 
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
    BTLeafNode& sibling, int& siblingKey)
{
  RC rc;
  int numMove = keyCount/2;
  int numStay = keyCount - numMove;

  if (rc = insert(key, rid) <0)
    return rc;

  for (int i = keyCount-1; i >= numStay; i--)
  {
    int readKey;
    RecordId readRid;
    if(rc = readEntry(i, readKey, readRid) < 0)
      return rc;
    if (rc = sibling.insert(readKey, readRid) < 0)
      return rc;
    if (rc = deleteFromBuffer(i) < 0)
      return rc;
  }

  RecordId readRid;
  if (rc = sibling.readEntry(0, siblingKey, readRid) < 0)
    return rc;
  return 0; 
}

  /*
   * Find the entry whose key value is larger than or equal to searchKey
   * and output the eid (entry number) whose key value >= searchKey.
   * Remeber that all keys inside a B+tree node should be kept sorted.
   * @param searchKey[IN] the key to search for
   * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
   * @return 0 if successful. Return an error code if there is an error.
   */
RC BTLeafNode::locate(int searchKey, int& eid)
{ 
  RC rc; 
  // If there is no entry or if eid is not valid, or if eid is larger than key count
  // Return error
  if (keyCount <=0 || eid < 0 || eid >= keyCount)
    return RC_INVALID_CURSOR;

  // Loop through entries and search for the key that are greater or equal to searchKey
  // Entries are sorted so we can avoid leftovers if we found the key
  for(int i = 0; i < keyCount; i++){
    int keyEntry;
    RecordId ridEntry;
    if (rc = readEntry(i, keyEntry, ridEntry) <0)
      return rc;
    if(keyEntry >= searchKey) {
      eid = i;
      return 0;
    }
  }
  return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ 
  // If there is no entry or if eid is not valid, or if eid is larger than key count
  // Return error
  if(keyCount <= 0 || eid < 0 || eid >= keyCount)
    return RC_INVALID_CURSOR;

  // Convert buffer pointer from char to int
  int *intBufferPtr = (int *) buffer;

  // Read from the buffer
  int index = eid * 3;
  key = *(intBufferPtr + index);
  rid.pid = *(intBufferPtr + index + 1);
  rid.sid = *(intBufferPtr + index + 2);

  return 0; 
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 
  int *intBufferPtr = (int *)buffer;

  // Return the last element from the buffer which is PageId of the next sibling node
  return (*(intBufferPtr + 255));
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ 
  int *intBufferPtr = (int *)buffer;
  // Set last element of the buffer to the PageId of next sibling node
  *(intBufferPtr + 255) = pid;
  return 0; 
}



////////////////////////////////////////////////////////////////////////////////
//                            BTLeafNode Helper Functions                     //
////////////////////////////////////////////////////////////////////////////////

//Convert integer to dynamic array of characters. 
//Return 0 if success -1 otherwise
RC BTLeafNode::insertToBuffer(const int key, const RecordId rid, const int eid)
{
  if(checkFull())
    return RC_NODE_FULL;
  int* intBufferPtr = (int*) buffer;
  int index = eid*3;
  *(intBufferPtr + index) = key;
  *(intBufferPtr + index + 1) = rid.pid;
  *(intBufferPtr + index + 2) = rid.sid;
  return 0;
}
bool BTLeafNode::checkFull()
{
  //Max key count, excluding last entry of the leaf node (page id)
  if(keyCount >= g_maxKeyCount)
    return true;
  return false;

}
//Shifts all the elements from eid by one entry
RC BTLeafNode::shift(const int eid)
{
  RC rc;
  int * intBufferPtr = (int *)buffer;
  for(int i = keyCount-1; i >= eid; i--) 
  {
    int readKey;
    RecordId readRid;
    // Read entry for eid = i
    if(rc = readEntry(i, readKey, readRid) < 0)
      return rc;
    // Insert to buffer for eid = i+1
    if(rc = insertToBuffer(readKey, readRid, i+1) < 0)
      return rc;
  }
  return 0;

}

RC BTLeafNode::deleteFromBuffer(const int eid)
{
  RC rc;
  if(eid >= keyCount)
    return RC_NO_SUCH_RECORD;

  int* intBufferPtr = (int*) buffer;
  int index = eid*3;
  *(intBufferPtr + index) = -1;
  *(intBufferPtr + index + 1) = -1;
  *(intBufferPtr + index + 2) = -1;
  return 0;

}


////////////////////////////////////////////////////////////////////////////////
//                            BTNonLeafNode Implementation                    //
////////////////////////////////////////////////////////////////////////////////

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 
  RC rc;
  if ((rc = pf.read(pid, buffer)) < 0)
    return rc;
  else
    return RC_FILE_READ_FAILED;
  
  return 0;
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ 
  RC rc;
  if ((rc = pf.write(pid, buffer)) < 0)
    return rc;
  else
    return RC_FILE_WRITE_FAILED;

  return 0; 
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ 
  return keyCount;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ return 0; }

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ return 0; }

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ return 0; }

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ return 0; }



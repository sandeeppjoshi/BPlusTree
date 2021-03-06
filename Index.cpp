/*
 * BPlus Tree Code
 * Sandeep Joshi 113050022
 * Nikhil Patil 113059004
 * github: git://github.com/nikpatil/BPlusTree.git  or git://github.com/sandeeppjoshi/BPlusTree.git
 *
 */

#include "TreeNode.h"
#include "FileHandler.h"
#include "Utils.h"

int keylen(KeyType *keytype){
	int len=0;
	for (int i=0;i<keytype->numAttrs; i++) {
		len += keytype->attrLen[i];
	}
	return len;
}
/*----------------------------------------------------------------------------------------------------------------------------
 * Index class contains main functions like lookup and insert.
 * Data Representation:
 * Node contains keys and payload arranged as follows:
 * Header,keys,payload
 * Keys and payload is arraged such that there is free space in the middle.This is done to avoid extra shifting.
 */

class Index{
public:
	FileHandler *fHandler;
	TreeNode *root;
	char rootAddress[NODE_OFFSET_SIZE];
	KeyType keytype;
	int payloadlen;
	class Utils *utils;
	char *header;
	int node_address_size;

	~Index(){

	}
/*
 *Construnctor for new Index
 */
	Index(char* indexName, KeyType *keytype, int payloadlen){
		utils = new Utils();
		fHandler = new FileHandler(indexName);
		this->keytype.numAttrs = keytype->numAttrs;
		for (int i=0; i < keytype->numAttrs;i++) {
			this->keytype.attrTypes[i] = keytype->attrTypes[i];
			this->keytype.attrLen[i] = keytype->attrLen[i];
		}
		this->payloadlen = payloadlen;
		header = (char *)malloc(BLOCK_SIZE);
		utils->copyBytes(&header[NODE_OFFSET_SIZE],utils->getBytesForInt(payloadlen),sizeof(int));
		utils->copyBytes(&header[NODE_OFFSET_SIZE+sizeof(payloadlen)],utils->getBytesForKeyType(this->keytype),sizeof(KeyType));
	}

	/*
	 * Constructor to load already existing index
	 */
	Index(char* indexName){
		fHandler = new FileHandler(indexName,'o');
		header = (char *)malloc(BLOCK_SIZE);
		fHandler->readBlock(0,header);
		utils = new Utils();
		utils->copyBytes(rootAddress,header,NODE_OFFSET_SIZE);
		root = new TreeNode();
		loadNode(root,rootAddress);
		payloadlen= utils->getIntForBytes(&header[NODE_OFFSET_SIZE]);
		keytype = utils->getKeyTypeForBytes(&header[NODE_OFFSET_SIZE+sizeof(payloadlen)]);
	}
/*
 * Compare function. Takes care of composite keys
 */
	//returns -1 if first value is smaller
	int compare(char *received_key, char *received_nodeKey) {
		char *key = received_key, *nodeKey = received_nodeKey;
		for (int i=0; i < keytype.numAttrs; i++){
			switch(keytype.attrTypes[i]) {
			case intType:
				if ( utils->getIntForBytes(key) > utils->getIntForBytes(nodeKey) )
					return 1;
				else if ( utils->getIntForBytes(key) < utils->getIntForBytes(nodeKey) )
					return -1;
				break;

			case stringType:
				int result = strncmp (key, nodeKey, keytype.attrLen[i]);
				if (result != 0 )
						return result;
				break;
			}
			key = key + keytype.attrLen[i];
			nodeKey = nodeKey + keytype.attrLen[i];
		}
		return 0;
	}
/*
 * This stores the node on the disk. If the offset passed is -1 node will be stored at the end of the file.
 * This indicated newly added node.
 */
	int storeNode(TreeNode *node, long long int offset){
		if(offset == -1)
		{
			offset = fHandler->getSize();

		}
		utils->copyBytes(node->myaddr,utils->getBytesForInt(offset),NODE_OFFSET_SIZE);
		char *block = (char *)malloc(BLOCK_SIZE);
		int position = 0;
		utils->copyBytes(&block[position],utils->getBytesForInt(offset),NODE_OFFSET_SIZE);
		position += NODE_OFFSET_SIZE;
		block[position]=node->flag;
		position += 1;
		utils->copyBytes(&block[position],utils->getBytesForInt(node->numkeys),sizeof(node->numkeys));
		position += sizeof(node->numkeys);
		utils->copyBytes(&block[position],(node->data),sizeof(node->data));
		fHandler->writeBlock(offset,block);
		free(block);
		return 0;
		}
/*
 * This loads the node.We are using an in memory structure for node.Following function generates the node from the block read.
 */
	int loadNode(TreeNode *here,char *offset){
		int position=0;
		char *block = (char *)calloc(BLOCK_SIZE,BLOCK_SIZE);
		fHandler->readBlock(utils->getIntForBytes(offset),block);
		utils->copyBytes(here->myaddr,offset,NODE_OFFSET_SIZE);
		position+= NODE_OFFSET_SIZE;
		here->flag = block[position];
		position+=1;
		here->numkeys = utils->getIntForBytes(&(block[position]));
		position += sizeof(here->numkeys);
		utils->copyBytes(here->data,&(block[position]),sizeof(here->data));
		free(block);
		return 0;
	}
/*
 * Main insert function to insert the node in tree.
 */
	int insert(char key[], char payload[]){
		if(root == 0)
		{
			addFirstElement(key,payload);
			return 0;
		}
		TreeNode * current = new TreeNode();
		loadNode(current,rootAddress);
		char *nodekey;
		nodekey = (char *)malloc(keylen(&keytype));
		char accessPath[MAX_TREE_HEIGHT][NODE_OFFSET_SIZE];
		int height = 0, i;
		while(current != 0)
		{
			utils->copyBytes((accessPath[height++]),(current->myaddr),NODE_OFFSET_SIZE);
			for (i = 0 ; i<current->numkeys ; i++ )
			{
				current->getKey(keytype,nodekey,i);
				int isLesser = compare(nodekey,key);
				if(isLesser == 0 && current->flag == 'c')
				{
					printf("Already Exists!!");
					return 1;
				}
				if ( isLesser >= 0)
				{
					break;
				}
			}

			if (current->flag == 'c')
				handleLeaf(key, payload, &current, i, accessPath,height);
			else
				handleNonLeaf(&current, i);
		}
		free(nodekey);
		loadNode(root,rootAddress);
		return 0;

	}
/*
 * This is to handle first addition. First addition needs to update header of the file with nodes address
 */
	int addFirstElement(byte *key,byte *payload)
	{
		root = new TreeNode();
		root->numkeys = 0;
		root->flag = 'c';
		root->addData(keytype,key,payloadlen,payload,0);
		root->numkeys = 1;
		utils->copyBytes(header,utils->getBytesForInt((long long int)1),NODE_OFFSET_SIZE);
		utils->copyBytes(rootAddress,utils->getBytesForInt((long long int)1),NODE_OFFSET_SIZE);
		fHandler->writeBlock(0,header);
		storeNode(root,1);
        return 0;
	}
	/*
	 * Get the next node to load
	 */
	int handleNonLeaf(TreeNode **rcvd_node, int position) {
		TreeNode *node=*rcvd_node;
		char *nextNodeAddress;
		nextNodeAddress = (char *)malloc(NODE_OFFSET_SIZE);
		node->getPayload(NODE_OFFSET_SIZE,nextNodeAddress,position);
		loadNode(*rcvd_node,nextNodeAddress);
		return 0;
	}
/*
 * This inserts data in the non leaf node.Subsequntly splits the node if necessary and also adds necessary pointers to the parent
 */
	int handleLeaf(byte key[], byte payload[], TreeNode **rcvd_node, int position, char accessPath[][NODE_OFFSET_SIZE],int height) {
		TreeNode *node=*rcvd_node;
		if(splitNecessary(node->numkeys+1,node->flag) != 1)
		{
			/*
			 * Added to this node.And no split
			 */
			node->addData(keytype,key,payloadlen,payload,position);
			node->numkeys = node->numkeys + 1;
			storeNode(node,utils->getIntForBytes(node->myaddr));
		}
		else
		{
			/*
			 * Addition with split
			 */
			TreeNode *newLeaf = new TreeNode();
			/*
			 * Temporary space to hold the node that will result on addition of new data
			 */
			int tempSpaceSize = DATA_SIZE+payloadlen+keylen(&keytype);
			char *tempSpace = (char*)calloc(tempSpaceSize,sizeof(char));

			utils->copyBytes(tempSpace,node->data,(node->numkeys)*keylen(&keytype));
			utils->copyBytes(&tempSpace[tempSpaceSize-(node->numkeys)*payloadlen],&(node->data[DATA_SIZE-(node->numkeys)*payloadlen]),(node->numkeys)*payloadlen);
			for(int j = node->numkeys-1; j >= (position); j--) {
					utils->copyBytes(&(tempSpace[(j+1)*keylen(&keytype)]), &(tempSpace[j*keylen(&keytype)]),keylen(&keytype));
				}
			strncpy(&(tempSpace[(position)*keylen(&keytype)]),key, keylen(&keytype));

			for(int j = (tempSpaceSize-node->numkeys*payloadlen); j < (tempSpaceSize-position*payloadlen); j+=payloadlen) {
					utils->copyBytes(&(tempSpace[j-payloadlen]), &(tempSpace[j]),payloadlen);
			}
			strncpy(&(tempSpace[tempSpaceSize-(position+1)*payloadlen]),payload,payloadlen);
			node->numkeys = node->numkeys+1;
			int n_by_two = (node->numkeys)/2;
			for(int i = 0 ; i < n_by_two ; i++)
			{
				utils->copyBytes(&(node->data[(i)*keylen(&keytype)]),&(tempSpace[(i*keylen(&keytype))]),keylen(&keytype));
				utils->copyBytes(&(node->data[DATA_SIZE-((i+1))*payloadlen]),&(tempSpace[tempSpaceSize-((i+1)*payloadlen)]),payloadlen);
			}
			for(int i = n_by_two; i< node->numkeys ; i++)
			{
				utils->copyBytes(&(newLeaf->data[(i-n_by_two)*keylen(&keytype)]),&(tempSpace[(i*keylen(&keytype))]),keylen(&keytype));
				utils->copyBytes(&(newLeaf->data[DATA_SIZE-((i+1)-n_by_two)*payloadlen]),&(tempSpace[tempSpaceSize-((i+1)*payloadlen)]),payloadlen);
			}
			newLeaf->flag = 'c';
			newLeaf->numkeys = node->numkeys - n_by_two;
			node->numkeys = n_by_two;
			/*
			 * Get the parent to add pointers to.accessPath has list of all nodes accessed till now.This array gives us the pareent
			 */
			TreeNode* parent = new TreeNode();
			for(int i = 0 ; i < height ; i++)
				if(strncmp(accessPath[i],(node->myaddr),NODE_OFFSET_SIZE) == 0){
					if( i != 0)
						loadNode(parent,accessPath[i-1]);
				}
			char* nextKey =(char *) malloc(keylen(&keytype));
			newLeaf->getKey(keytype,nextKey,0);
			storeNode(node,utils->getIntForBytes(node->myaddr));
			storeNode(newLeaf,-1);
			char left[NODE_OFFSET_SIZE];
			utils->copyBytes(left,node->myaddr,NODE_OFFSET_SIZE);
			char right[NODE_OFFSET_SIZE];
			utils->copyBytes(right,newLeaf->myaddr,NODE_OFFSET_SIZE);
			char parentAdd[NODE_OFFSET_SIZE];
			utils->copyBytes(parentAdd,parent->myaddr,NODE_OFFSET_SIZE);
			if(node != root)
				delete(node);
			delete(newLeaf);
			delete(parent);
			insertIntoParent(left,nextKey,right,parentAdd,height,accessPath);
		}
		*rcvd_node=0;
		return 0;
	}
	/*
	 * This inserts pointers into node.May result in node split which is also handled.This results in recursive call to add pointers
	 * to the parents up the tree till root
	 */
	int insertIntoParent(byte left[NODE_OFFSET_SIZE],byte key[],byte right[NODE_OFFSET_SIZE],byte parentOffset[NODE_OFFSET_SIZE],int height,char accessPath[][NODE_OFFSET_SIZE]){
		if(strncmp(rootAddress,left,NODE_OFFSET_SIZE) == 0)
		{
			TreeNode *newRoot = new TreeNode();
			newRoot->numkeys = 1;
			newRoot->flag='n';
			utils->copyBytes(newRoot->data,key,keylen(&keytype));
			utils->copyBytes(&(newRoot->data[DATA_SIZE-NODE_OFFSET_SIZE]),left,NODE_OFFSET_SIZE);
			utils->copyBytes(&(newRoot->data[DATA_SIZE-NODE_OFFSET_SIZE*2]),right,NODE_OFFSET_SIZE);
			root = newRoot;
			storeNode(newRoot,-1);
			utils->copyBytes(rootAddress,root->myaddr,NODE_OFFSET_SIZE);
			byte *block = (byte *)malloc(BLOCK_SIZE);
			fHandler->readBlock(0,block);
			utils->copyBytes(block,rootAddress,NODE_OFFSET_SIZE);
			fHandler->writeBlock(0,block);
			free(block);
			return 0;
		}

		TreeNode *parent = new TreeNode();
		loadNode(parent,parentOffset);
		int i;
		char *nodekey;
		nodekey = (char *)malloc(keylen(&keytype));
		for (i = 0 ; i < parent->numkeys ; i++ )
		{
			parent->getKey(keytype,nodekey,i);
			int isLesser = compare(nodekey,key);
			if ( isLesser != -1)
			{
				break;
			}
		}
		if(splitNecessary(parent->numkeys+1,parent->flag) != 1)
		{
			parent->addData(keytype,key,NODE_OFFSET_SIZE,right,i);
			parent->numkeys = parent->numkeys +1;
			storeNode(parent,utils->getIntForBytes(parent->myaddr));
		}
		else
		{
			TreeNode *newNonLeaf = new TreeNode();
			int numPointers = parent->numkeys;
			if(parent->flag != 'c')
				numPointers++;
			int tempSpaceSize = DATA_SIZE+payloadlen+keylen(&keytype);
			char *tempSpace =(char *)calloc(tempSpaceSize,sizeof(char));
			utils->copyBytes(tempSpace,parent->data,(parent->numkeys)*keylen(&keytype));
			utils->copyBytes(&tempSpace[tempSpaceSize-(numPointers)*NODE_OFFSET_SIZE],&(parent->data[DATA_SIZE-(numPointers)*NODE_OFFSET_SIZE]),(numPointers)*NODE_OFFSET_SIZE);
			for(int j = parent->numkeys-1; j >= (i); j--) {
					utils->copyBytes(&(tempSpace[(j+1)*keylen(&keytype)]), &(tempSpace[j*keylen(&keytype)]),keylen(&keytype));
				}
			strncpy(&(tempSpace[(i)*keylen(&keytype)]),key, keylen(&keytype));
			int pointerPosition = i;
			if(parent->flag != 'c')
				pointerPosition ++;
			for(int j = (tempSpaceSize-numPointers*NODE_OFFSET_SIZE); j < (tempSpaceSize-pointerPosition*NODE_OFFSET_SIZE); j+=NODE_OFFSET_SIZE) {
					utils->copyBytes(&(tempSpace[j-NODE_OFFSET_SIZE]), &(tempSpace[j]),NODE_OFFSET_SIZE);
			}
			strncpy(&(tempSpace[tempSpaceSize-(pointerPosition+1)*NODE_OFFSET_SIZE]),right,NODE_OFFSET_SIZE);

			parent->numkeys = parent->numkeys+1;
			int n_by_two = (parent->numkeys)/2;
			int k = 0;
			for(int i = 0 ; i < n_by_two ; i++)
			{
				utils->copyBytes(&(parent->data[(i)*keylen(&keytype)]),&(tempSpace[(i*keylen(&keytype))]),keylen(&keytype));
				utils->copyBytes(&(parent->data[DATA_SIZE-((i+1))*NODE_OFFSET_SIZE]),&(tempSpace[tempSpaceSize-((i+1)*NODE_OFFSET_SIZE)]),NODE_OFFSET_SIZE);
				k = i+1;
			}
			if(parent->flag != 'c')
				utils->copyBytes(&(parent->data[DATA_SIZE-((k+1))*NODE_OFFSET_SIZE]),&(tempSpace[tempSpaceSize-((k+1)*NODE_OFFSET_SIZE)]),NODE_OFFSET_SIZE);
			for(int i = n_by_two+1; i< parent->numkeys ; i++)
			{
				utils->copyBytes(&(newNonLeaf->data[(i-(n_by_two+1))*keylen(&keytype)]),&(tempSpace[(i*keylen(&keytype))]),keylen(&keytype));
				utils->copyBytes(&(newNonLeaf->data[DATA_SIZE-((i+1)-(n_by_two+1))*NODE_OFFSET_SIZE]),&(tempSpace[tempSpaceSize-((i+1)*NODE_OFFSET_SIZE)]),NODE_OFFSET_SIZE);
				k= i+1;
			}
			if(parent->flag !='c')
				utils->copyBytes(&(newNonLeaf->data[DATA_SIZE-((k+1)-(n_by_two+1))*NODE_OFFSET_SIZE]),&(tempSpace[tempSpaceSize-((k+1)*NODE_OFFSET_SIZE)]),NODE_OFFSET_SIZE);
			newNonLeaf->flag = 'n';
			newNonLeaf->numkeys = parent->numkeys - n_by_two -1;
			parent->numkeys = n_by_two;
			TreeNode* grandParent = new TreeNode();
			for(int i = 0 ; i < height ; i++)
				if(strncmp(accessPath[i],(parent->myaddr),NODE_OFFSET_SIZE) == 0)
					if (i != 0)
						loadNode(grandParent,accessPath[i-1]);
			char* nextKey =(char *) malloc(keylen(&keytype));
			utils->copyBytes(nextKey,&(tempSpace[(n_by_two*keylen(&keytype))]),keylen(&keytype));
			storeNode(newNonLeaf,-1);
			storeNode(parent,utils->getIntForBytes(parent->myaddr));
			char left[NODE_OFFSET_SIZE];
			utils->copyBytes(left,parent->myaddr,NODE_OFFSET_SIZE);
			char right[NODE_OFFSET_SIZE];
			utils->copyBytes(right,newNonLeaf->myaddr,NODE_OFFSET_SIZE);
			char parentAdd[NODE_OFFSET_SIZE];
			utils->copyBytes(parentAdd,grandParent->myaddr,NODE_OFFSET_SIZE);
			if(parent!=root)
				delete(parent);
			delete(newNonLeaf);
			delete(grandParent);
			insertIntoParent(left,nextKey,right,parentAdd,height,accessPath);
		}
		return 0;
	}
	/*
	 * To check if the split is necessary
	 */

	int splitNecessary(int numkeys,char type){
		int allowedKeys;
		if(type == 'c')
			allowedKeys = (DATA_SIZE)/((keylen(&keytype)+payloadlen)+NODE_OFFSET_SIZE);
		else
		{
			allowedKeys = (DATA_SIZE)/((keylen(&keytype)+NODE_OFFSET_SIZE)+NODE_OFFSET_SIZE);
		}
		if(numkeys > allowedKeys)
			return 1;
		return 0;
	}
/*
 * Lookup in tree
 */
	int lookup(char key[], char payload[]){
		if(root == 0) {
			printf("BPlus Tree empty.");
			return 1;
		}
		TreeNode * current = new TreeNode();
		loadNode(current,rootAddress);
		char *nodekey;
		nodekey = (char *)malloc(keylen(&keytype));
		int i, isLesser;
		while(current != 0) {
			for (i = 0 ; i<current->numkeys ; i++ ) {
				current->getKey(keytype,nodekey,i);
				isLesser = compare(nodekey,key);
				if ( isLesser == 1 || (isLesser ==0 && current->flag =='c') ){
					break;
				}
			}

			if (current->flag == 'c') {
				if (isLesser != 0)	//key not found
					return 1;

				//key found, copy payload
				strncpy(payload,&(current->data[DATA_SIZE-(i+1)*payloadlen]),payloadlen);
				return 0;
			}
			else
				handleNonLeaf(&current, i);
		}
		return 1;
	}
};
/*
 * Test main function. Generate 10000 random numbers and lookup for the same.
 */
int main(){
	KeyType keyType;
	keyType.numAttrs=1;
	keyType.attrTypes[0]=stringType;
	keyType.attrLen[0]=8;
	char *filename = "/home/sandeep/work/cs631/BPlusTree/indexomp1.ind";
	srand(1);
	class Index *index = new Index(filename);
//	class Index *index = new Index(filename,&keyType,20);
	char *keyN = (char *)calloc(8,1);
	char payL[20];
	int a;
	Utils *utils  = new Utils();
	int i;
	for(i = 0 ; i < 10000 ; i++)
	{
//		sprintf(keyN,"%d",rand()%20);
		a = rand()%2000;

//		char *str = utils->getBytesForInt(rand()%20);
		printf("\n %d",a);
		utils->copyBytes(keyN,utils->getBytesForInt(a),8);
//		printf("str = %s and %s",str,keyN);
		strcpy(payL,keyN);
		strcat(payL,"-ptr");
		index->insert(keyN,payL);
	}
	printf("CHECK ME++++   %d",i);
	delete(index);
//	int a;
//	int found;
//	srand(1);
//	class Index *index = new Index(filename);
//	for ( int i = 0 ; i < 200 ; i++){
//		utils->copyBytes(keyN,utils->getBytesForInt(rand()%30),8);
//		found = index->lookup(keyN,payL);
//		printf("\n %d ==== %d",i,found);
//	}
//
//	index->insert("3","3-ptr");
//	index->insert("17","17-ptr");
//	index->insert("6","6-ptr");
//	index->insert("6","6-ptr");
//	index->insert("61","6-ptr");
//	index->insert("5","ankur");
//	index->insert("53","ankur");
//	index->insert("62","6-ptr");
//	index->insert("67","6-ptr");
//	index->insert("17","17-ptr");
//	index->insert("173","17-ptr");
//	index->insert("5","ankur");
//	index->insert("52","ankur");
//	index->insert("171","17-ptr");
//	index->insert("174","17-ptr");
//	index->insert("33","swapnil");
//	index->insert("23","meme");
//	delete(index);
//	index->insert("7","7");
//	index->insert("9","9");
//	index->insert("4","4");
//	index->insert("45","45");
//	index->insert("46","46");

//	char answer[60];
//	index->lookup("62",answer);
//	printf("Found!! %s",answer);
//	index->lookup("53",answer);
//	printf("Found!! %s",answer);
//	index->lookup("174",answer);
//	printf("Found!! %s",answer);
//	index->lookup("0",answer);
//	printf("Found!! %s",answer);
//	index->lookup("1746",answer);
//	printf("Found!! %s",answer);
	return 0;
}



/*
 * Functions to handle generation of composite keys.
 */
int getStringAttrVal(char *key, KeyType *keytype, int attrnum, char *retval){
	if(attrnum > keytype->numAttrs || keytype->attrTypes[attrnum] != stringType)
			return 0;
	int i;
	for(i = 0 ; i < attrnum ; i++ )
		key = key + keytype->attrLen[i];
	for(int j = 0 ; j < keytype->attrLen[i] ; j++)
		retval[j] = *(key+j);
	return 1;
}

int setStringAttrVal(char *key, KeyType *keytype, int attrnum, char *retval){
	if(attrnum > keytype->numAttrs || keytype->attrTypes[attrnum] != stringType)
		return 0;
	int i;
		for(i = 0 ; i < attrnum ; i++ )
			key = key + keytype->attrLen[i];
		for(int j = 0 ; j < keytype->attrLen[i] ; j++)
			*(key+j) = retval[j];
	return 1;
}

int getIntAttrVal(char *key, KeyType *keytype, int attrnum, int& retval){
	if(attrnum > keytype->numAttrs || keytype->attrTypes[attrnum] != intType)
			return 0;
	for(int i = 0 ; i < attrnum ; i++ )
			key = key + keytype->attrLen[i];
	retval = *(int *)key;
	return 1;

}

int setIntVal(char *key, KeyType *keytype, int attrnum, int val){
	if(attrnum > keytype->numAttrs || keytype->attrTypes[attrnum] != intType)
		return 0;
	for(int i = 0 ; i < attrnum ; i++ )
		key = key + keytype->attrLen[i];
	*(int *)key = val;
	return 1;
}

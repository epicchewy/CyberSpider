//
//  DiskMultiMap.cpp
//  Project 4
//
//  Created by Luke Chui on 3/9/16.
//  Copyright © 2016 Luke Chui. All rights reserved.
//

#include "DiskMultiMap.h"
#include <functional>
#include <string>

size_t DiskMultiMap::hashKey(const std::string str){
    hash<std::string> keyHash;
    return keyHash(str);
}

int DiskMultiMap::hashToOffset(const std::size_t hashedItem){
    Header head;
    m_bf.read(head, 0);
    int index = (hashedItem % head.m_buckets) * (sizeof(HashTableBucket)) + sizeof(Header);
    return index;
}

bool DiskMultiMap::updateHeader(Header head){
    return m_bf.write(head, 0);
}

// updates the given bucket
bool DiskMultiMap::updateBucket(HashTableBucket bucket, int offset){
    return m_bf.write(bucket, offset);
}

// updates the given node
bool DiskMultiMap::updateNode(Node node, int offset){
    return m_bf.write(node, offset);
}

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets){
    
    m_bf.close();
    
    bool success = m_bf.createNew(filename);
    if(!success){
        return false;
    }
    
    m_fileName = filename;
    
    int currentBucket = sizeof(Header);
    int sizeBucket = sizeof(HashTableBucket);
    bool check = false;
    
    for(int i = 0; i < numBuckets; i++){
        check = m_bf.write(HashTableBucket(), currentBucket);
        currentBucket += sizeBucket;
    }
    
    Header head;
    head.m_buckets = numBuckets;
    head.m_tail = currentBucket;
    
    check = m_bf.write(head, 0);
    return check;
}

bool DiskMultiMap::openExisting(const std::string& filename){
    
    m_bf.close();
    
    bool success = m_bf.openExisting(filename);
    if(!success){
        return false;
    }
    
    return true;
}

void DiskMultiMap::close(){
    m_bf.close();
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context){
    
    if(key.size() > 120 || value.size() > 120 || context.size() > 120){
        return false;
    }
    
    MultiMapTuple tuple;
    tuple.key = key;
    tuple.value = value;
    tuple.context = context;
    
    Node nodeToAdd(tuple);
    int lastNode = -1; //this is to track the deleted nodes
    
    size_t index_hash = hashKey(key);
    int bucketIndex = hashToOffset(index_hash);
    HashTableBucket bucket;
    m_bf.read(bucket, bucketIndex);
    
    Header head;
    m_bf.read(head, 0);
    
    string k = nodeToAdd.m_key;
    string v = nodeToAdd.m_value;
    string c = nodeToAdd.m_context;
    
    if(head.m_deleted == -1){
        lastNode = head.m_tail;
        head.m_tail += sizeof(Node);
    }else{
        lastNode = head.m_deleted;
        Node newNode;
        m_bf.read(newNode, head.m_deleted);
        head.m_deleted = newNode.m_next;
    }
    
    nodeToAdd.m_next = bucket.m_start;
    bucket.m_start = lastNode;
    
    updateBucket(bucket, bucketIndex);
    
    if(!updateHeader(head)){
        return false;
    }
    
    if(updateNode(nodeToAdd, lastNode)){
        return true;
    }else{
        return false;
    }
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key){
    int hashedKey = hashKey(key);
    int index = hashToOffset(hashedKey);
    HashTableBucket bucket;
    m_bf.read(bucket, index);
    return Iterator(&m_bf, key, bucket.m_start);
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context){
    
    Header head;
    m_bf.read(head, 0);
    
    size_t hashedKey = hashKey(key);
    int index = hashToOffset(hashedKey);
    HashTableBucket bucket;
    m_bf.read(bucket, index); //read to proper bucket
    
    Node node;
    int address = bucket.m_start;
    
    int deletedBuckets = 0;
    
    while(address != -1){
        m_bf.read(node, address); //go through bucket
        int nextIndex = node.m_next;
        
        string m_Key = node.m_key; string m_Value = node.m_value; string m_Context = node.m_context;
        if(m_Key == key && m_Value == value && m_Context == context){
            
            deletedBuckets++;
            
            int temp = head.m_deleted; //keep track of the prior deleted node location
            head.m_deleted = address; //update the deleted location for the Header
            node.m_next = temp; //update the nodes next position
            
            updateNode(node, address);
            updateHeader(head);
        }
        address = nextIndex;
    }
    return deletedBuckets;
}

DiskMultiMap::Iterator::Iterator(){
    m_address = -1;
    key = "";
    m_binaryFile = nullptr;
}

DiskMultiMap::Iterator::Iterator(BinaryFile* bf, std::string k, int address){
    m_binaryFile = bf;
    key = k;
    Node node;
    m_address = address;
    while(m_address != -1){
        m_binaryFile->read(node, m_address);
        if(strcmp(node.m_key, key.c_str()) == 0){
            break;
        }
        m_address = node.m_next;
    }
}

bool DiskMultiMap::Iterator::isValid() const{
    return (m_address != -1);
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++(){
    if(!isValid())
        return *this;
    Node node;
    m_binaryFile->read(node, m_address); // we need to move at least once
    m_address = node.m_next;
    while(m_address != -1){
        m_binaryFile->read(node, m_address);
        if(strcmp(node.m_key, key.c_str()) == 0){
            break;
        }
        int nextAddress = node.m_next;
        m_address = nextAddress;
    }
    return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*(){
    if(!isValid()){
        return MultiMapTuple();
    }
    
    Node node;
    
    if(!m_binaryFile->read(node, m_address)){
        return MultiMapTuple();
    }
    
    MultiMapTuple tuple;
    tuple.key = node.m_key;
    tuple.value = node.m_value;
    tuple.context = node.m_context;
    return tuple;
}

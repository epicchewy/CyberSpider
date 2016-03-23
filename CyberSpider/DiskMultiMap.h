//
//  DiskMultiMap.hpp
//  Project 4
//
//  Created by Luke Chui on 3/9/16.
//  Copyright © 2016 Luke Chui. All rights reserved.
//

#ifndef DiskMultiMap_hpp
#define DiskMultiMap_hpp
#include "MultiMapTuple.h"
#include "BinaryFile.h"
#include <stdio.h>
#include <string>

class DiskMultiMap
{
public:
    //	You	must	implement	this	public	nested	DiskMultiMap::Iterator	type
    class Iterator
    {
    public:
        Iterator();
        Iterator(BinaryFile* bf, std::string str, int address);
        bool isValid()	const;
        Iterator&	operator++();
        MultiMapTuple operator*();
    private:
        BinaryFile* m_binaryFile;
        std::string key;
        int m_address;
    };
    DiskMultiMap(){};
    ~DiskMultiMap(){
        m_bf.close();
    };
    bool createNew(const std::string& filename,	unsigned int numBuckets);
    bool openExisting(const std::string& filename);
    void close();
    bool insert(const std::string& key,	const std::string& value, const std::string& context);
    Iterator search(const std::string& key);
    int erase(const std::string& key, const std::string& value, const std::string& context);
private:
    struct Header{
        Header(){
            m_buckets = 0;
            m_tail = -1;
            m_deleted = -1;
        }
        int m_buckets;
        int m_tail;
        int m_deleted;
    };
    struct HashTableBucket{
        HashTableBucket(){
            m_start = -1;
        }
        int m_start;
    };
    struct Node{
        Node(){
            m_key[0] = 0;
            m_value[0] = 0;
            m_context[0] = 0;
            m_next = -1;
        }
        Node(MultiMapTuple tuple){ //copy read values into the node
            strcpy(m_key, tuple.key.c_str());
            strcpy(m_value, tuple.value.c_str());
            strcpy(m_context, tuple.context.c_str());
        }
        
        char m_key[120];
        char m_value[120];
        char m_context[120];
        int m_next;
    };
    
    size_t hashKey(const std::string str);
    int hashToOffset(const size_t);
    
    BinaryFile m_bf;
    std::string m_fileName;
    // updates the header
    bool updateHeader(Header head);
    
    // updates the given bucket
    bool updateBucket(HashTableBucket bucket, int offset);
    
    // updates the given node
    bool updateNode(Node node, int offset);
};

#endif /* DiskMultiMap_hpp */

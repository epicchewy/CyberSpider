//
//  IntelWeb.hpp
//  Project 4
//
//  Created by Luke Chui on 3/9/16.
//  Copyright © 2016 Luke Chui. All rights reserved.
//

#ifndef IntelWeb_hpp
#define IntelWeb_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <unordered_map>
#include <list>

class IntelWeb
{
public:
    IntelWeb(){
        m_ready = false;
    };
    ~IntelWeb(){
        m_ready = false;
        m_in.close();
        m_out.close();
    };
    bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
    bool openExisting(const std::string& filePrefix);
    void close();
    bool ingest(const std::string& telemetryFile);

    unsigned int crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& badInteractions
                                   );
    bool purge(const std::string& entity);
private:
    
    bool m_ready;
    
    DiskMultiMap m_in;
    DiskMultiMap m_out;

    unordered_map<string, int> m_badObjects;
    unordered_map<string, list<InteractionTuple> > m_in_cache;
    unordered_map<string, list<InteractionTuple> > m_out_cache;
    
    struct compare_interactionTuple {
        bool operator() (const InteractionTuple& lhs, const InteractionTuple& rhs) const {
            if(lhs.context == rhs.context) {
                if(lhs.from == rhs.from) {
                    return rhs.to > lhs.to;
                } else {
                    return rhs.from > lhs.from;
                }
            } else {
                return rhs.context > lhs.context;
            }
        }
    };
    
    unsigned int getBadCount(const string& key, int min);
    
};

#endif /* IntelWeb_hpp */

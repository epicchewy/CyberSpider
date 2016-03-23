//
//  IntelWeb2.cpp
//  Project 4
//
//  Created by Luke Chui on 3/10/16.
//  Copyright © 2016 Luke Chui. All rights reserved.
//

#include <stdio.h>
#include "IntelWeb.h"
#include <iostream> // needed for any I/O
#include <fstream>  // needed in addition to <iostream> for file I/O
#include <sstream>  // needed in addition to <iostream> for string stream I/O
#include <string>
#include <algorithm>
#include <queue>
#include <set>
#include <list>
#include <unordered_map>

bool IntelWeb::createNew(const std::string &filePrefix, unsigned int maxDataItems){
    string temp1 = filePrefix + "_in.dat";
    string temp2 = filePrefix + "_out.dat";
    
    unsigned int hashMapSize = maxDataItems / .75;
    bool check1 = m_in.createNew(temp1, hashMapSize);
    bool check2 = m_out.createNew(temp2, hashMapSize);
    
    if(check1 && check2){
        m_ready = true;
        return true;
    }else{
        close();
        return false;
    }
}

bool IntelWeb::openExisting(const std::string &filePrefix){
    
    m_in.close();
    m_out.close();
    
    string temp1 = filePrefix + "_in.dat";
    string temp2 = filePrefix + "_out.dat";
    
    bool check1 = m_in.openExisting(temp1);
    bool check2 = m_out.openExisting(temp2);
    
    if(check1 && check2){
        m_ready = true;
        return true;
    }else{
        close();
        return false;
    }
    
}

bool IntelWeb::ingest(const std::string& telemetryFile){
    
    ifstream inf(telemetryFile);
    if (!inf){
        return false;
    }
    
    string line;
    
    while (getline(inf, line))
    {
        string context, key, value;
        // To extract the information from the line, we'll
        // create an input stringstream from it, which acts
        // like a source of input for operator>>
        istringstream iss(line);
        // The return value of operator>> acts like false
        // if we can't extract a word followed by a number
        if ( ! (iss >> context >> key >> value) )
        {
            continue;
        }
        // If we want to be sure there are no other non-whitespace
        // characters on the line, we can try to continue reading
        // from the stringstream; if it succeeds, extra stuff
        // is after the double.
        char dummy;
        if (iss >> dummy){
            continue;
        }
        m_out.insert(key, value, context);
        m_in.insert(value, key, context);
    }
    return true;
}

void IntelWeb::close(){
    m_in.close();
    m_out.close();
    m_ready = false;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood, std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& badInteractions){
    
    if(!m_ready){
        return false;
    }
    
    queue<string> qIndicators;
    set<string> maliciousItems;
    set<InteractionTuple, compare_interactionTuple> interactions;
    unordered_map<string, bool> m_marks;
    
    for(auto it = indicators.begin(); it != indicators.end(); it++){
        m_marks[*it] = true;
        int countBad = getBadCount(*it, minPrevalenceToBeGood);
        if(countBad > 0){
            maliciousItems.insert(*it);
            qIndicators.push(*it);
        }
    }
    
    while(!qIndicators.empty()){
        string front = qIndicators.front();
        qIndicators.pop();
        auto in_cache = m_in_cache.find(front);
        if(in_cache != m_in_cache.end()){
            auto& cache_list = in_cache->second;
            for(auto it = cache_list.begin(); it != cache_list.end(); it++){
                interactions.insert(*it);
                if(getBadCount(it->to, minPrevalenceToBeGood) >= minPrevalenceToBeGood){
                    continue;
                }
                maliciousItems.insert(it->to);
                if(m_marks[it->to] == true){
                    continue;
                }
                m_marks[it->to] = true;
                qIndicators.push(it->to);
            }
        }else{
            auto input_it = m_in.search(front);
            while(input_it.isValid()){
                auto iterator = *input_it;
                InteractionTuple newInTuple(iterator.key, iterator.value, iterator.context);
                interactions.insert(newInTuple);
                
                if(getBadCount(newInTuple.to, minPrevalenceToBeGood) >= minPrevalenceToBeGood){
                    ++input_it;
                    continue;
                }
                
                maliciousItems.insert(newInTuple.to);
                
                if(m_marks[newInTuple.to] == true){
                    ++input_it;
                    continue;
                }
                
                m_marks[newInTuple.to] = true;
                qIndicators.push(newInTuple.to);
            }
        }
        
        auto out_cache = m_out_cache.find(front);
        if(out_cache != m_out_cache.end()){
            auto& cache_list = out_cache->second;
            for(auto it = cache_list.begin(); it != cache_list.end();it++){
                interactions.insert(*it);
                if(getBadCount(it->from, minPrevalenceToBeGood) >= minPrevalenceToBeGood){
                    continue;
                }
                maliciousItems.insert(it->from);
                if(m_marks[it->from] == true){
                    continue;
                }
                m_marks[it->from] = true;
                qIndicators.push(it->from);
            }
        }else{
            auto output_it = m_out.search(front);
            while(output_it.isValid()){
                auto t = *output_it;
                InteractionTuple newOutTuple(t.value, t.key, t.context);
                interactions.insert(newOutTuple);
                if(getBadCount(newOutTuple.from, minPrevalenceToBeGood) >= minPrevalenceToBeGood){
                    ++output_it;
                    continue;
                }
                maliciousItems.insert(newOutTuple.from);
                if(m_marks[newOutTuple.from] == true){
                    ++output_it;
                    continue;
                }
                m_marks[newOutTuple.from] = true;
                qIndicators.push(newOutTuple.from);
            }
        }
        
    }
    
    badEntitiesFound.assign(maliciousItems.begin(), maliciousItems.end());
    badInteractions.assign(interactions.begin(), interactions.end());
    
    for(auto it = indicators.begin(); it != indicators.end(); it++){
        maliciousItems.insert(*it);
    }
    
    return maliciousItems.size();
}

bool IntelWeb::purge(const std::string& entity){
    
    if(!m_ready){
        return false;
    }
    
    bool purged = false;
    
    m_in_cache.erase(entity);
    m_out_cache.erase(entity);
    
    list<InteractionTuple> input;
    list<InteractionTuple> output;
    
    auto iterator_in = m_in.search(entity);
    auto iterator_out = m_out.search(entity);
    
    while(iterator_in.isValid()){
        auto interaction = *iterator_in;
        InteractionTuple tuple(interaction.key, interaction.value, interaction.context);
        input.push_back(tuple);
    }
    
    while(iterator_out.isValid()){
        auto interaction = *iterator_out;
        InteractionTuple tuple(interaction.value, interaction.key, interaction.context);
        output.push_back(tuple);
    }
    
    for(auto it = input.begin(); it != input.end(); it++){
        int i = m_in.erase(it->from, it->to, it->context);
        int j = m_out.erase(it->to, it->from, it->context);
        if(i > 0 || j > 0){
            purged = true;
        }
    }
    
    for(auto it = output.begin(); it != output.end(); it++){
        int i = m_in.erase(it->from, it->to, it->context);
        int j = m_out.erase(it->to, it->from, it->context);
        if(i > 0 || j > 0){
            purged = true;
        }
    }
    
    return purged;
}


unsigned int IntelWeb::getBadCount(const std::string& key, int min){
    auto it = m_badObjects.find(key);
    
    if(it == m_badObjects.end()){
        int numBad = 0;
        auto input_it = m_in.search(key);
        while(input_it.isValid()){
            auto interaction = *input_it;
            InteractionTuple tuple(interaction.key, interaction.value, interaction.context);
            m_in_cache[key].push_back(tuple);
            if(++numBad >= min){
                m_badObjects[key] = numBad;
                return numBad;
            }
            ++input_it;
        }
        auto output_it = m_out.search(key);
        while(output_it.isValid()){
            auto interaction = *output_it;
            InteractionTuple tuple(interaction.value, interaction.key, interaction.context);
            m_out_cache[key].push_back(tuple);
            if(++numBad >= min){
                m_badObjects[key] = numBad;
                return numBad;
            }
            ++output_it;
        }
        m_badObjects[key] = numBad;
        return numBad;
    }else{
        return it->second;
    }
}


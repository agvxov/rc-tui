#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "Service.hpp"
#include <vector>

void search(std::vector<Service*> &list, const char * const &query) {
	for (int i = list.size()-1; i > -1; i--) {
		if (list[i]->name.find(query) == std::string::npos) {
			list.erase(list.begin() + i);
		}
	}
}

#if 0
#include <string>
#include <vector>
#include <algorithm>

// https://github.com/Meteorix/pylcs

vector<string> utf8_split(const string &str){
    vector<string> split;
    int len = str.length();
    int left = 0;
    int right = 1;

    for (int i = 0; i < len; i++){
        if (right >= len || ((str[right] & 0xc0) != 0x80)){
            string s = str.substr(left, right - left);
            split.push_back(s);
            // printf("%s %d %d\n", s.c_str(), left, right);
            left = right;
        }
        right ++;
    }
    return split;
}

// https://github.com/schiffma/distlib
int mini(int a, int b, int c){
    return(min(a, min(b,c)));
}

int levenshtein_dist(const string &word1, const string &word2){
    ///
    ///  Please use lower-case strings
    /// word1 : first word
    /// word2 : second word
    ///
	
    //int size1 = word1.size(), size2 = word2.size();

    vector<string> word1_ = utf8_split(word1);
    vector<string> word2_ = utf8_split(word2);
    int size1 = word1_.size();
    int size2 = word2_.size();		
	
    int suppr_dist, insert_dist, subs_dist;
    int* dist = new int[(size1+1)*(size2+1)];

    for(int i=0; i<size1+1; ++i)
        dist[(size2+1)*i] = i;
    for(int j=0; j<size2+1; ++j)
        dist[j] = j;
    for(int i=1; i<size1+1; ++i){
        for(int j=1; j<size2+1; ++j){
            suppr_dist = dist[(size2+1)*(i-1)+j] + 1;
            insert_dist = dist[(size2+1)*i+j-1] + 1;
            subs_dist = dist[(size2+1)*(i-1)+j-1];
            if(word1_[i-1]!=word2_[j-1]){  // word indexes are implemented differently.
                subs_dist += 1;
            }
            dist[(size2+1)*i+j] = mini(suppr_dist, insert_dist, subs_dist);
        }
    }
    // --------------------------------------------------------
    int res = dist[(size1+1)*(size2+1) - 1];
    delete dist;
    return(res);
}
#endif

#endif

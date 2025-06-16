#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>

using namespace std;


struct hfTreeNode {
    char ch;
    uint64_t freq;
    hfTreeNode* left;
    hfTreeNode* right;

    hfTreeNode(char c, uint64_t f, hfTreeNode* l, hfTreeNode* r)
        : ch(c), freq(f), left(l), right(r) {}
    hfTreeNode(char c, uint64_t f)
        : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare{
    bool operator()(hfTreeNode* a, hfTreeNode* b){
        return a->freq > b->freq;
    }
};

hfTreeNode* build_huffman_tree(unordered_map<char, uint64_t> &freq_map){
    priority_queue<hfTreeNode*, vector<hfTreeNode*>,Compare> pq;
    
    for (const auto& x : freq_map)
        pq.push(new hfTreeNode(x.first,x.second));

    while(pq.size() > 1){
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();

        hfTreeNode* new_node = new hfTreeNode('\0',left->freq+right->freq,left,right);
        pq.push(new_node);
    }
    return pq.top();
}

void build_code_map(hfTreeNode* node, string& path, unordered_map<char, string>& code_map){
    if(!node) return;
    if(node->left == nullptr and node->right == nullptr){
        code_map[node->ch] = path;
        return;
    }

    path.push_back('0');
    build_code_map(node->left,path,code_map);
    path.pop_back();

    path.push_back('1');
    build_code_map(node->right,path,code_map);
    path.pop_back();
}

string encode(const string& input, const unordered_map<char, string>& code_map){
    ostringstream oss;
    for (char ch : input)
        oss << code_map.at(ch);
    return oss.str();
}

int main() {
    string input_str = "It was the best of times, it was the worst of times.";
    
    // Step 1: Frequency count
    unordered_map<char, uint64_t> freq_map;
    for (char ch : input_str) freq_map[ch]++;

    // Step 2: Build Huffman tree
    hfTreeNode* root = build_huffman_tree(freq_map);

    string path;
    unordered_map<char, string> code_map;
    build_code_map(root,path,code_map);

    // Step 4: Encode input string
    string encoded = encode(input_str, code_map);

    cout << "Original size:   " << input_str.length() * 8 << " bits\n";
    cout << "Compressed size: " << encoded.length() << " bits\n";
    cout << "\nEncoded string:\n" << encoded << "\n";

    cout << "\nHuffman Codes:\n";
    for (const auto& x : code_map)
        cout << "'" << x.first << "': " << x.second << "\n";

}
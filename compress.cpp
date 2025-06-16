#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <fstream>

using namespace std;

class BitWriter {
    std::ofstream out;
    uint8_t buffer = 0;
    int bit_count = 0;
    size_t total_bits = 0;

public:
    BitWriter(const std::string& filename) {
        out.open(filename, std::ios::binary);
    }

    ~BitWriter() {
        flush(); // flush any remaining bits
        out.close();
    }

    void write_bit(bool bit) {
        buffer = (buffer << 1) | bit;
        bit_count++;
        total_bits++;

        if (bit_count == 8) {
            out.put(buffer);
            bit_count = 0;
            buffer = 0;
        }
    }

    void write_code(const std::string& code) {
        for (char c : code) {
            write_bit(c == '1');
        }
    }

    void flush() {
        if (bit_count > 0) {
            buffer <<= (8 - bit_count); // pad with 0s
            out.put(buffer);
        }
    }

    size_t get_total_bits() const {
        return total_bits;
    }
};

class hfTreeNode {
public:
    char ch;
    uint64_t freq;
    hfTreeNode* left;
    hfTreeNode* right;

    hfTreeNode(char c, uint64_t f, hfTreeNode* l, hfTreeNode* r)
        : ch(c), freq(f), left(l), right(r) {}
    hfTreeNode(char c, uint64_t f)
        : ch(c), freq(f), left(nullptr), right(nullptr) {}

    bool isLeafNode(){
        return left == nullptr && right == nullptr;
    }
    
    ~hfTreeNode() {
        delete left;
        delete right;
    }
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
    if(node->isLeafNode()){
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

void encode(const string& input_filename, const string&output_filename, unordered_map<char, string>& code_map){
    // ostringstream oss;
    // for (char ch : input)
    //     oss << code_map.at(ch);
    // return oss.str();

    ifstream input_file_again(input_filename); // Reopen for second pass
        BitWriter writer(output_filename);
        char ch;
        while (input_file_again.get(ch)) {
            writer.write_code(code_map[ch]); // stream encode one char at a time
        }
        cout << "Compression successful. Output saved to: " << output_filename << "\n";
    return;
}

void build_freq_map(string input_filename, unordered_map<char, uint64_t>& freq_map){
    ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        cerr << "Failed to open input file.\n";
        return;
    }

    char ch;
    while (input_file.get(ch)) {
        freq_map[ch]++;
    }
    input_file.close();
    return;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>\n";
        return 1;
    }

    string input_filename = argv[1];
    
    // Step 1: Frequency count
    unordered_map<char, uint64_t> freq_map;
    build_freq_map(input_filename, freq_map);
    if(freq_map.size() == 0) return 1; // return fail if file is not opened or it is empty.

    // Step 2: Build Huffman tree
    hfTreeNode* root = build_huffman_tree(freq_map);

    // Step 3: Build a char to bitstream dictionary
    string path;
    unordered_map<char, string> code_map;
    build_code_map(root,path,code_map);

    string output_filename = input_filename + ".bin";

    try{
        encode(input_filename, output_filename, code_map);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    delete root;
    return 0;
}
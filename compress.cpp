#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <bitset>

using namespace std;

class BitWriter {
    std::ofstream out;
    uint8_t buffer = 0;
    int bit_count = 0;
    size_t total_bits = 0;

public:
    BitWriter(const std::string& filename) {
        out.open(filename, std::ios::binary | std::ios::trunc);
    }

    ~BitWriter() {
        flush(); // flush any remaining bits
        out.close();
    }

    void write_byte(uint8_t byte){
        bitset<8> bits(byte);
#ifdef ENABLE_LOGS
        cout << "bitset is : " << bits << endl << "Written bits are : "; 
#endif
        for (int i = 0; i<8; i++) {
            write_bit(bits[7-i]);
#ifdef ENABLE_LOGS
            cout << bits[7-i]; //debug logs
#endif
        }
#ifdef ENABLE_LOGS
        cout << endl; // debug logs
#endif
    }
    void write_bit(bool bit) {
        buffer = (buffer << 1) | bit;
        bit_count++;
        total_bits++;
#ifdef ENABLE_LOGS
        cout << bit; // debug logs
#endif
        if (bit_count == 8) {
            out.put(buffer);
            bit_count = 0;
            buffer = 0;
#ifdef ENABLE_LOGS
            cout << endl; // debug logs
#endif
        }
    }

    void write_code(const std::string& code) {
        for (char c : code) {
            write_bit(c == '1');
            total_bits += 8;
        }
    }

    void write_raw_uint16(uint16_t value) {
        for (int i = 0; i < 2; ++i)
        {
            out.put((value >> (i * 8)) & 0xFF);
            total_bits += 8;
        }
    }

    void patch_uint16_at_position(std::streampos pos, uint16_t value) {
        std::streampos current = out.tellp();
        out.seekp(pos);
        write_raw_uint16(value);
        out.seekp(current);
    }


    void flush() {
        if (bit_count > 0) {
            buffer <<= (8 - bit_count); // pad with 0s
            out.put(buffer);
            buffer = 0;
            bit_count = 0;
        }
        total_bits += bit_count;
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

struct Compare {
    bool operator()(hfTreeNode* a, hfTreeNode* b) {
        return a->freq > b->freq;
    }

};

hfTreeNode* build_huffman_tree(unordered_map<char, uint64_t> &freq_map) {
    priority_queue<hfTreeNode*, vector<hfTreeNode*>,Compare> pq;
    
    for (const auto& x : freq_map)
        pq.push(new hfTreeNode(x.first,x.second));

    while(pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();

        hfTreeNode* new_node = new hfTreeNode('\0',left->freq+right->freq,left,right);
        pq.push(new_node);
    }
    return pq.top();
}

void build_code_map(hfTreeNode* node, string& path, unordered_map<char, string>& code_map) {
    if(!node) return;
    if(node->isLeafNode()) {
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

void encode(const string& input_filename, BitWriter& writer, unordered_map<char, string>& code_map) {
    // ostringstream oss;
    // for (char ch : input)
    //     oss << code_map.at(ch);
    // return oss.str();

    ifstream input_file_again(input_filename); // Reopen for second pass
        
    char ch;
    while (input_file_again.get(ch)) {
        writer.write_code(code_map[ch]); // stream encode one char at a time
    }

    return;
}

void build_freq_map(string input_filename, unordered_map<char, uint64_t>& freq_map) {
    ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        cerr << "Failed to open input file." << endl;
        return;
    }

    char ch;
    while (input_file.get(ch)) {
        freq_map[ch]++;
    }
    input_file.close();
    return;
}

void serialize_tree(hfTreeNode* node, BitWriter &writer, uint16_t &tree_size) {
    if(node->isLeafNode()) {
        writer.write_bit(1);
        tree_size++;
#ifdef SHOW_TREE
        cout << "1"; // debug logs
#endif
        writer.write_byte(static_cast<uint8_t>(node->ch));
#ifdef SHOW_TREE
        cout << "\'" << node->ch << "\' "; // debug logs
#endif
        tree_size += 8;
        return;
    }
    else {
        writer.write_bit(0);
        tree_size++;
#ifdef SHOW_TREE
        cout << "0"; // debug logs
#endif
    }
    serialize_tree(node->left, writer, tree_size);
    serialize_tree(node->right, writer, tree_size); 
        
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>" << endl;
        return 1;
    }

    string input_filename = argv[1];
    
    // Step 1: Frequency count
    unordered_map<char, uint64_t> freq_map;
    build_freq_map(input_filename, freq_map);
    if(freq_map.size() == 0) {
        cerr << "\nCompression failed, because file couldn't be opened or it is empty." << endl;
        return 1; // return fail if file is not opened or it is empty.
    }
    // Step 2: Build Huffman tree
    hfTreeNode* root = build_huffman_tree(freq_map);
    
    // Step 3: Build a char to bitstream dictionary
    string path;
    unordered_map<char, string> code_map;
    build_code_map(root, path, code_map);

    string output_filename = input_filename + ".bin";
    uint16_t header_size = 0; //placeholder for now
    try{
        BitWriter writer(output_filename);
        writer.write_raw_uint16(header_size);
#ifdef ENABLE_LOGS
        bitset<8> bits(0); // debug logs
        cout << bits << endl; // debug logs
        cout << bits << endl; // debug logs
#endif
        serialize_tree(root, writer, header_size);
        encode(input_filename, writer, code_map);
        writer.patch_uint16_at_position(std::ios::beg,header_size);
        cout << "Compression successful. Output saved to: " << output_filename << endl;
        cout << "And your header size is: " << header_size << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    delete root;
    return 0;
}
#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <bitset>

using namespace std;

#ifdef SHOW_TREE
    ostringstream tree_ss;
#endif

class BitWriter {
    std::ofstream out;
    uint8_t buffer = 0;
    int bit_count = 0;
    size_t total_bits = 0;
#ifdef ENABLE_LOGS
    ostringstream logs_ss;
#endif
#ifdef SHOW_BYTES
    ostringstream bytes_ss;
#endif
public:
    BitWriter(const std::string& filename) {
        out.open(filename, std::ios::binary | std::ios::trunc);
    }

    ~BitWriter() {
#ifdef ENABLE_LOGS
        cout << logs_ss.str() << endl;
#endif
#ifdef SHOW_BYTES
        cout << bytes_ss.str() << endl;
#endif
        if(bit_count > 0) {
            cerr << "Some bits left in buffer, flushing at the end." << endl;
            cerr << "With padding size: " << 8 - bit_count << endl;
            flush(); // flush any remaining bits
        }
        out.close();
    }

    void write_byte(uint8_t byte)
    {
        bitset<8> bits(byte);
#ifdef SHOW_BYTES
        bytes_ss << "bitset is : " << bits << endl << "Written bits are : "; 
#endif
        for (int i = 0; i<8; i++) {
            write_bit(bits[7-i]);
#ifdef SHOW_BYTES
            bytes_ss << bits[7-i]; //debug logs
#endif
        }
#ifdef SHOW_BYTES
        bytes_ss << endl; // debug logs
#endif
    }

    void write_bit(bool bit) 
    {
        buffer = (buffer << 1) | bit;
        bit_count++;
        total_bits++;
        if (bit_count == 8) {
            out.put(buffer);
#ifdef ENABLE_LOGS
            bitset<8> bits(buffer);
            logs_ss << bits << ": " << buffer << endl; // debug logs
#endif
            bit_count = 0;
            buffer = 0;
        }
    }

    void write_code(const std::string& code) 
    {
        for (char c : code) {
            write_bit(c == '1');
            total_bits += 8;
        }
    }

    template <typename T>
    void write_raw_uint(T value) 
    {
        static_assert(std::is_integral<T>::value, "Only integral types supported");

        for (size_t i = 0; i < sizeof(T); ++i) {
            out.put((value >> (i * 8)) & 0xFF);  // Little-endian write
#ifdef ENABLE_LOGS
            bitset <8> bits((value >> (i * 8)) & 0xFF);
            logs_ss << bits << ": " << ((value >> (i * 8)) & 0xFF) << endl;
#endif
            total_bits += 8;
        }
    }

    template <typename M>
    void patch_uint_at_position(std::streampos pos, M value) 
    {
        static_assert(std::is_integral<M>::value, "Only integral types supported");

        std::streampos current_position = out.tellp();
        out.seekp(pos);
        total_bits -= sizeof(M)*8;
        write_raw_uint(value);
        out.seekp(current_position);
    }


    void flush() 
    {
        if (bit_count > 0) {
            buffer <<= (8 - bit_count); // pad with 0s
            patch_uint_at_position(std::ios::beg + 2, uint8_t(8 - bit_count));      
            out.put(buffer);
#ifdef ENABLE_LOGS
            bitset<8> bits(buffer);
            logs_ss << bits << ": " << buffer << endl;
#endif
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

hfTreeNode* build_huffman_tree(unordered_map<char, uint64_t> &freq_map) 
{
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

void build_code_map(hfTreeNode* node, string& path, unordered_map<char, string>& code_map)
{
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

void encode(const string& input_filename, BitWriter& writer, unordered_map<char, string>& code_map)
{
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

void build_freq_map(string input_filename, unordered_map<char, uint64_t>& freq_map)
{
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

void serialize_tree(hfTreeNode* node, BitWriter &writer, uint16_t &tree_size) 
{
    if(node->isLeafNode()) {
        writer.write_bit(1);
        tree_size++;
#ifdef SHOW_TREE
        tree_ss << "1"; // debug logs
#endif
        writer.write_byte(static_cast<uint8_t>(node->ch));
#ifdef SHOW_TREE
        tree_ss << "\'" << node->ch << "\' "; // debug logs
#endif
        tree_size += 8;
        return;
    }
    else {
        writer.write_bit(0);
        tree_size++;
#ifdef SHOW_TREE
        tree_ss << "0"; // debug logs
#endif
    }
    serialize_tree(node->left, writer, tree_size);
    serialize_tree(node->right, writer, tree_size); 
        
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>" << endl;
        return 1;
    }
    string input_filename = argv[argc-1];
    
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
    uint8_t padding_size = 0; //placeholder for now
    try{
        BitWriter writer(output_filename);
        writer.write_raw_uint(header_size);
        writer.write_raw_uint(padding_size);
        serialize_tree(root, writer, header_size);
        encode(input_filename, writer, code_map);
        writer.flush();
        writer.patch_uint_at_position(std::ios::beg,header_size);
#ifdef SHOW_TREE
        cout << tree_ss.str() << endl;
#endif
        cout << "Compression successful. Output saved to: " << output_filename << endl;
        cout << "And your header size is: " << header_size << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    delete root;
    return 0;
}

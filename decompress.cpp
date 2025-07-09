#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <math.h>
#include <stack>

using namespace std;

class BitReader {
    public:
    BitReader(std::istream& in) : in(in), buffer(0), bitsRemaining(0) {}

    bool readBit(bool& bit) {
        if (bitsRemaining == 0) {
            buffer = in.get();
            if (in.eof()) return false;
            bitsRemaining = 8;
        }
        bit = (buffer >> (bitsRemaining - 1)) & 1;
        --bitsRemaining;
        return true;
    }

    uint8_t readChar() {
        uint8_t byte = 0;
        for (int i = 0; i < 8; ++i) {
            bool bit;
            if (!readBit(bit)) {
                throw std::runtime_error("Unexpected end of stream while reading byte");
            }
            byte |= bit << (7-i);
        }
        bitset<8> bits(byte);
        cout << "\n" << bits << endl;
        return byte;
    }

    bool readByte(uint8_t& byte) {
        if (in.eof()) return false;

        byte = in.get();
        return true;
    }

private:
    std::istream& in;
    uint8_t buffer;
    int bitsRemaining;
};

class hfTreeNode {
public:
    char ch;
    hfTreeNode* left;
    hfTreeNode* right;

    hfTreeNode(char c, hfTreeNode* l, hfTreeNode* r)
        : ch(c), left(l), right(r) {}
    hfTreeNode(char c)
        : ch(c), left(nullptr), right(nullptr) {}
    hfTreeNode()
        : ch('\0'), left(nullptr), right(nullptr) {}

    bool isLeafNode(){
        return left == nullptr && right == nullptr;
    }
    
    ~hfTreeNode() {
        delete left;
        delete right;
    }
};

uint16_t get_tree_size(uint16_t& tree_size, BitReader& reader) {
    for (int i = 0; i < 2; ++i) {
        uint8_t byte; 
        if (!(reader.readByte(byte))) {
            throw std::runtime_error("Unexpected end of file while reading uint16_t.");
        }
        bitset<8> bits(byte);
        tree_size |= static_cast<uint16_t>((byte) << (i * 8));  // Little-endian
        // cout << "Bits[" << i << "]: " << bits << "\n";
        // cout << tree_size << endl;
    }
    cout << "Your huffman tree size is: " << tree_size << endl;

    return tree_size;
}

void build_huffman_tree(BitReader& reader, hfTreeNode* node, uint16_t tree_size) {
    //this function is incomplete
    stack<hfTreeNode*> st();
    bool bit = 0;
    if (reader.readBit(bit)) {
        // cout << bit << " ";
        if (bit){
            uint8_t ch = reader.readChar();
            // if(!reader.readByte(ch)){
            //     cerr << "Unexpected end of stream while reading byte" << endl;
            // }
            // cout << "\'" << static_cast<char>(ch) << "\' "; //debugging purposes
            node = new hfTreeNode(ch);
            return;
        } else {
            node = new hfTreeNode();
        }
    } else {
        throw std::runtime_error("Unexpected end of stream while reading byte");
    }
    
    for (uint16_t i = 1; i < tree_size;) {
        if(reader.readBit(bit)) {
            // cout << bit << " ";
            if (bit) {
                char ch = reader.readChar();
                // cout << "\'" << ch << "\' "; //debugging purposes
                // hfTreeNode* newNode = new hfTreeNode(ch);
                // if(node->left == nullptr) node->left = newNode;
                // else node->right = newNode;
                i += 9;
            } else {
                // hfTreeNode* newNode = new hfTreeNode();
                // if(node->left == nullptr) node->left = newNode;
                // else node->right = newNode;
                i++;
            }
        } else {
            throw std::runtime_error("Unexpected end of stream while reading byte");
        }
    }

}

hfTreeNode* deserialize_huffman_tree(BitReader& reader) {
    hfTreeNode* root = nullptr;
    uint16_t tree_size = 0;
    tree_size = get_tree_size(tree_size, reader);
    build_huffman_tree(reader, root, tree_size);
    return root;
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>" << endl;
        return 1;
    }
    string input_filename = argv[1];
    std::ifstream infile;
    infile.open(input_filename, std::ios::binary);
    BitReader reader = BitReader(infile);

    // uint8_t byte;
    // while(reader.readByte(byte)) {
    //     bitset<8> bits(byte);
    //     cout << bits << endl;
    // }
    hfTreeNode* root = deserialize_huffman_tree(reader);


    return 0;
}
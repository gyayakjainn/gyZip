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

    bool readBit(bool& bit) 
    {
        if (bitsRemaining == 0) {
            buffer = in.get();
            if (in.eof()) return false;
            bitsRemaining = 8;
        }
        bit = (buffer >> (bitsRemaining - 1)) & 1;
        --bitsRemaining;
        return true;
    }

    uint8_t readChar() 
    {
        uint8_t byte = 0;
        for (int i = 0; i < 8; ++i) {
            bool bit;
            if (!readBit(bit)) {
                throw std::runtime_error("Unexpected end of stream while reading byte");
            }
            byte |= bit << (7-i);
        }
#ifdef ENABLE_LOGS
        bitset<8> bits(byte);
        cout << "\n" << bits << endl;
#endif
        return byte;
    }

    bool readByte(uint8_t& byte) 
    {
        byte = in.get();
        if (in.eof()) return false;

        return true;
    }

    void seek_n_bytes_ahead(int n)
    {
        std::streampos current_position = in.tellg();
        in.seekg(current_position + n);
    }

    void seek_back_to_beginning()
    {
        in.clear();
        in.seekg(std::ios::beg);
        cout << "Position after seek: " << in.tellg() << endl;
    }

private:
    std::istream& in;
    uint8_t buffer;
    int bitsRemaining;
};

class hfTreeNode {
public:
    union {
        char ch;
        bool count;
    } u;
    hfTreeNode* left;
    hfTreeNode* right;

    hfTreeNode(char c)
        : left(nullptr), right(nullptr) 
            { u.ch = c; }
    hfTreeNode(bool x)
        : left(nullptr), right(nullptr) 
            { u.count = x; }

    bool isLeafNode(){
        return left == nullptr && right == nullptr;
    }
    
    ~hfTreeNode() {
        delete left;
        delete right;
    }
};

#ifdef SHOW_TREE
ostringstream tree_ss;
#endif

uint16_t get_tree_size(uint16_t& tree_size, BitReader& reader)
{
    for (int i = 0; i < 2; ++i) {
        uint8_t byte; 
        if (!(reader.readByte(byte))) {
            throw std::runtime_error("Unexpected end of file while reading uint16_t.");
        }
        bitset<8> bits(byte);
        tree_size |= static_cast<uint16_t>((byte) << (i * 8));  // Little-endian
#ifdef ENABLE_LOGS
        cout << "Bits[" << i << "]: " << bits << "\n";
        cout << tree_size << endl;
#endif
    }
    cout << "Your huffman tree size is: " << tree_size << endl;

    return tree_size;
}

hfTreeNode* build_huffman_tree(BitReader& reader, uint16_t tree_size) 
{
    //this function is incomplete
    hfTreeNode* root = nullptr;
    reader.seek_n_bytes_ahead(1);
    stack<hfTreeNode*> st;
    bool bit = 0;
    if (reader.readBit(bit)) {
        if (bit) {
            uint8_t ch = reader.readChar();
            root = new hfTreeNode(char(ch));
            return root;
        } else {
            root = new hfTreeNode(false);
            st.push(root);
        }
    } else {
        throw std::runtime_error("Unexpected end of stream while reading byte at line 128");
    }
    
    while (!st.empty()) {
        if(reader.readBit(bit)) {
            if (bit) {
                char ch = reader.readChar();
                hfTreeNode* newNode = new hfTreeNode(ch);
                if(st.top()->u.count == false) {
                    st.top()->left = newNode;
                    st.top()->u.count = true;
                }
                else {
                    st.top()->right = newNode;
                    st.pop();
                }
            } else {
                hfTreeNode* newNode = new hfTreeNode(false);
                if(st.top()->u.count == false) {
                    st.top()->left = newNode;
                    st.top()->u.count = true;
                }
                else{
                    st.top()->right = newNode;
                    st.pop();
                }
                st.push(newNode);
            }
        } else {
            throw std::runtime_error("Unexpected end of stream while reading byte at line 163");
        }
    }
    return root;
}

#ifdef SHOW_TREE
void print_deserialized_tree(hfTreeNode* root, uint16_t& tree_size)
{
    if(root == nullptr)
    {
        tree_ss << "Tree is empty or Root node is not assigned properly." << endl;
        return;
    }

    if(root->isLeafNode()) {
        tree_ss << "1\'" << root->u.ch << "\' ";
        tree_size += 9;
        return;
    }
    else {
        tree_ss << "0";
        tree_size++;
    }
    print_deserialized_tree(root->left, tree_size);
    print_deserialized_tree(root->right, tree_size);
}
#endif

hfTreeNode* deserialize_huffman_tree(BitReader& reader) 
{
    hfTreeNode* root = nullptr;
    uint16_t tree_size = 0;
    tree_size = get_tree_size(tree_size, reader);
    root = build_huffman_tree(reader, tree_size);
#ifdef SHOW_TREE
    uint16_t size = 0;
    print_deserialized_tree(root, size);
    tree_ss << endl << "Tree Size is: " << size << endl;
    cout << tree_ss.str();
#endif
    return root;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>" << endl;
        return 1;
    }
    string input_filename = argv[argc - 1];
    std::ifstream infile;
    infile.open(input_filename, std::ios::binary);
    BitReader reader = BitReader(infile);
#ifdef SHOW_BYTES
    ostringstream bytes_ss;
    uint8_t byte = 0;
    while(reader.readByte(byte)) {
        bitset<8> bits(byte);
        bytes_ss << bits << endl;
    }
    cout << bytes_ss.str();
    reader.seek_back_to_beginning();
#endif
    hfTreeNode* root = deserialize_huffman_tree(reader);

    return 0;
}

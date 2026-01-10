#include <iostream>
using namespace std;

class BPlusNode {
public:
    int *keys;
    BPlusNode **children;
    int n;
    bool leaf;
    BPlusNode *next;

    BPlusNode(int m, bool isLeaf) {
        leaf = isLeaf;
        keys = new int[m];
        children = new BPlusNode*[m+1];
        n = 0;
        next = NULL;
    }
};

class BPlusTree {
    int m;
    BPlusNode *root;

public:
    BPlusTree(int _m) {
        m = _m;
        root = new BPlusNode(m, true);
    }

    void insert(int k) {
        if (root->n == m - 1) {
            BPlusNode *newRoot = new BPlusNode(m, false);
            newRoot->children[0] = root;
            splitChild(newRoot, 0);
            root = newRoot;
        }
        insertNonFull(root, k);
    }

    void insertNonFull(BPlusNode *node, int k) {
        int i = node->n - 1;

        if (node->leaf) {
            while (i >= 0 && node->keys[i] > k) {
                node->keys[i+1] = node->keys[i];
                i--;
            }
            node->keys[i+1] = k;
            node->n++;
        } else {
            while (i >= 0 && node->keys[i] > k)
                i--;

            if (node->children[i+1]->n == m - 1) {
                splitChild(node, i+1);
                if (k > node->keys[i+1])
                    i++;
            }
            insertNonFull(node->children[i+1], k);
        }
    }

    void splitChild(BPlusNode *parent, int idx) {
        BPlusNode *node = parent->children[idx];
        BPlusNode *newNode = new BPlusNode(m, node->leaf);

        int mid = m / 2;

        newNode->n = node->n - mid;
        for (int i = 0; i < newNode->n; i++)
            newNode->keys[i] = node->keys[mid + i];

        if (!node->leaf) {
            for (int i = 0; i <= newNode->n; i++)
                newNode->children[i] = node->children[mid + i];
        } else {
            newNode->next = node->next;
            node->next = newNode;
        }

        node->n = mid;

        for (int i = parent->n; i > idx; i--) {
            parent->keys[i] = parent->keys[i-1];
            parent->children[i+1] = parent->children[i];
        }

        parent->keys[idx] = newNode->keys[0];
        parent->children[idx+1] = newNode;
        parent->n++;
    }

    void traverse() {
        BPlusNode *cur = root;
        while (!cur->leaf)
            cur = cur->children[0];

        while (cur) {
            for (int i = 0; i < cur->n; i++)
                cout << cur->keys[i] << " ";
            cur = cur->next;
        }
    }
};

int main() {
    BPlusTree t(4);

    t.insert(2);
    t.insert(40);
    t.insert(12);
    t.insert(30);
    t.insert(5);
    t.insert(6);
    t.insert(72);
    t.insert(173);
    t.insert(1);

    cout << "B+ Tree traversal: ";
    t.traverse();
    return 0;
}

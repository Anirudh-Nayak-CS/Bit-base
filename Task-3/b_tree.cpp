#include <iostream>

using namespace std;

// A B-Tree node
class BTreeNode
{
  int *keys;     // An array of keys
  int t;         // Minimum degree (defines the range for number of keys)
  BTreeNode **C; // An array of child pointers
  int n;         // Current number of keys
  bool leaf;

public:
  BTreeNode(int _t, bool _leaf);

  void insertNonFull(int k);

  void splitChild(int i, BTreeNode *y);

  void traverse();

  BTreeNode *search(int k);

  //  BTree friend so we can access private members of this class in BTree functions
  friend class BTree;
};

class BTree
{
  BTreeNode *root;
  int t; // Minimum degree
public:
  BTree(int _t)
  {
    root = NULL;
    t = _t;
  }

  void traverse()
  {
    if (root != NULL)
      root->traverse();
  }

  BTreeNode *search(int k)
  {
    return (root == NULL) ? NULL : root->search(k);
  }

  void insert(int k);
};

// Constructor for BTreeNode class
BTreeNode::BTreeNode(int t1, bool leaf1)
{
  t = t1;
  leaf = leaf1;

  keys = new int[2 * t - 1];
  C = new BTreeNode *[2 * t];

  // Initialize the number of keys as 0
  n = 0;
}

// Function to traverse all nodes in a subtree rooted with this node
void BTreeNode::traverse()
{
  int i;
  for (i = 0; i < n; i++)
  {
    // If this is not leaf, then before printing keys[i],
    // traverse the subtree rooted with child C[i].
    if (leaf == false)
      C[i]->traverse();
    cout << " " << keys[i];
  }

  // Print the subtree rooted with last child
  if (leaf == false)
    C[i]->traverse();
}

// Function to search key k in subtree rooted with this node
BTreeNode *BTreeNode::search(int k)
{
  int i = 0;
  while (i < n && k > keys[i])
    i++;

  if (keys[i] == k)
    return this;

  if (leaf == true)
    return NULL;

  return C[i]->search(k);
}

// The main function that inserts a new key in this B-Tree
void BTree::insert(int k)
{
  if (root == NULL)
  {
    root = new BTreeNode(t, true);
    root->keys[0] = k;
    root->n = 1;
  }
  else
  {
    // If root is full, then tree grows in height
    if (root->n == 2 * t - 1)
    {
      BTreeNode *s = new BTreeNode(t, false);
      s->C[0] = root;
      s->splitChild(0, root);

      // New root has two children now. Decide which of the two children is going to have new key
      int i = 0;
      if (s->keys[0] < k)
        i++;
      s->C[i]->insertNonFull(k);

      root = s;
    }
    else
    {
  
      root->insertNonFull(k);
    }
  }
}

void BTreeNode::insertNonFull(int k)
{
  int i = n - 1;

  if (leaf == true)
  {
    // Find location of new key and move all greater keys one space ahead
    while (i >= 0 && keys[i] > k)
    {
      keys[i + 1] = keys[i];
      i--;
    }

    keys[i + 1] = k;
    n = n + 1;
  }
  else
  {
    // Find the child which is going to have the new key
    while (i >= 0 && keys[i] > k)
      i--;

    // See if the found child is full
    if (C[i + 1]->n == 2 * t - 1)
    {
      splitChild(i + 1, C[i + 1]);

      // After split, the middle key of C[i] goes up and
      // C[i] is split into two. See which of the two
      // is going to have the new key
      if (keys[i + 1] < k)
        i++;
    }
    C[i + 1]->insertNonFull(k);
  }
}

void BTreeNode::splitChild(int i, BTreeNode *y)
{
  BTreeNode *z = new BTreeNode(y->t, y->leaf);
  z->n = t - 1;

  // Copy the last (t-1) keys of y to z
  for (int j = 0; j < t - 1; j++)
    z->keys[j] = y->keys[j + t];

  // Copy the last t children of y to z
  if (y->leaf == false)
  {
    for (int j = 0; j < t; j++)
      z->C[j] = y->C[j + t];
  }

  y->n = t - 1;

  // Since this node is going to have a new child, create space
  for (int j = n; j >= i + 1; j--)
    C[j + 1] = C[j];

  C[i + 1] = z;

  // A key of y will move to this node. Find location of
  // new key and move all greater keys one space ahead
  for (int j = n - 1; j >= i; j--)
    keys[j + 1] = keys[j];

  keys[i] = y->keys[t - 1];
  n = n + 1;
}


int main()
{
  BTree t(3); // A B-Tree with min degree 3

  t.insert(10);
  t.insert(20);
  t.insert(5);
  t.insert(6);
  t.insert(12);
  t.insert(30);
  t.insert(7);
  t.insert(17);

  cout << "Traversal of the constructed B-Tree is:";
  t.traverse();
  cout << endl;

  int k = 6;
  (t.search(k) != NULL) ? cout << "Key " << k << " is present\n" : cout << "Key " << k << " is not present\n";

  k = 15;
  (t.search(k) != NULL) ? cout << "Key " << k << " is present\n" : cout << "Key " << k << " is not present\n";

  return 0;
}
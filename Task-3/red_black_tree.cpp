#include <iostream>

enum Color
{
  RED,
  BLACK
};

struct Node
{
  int data;
  Color color;
  Node *left, *right, *parent;

  Node(int data) : data(data), color(RED), left(nullptr), right(nullptr), parent(nullptr) {}
};

class RBTree
{
private:
  Node *root;

  void rotateLeft(Node *&ptr)
  {
    Node *right_child = ptr->right;
    ptr->right = right_child->left;
    if (ptr->right != nullptr)
      ptr->right->parent = ptr;
    right_child->parent = ptr->parent;
    if (ptr->parent == nullptr)
      root = right_child;
    else if (ptr == ptr->parent->left)
      ptr->parent->left = right_child;
    else
      ptr->parent->right = right_child;
    right_child->left = ptr;
    ptr->parent = right_child;
  }

  void rotateRight(Node *&ptr)
  {
    Node *left_child = ptr->left;
    ptr->left = left_child->right;
    if (ptr->left != nullptr)
      ptr->left->parent = ptr;
    left_child->parent = ptr->parent;
    if (ptr->parent == nullptr)
      root = left_child;
    else if (ptr == ptr->parent->left)
      ptr->parent->left = left_child;
    else
      ptr->parent->right = left_child;
    left_child->right = ptr;
    ptr->parent = left_child;
  }

  void fixViolation(Node *&ptr)
  {
    Node *parent_ptr = nullptr;
    Node *grand_parent_ptr = nullptr;

    while ((ptr != root) && (ptr->color != BLACK) && (ptr->parent->color == RED))
    {
      parent_ptr = ptr->parent;
      grand_parent_ptr = ptr->parent->parent;

      if (parent_ptr == grand_parent_ptr->left)
      {
        Node *uncle_ptr = grand_parent_ptr->right;
        if (uncle_ptr != nullptr && uncle_ptr->color == RED)
        {
          grand_parent_ptr->color = RED;
          parent_ptr->color = BLACK;
          uncle_ptr->color = BLACK;
          ptr = grand_parent_ptr;
        }
        else
        {
          if (ptr == parent_ptr->right)
          {
            rotateLeft(parent_ptr);
            ptr = parent_ptr;
            parent_ptr = ptr->parent;
          }
          rotateRight(grand_parent_ptr);
          std::swap(parent_ptr->color, grand_parent_ptr->color);
          ptr = parent_ptr;
        }
      }
      else
      {
        Node *uncle_ptr = grand_parent_ptr->left;
        if ((uncle_ptr != nullptr) && (uncle_ptr->color == RED))
        {
          grand_parent_ptr->color = RED;
          parent_ptr->color = BLACK;
          uncle_ptr->color = BLACK;
          ptr = grand_parent_ptr;
        }
        else
        {
          if (ptr == parent_ptr->left)
          {
            rotateRight(parent_ptr);
            ptr = parent_ptr;
            parent_ptr = ptr->parent;
          }
          rotateLeft(grand_parent_ptr);
          std::swap(parent_ptr->color, grand_parent_ptr->color);
          ptr = parent_ptr;
        }
      }
    }
    root->color = BLACK;
  }

  void inorderHelper(Node *root)
  {
    if (root == nullptr)
      return;
    inorderHelper(root->left);
    std::cout << root->data << "(" << (root->color == RED ? "R" : "B") << ") ";
    inorderHelper(root->right);
  }

public:
  RBTree() { root = nullptr; }

  void insert(const int &data)
  {
    Node *newNode = new Node(data);
    if (root == nullptr)
    {
      newNode->color = BLACK;
      root = newNode;
    }
    else
    {
      Node *temp = root;
      Node *parent = nullptr;
      while (temp != nullptr)
      {
        parent = temp;
        if (newNode->data < temp->data)
          temp = temp->left;
        else
          temp = temp->right;
      }
      newNode->parent = parent;
      if (newNode->data < parent->data)
        parent->left = newNode;
      else
        parent->right = newNode;
      fixViolation(newNode);
    }
  }

  Node *search(int data)
  {
    Node *temp = root;
    while (temp != nullptr)
    {
      if (data == temp->data)
        return temp;
      if (data < temp->data)
        temp = temp->left;
      else
        temp = temp->right;
    }
    return nullptr;
  }

  void printTree()
  {
    inorderHelper(root);
    std::cout << std::endl;
  }
};

int main()
{
  RBTree tree;

  tree.insert(7);
  tree.insert(3);
  tree.insert(18);
  tree.insert(10);
  tree.insert(22);
  tree.insert(8);
  tree.insert(11);

  std::cout << "Inorder Traversal (Value and Color):" << std::endl;
  tree.printTree();

  int key = 10;
  if (tree.search(key))
    std::cout << "Key " << key << " found in the tree." << std::endl;
  else
    std::cout << "Key " << key << " not found." << std::endl;

  return 0;
}
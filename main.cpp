#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <string>
#include <memory>
#include <cctype>
#include <cmath>
using namespace std;

double unit_wire_res = 0;
double unit_wire_cap = 0;

double inv_input_cap = 0;
double inv_output_cap = 0;
double inv_output_res = 0;

double time_constraint = 0;

enum NodeType {
    LEAF,
    BRIDGE,
    INV
};


struct Node {
    NodeType type;
    int label;                    // valid if isLeaf
    double capacitance;           // valid if isLeaf

    double leftWire, rightWire;   // valid if !isLeaf
    Node* left;
    Node* right;

    double total_capacitance;
    double elmore_capacitance;
    double elmore_delay;

    double cut_wire;

    int polarity;

    Node(int lbl, double cap) 
        : type(LEAF), label(lbl), capacitance(cap),
          leftWire(0), rightWire(0), left(nullptr), right(nullptr), 
          total_capacitance(0.0), elmore_capacitance(0.0), elmore_delay(0.0), cut_wire(0), polarity(0){}

    Node(double lw, double rw, Node* l, Node* r)
        : type(BRIDGE), label(-1), capacitance(0),
          leftWire(lw), rightWire(rw), left(l), right(r),
          total_capacitance(0.0), elmore_capacitance(0.0), elmore_delay(0.0), cut_wire(0), polarity(0){}

    Node(double cap, double wire_left, bool inv)
        :type(INV), label(-1), capacitance(cap),
        leftWire(0), rightWire(0), left(nullptr), right(nullptr), 
        total_capacitance(0.0), elmore_capacitance(0.0), elmore_delay(0.0), cut_wire(wire_left), polarity(0) {

        }
};


int storeWireParams(const std::string& filename) {
    std::ifstream fin(filename); // object for a file
    if (!fin) { // NULL if unable to open file
        cout << "Unable to open file" << endl;
        return 0;
    }

    std::string line;

    std::getline(fin, line);
    if (line.empty())  {
        return 0;
    }

    double unit_res;
    double unit_cap;
    std::stringstream ss(line);
    ss >> unit_res >> unit_cap; // reads: label '(' capacitance

    unit_wire_res = unit_res;
    unit_wire_cap = unit_cap;

    fin.close();
    return 1;

} 

int storeInvParams(const std::string& filename) {
    std::ifstream fin(filename); // object for a file
    if (!fin) { // NULL if unable to open file
        cout << "Unable to open file" << endl;
        return 0;
    }

    std::string line;

    std::getline(fin, line);
    if (line.empty())  {
        return 0;
    }

    double first;
    double second;
    double third;
    std::stringstream ss(line);
    ss >> first >> second >> third; // reads: label '(' capacitance

    inv_input_cap = first;
    inv_output_cap = second;
    inv_output_res = third;

    fin.close();
    return 1;
}
Node* parseTree(const std::string& filename) {
    std::ifstream fin(filename); // object for a file
    if (!fin) { // NULL if unable to open file
        cout << "Unable to open file" << endl;
        return NULL;
    }

    std::stack<Node*> st; // to declare stack just define stack object
    std::string line;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // Leaf node format: d(%.10le)
        if (std::isdigit(line[0])) {
            int lbl;
            double cap;
            char dummy;
            std::stringstream ss(line);
            ss >> lbl >> dummy >> cap; // reads: label '(' capacitance
            Node* leaf = new Node(lbl, cap);
            
            leaf->total_capacitance += cap;
            
            st.push(leaf);
        }
        // non-leaf node format: (%.10le %.10le)
        else if (line[0] == '(') {
            double lw, rw;
            char dummy;
            std::stringstream ss(line);
            ss >> dummy >> lw >> rw; // reads: '(' leftWire rightWire
            Node* right = st.top(); st.pop();
            Node* left = st.top(); st.pop();
            Node* parent = new Node(lw, rw, left, right);

            // at this point we know wire connecting parent to child
            double l_wire_cap = unit_wire_cap * lw / (double) 2;
            double r_wire_cap = unit_wire_cap * rw / (double) 2;

            left->total_capacitance += l_wire_cap; // calculate Ce/2 of left wire
            right->total_capacitance += r_wire_cap; // calculate Ce/2 of right wire

            parent-> total_capacitance += l_wire_cap + r_wire_cap;//Ce/2 of left wire and right
            
            st.push(parent);
        }
    }

    if (st.empty()) {
        cout << "No tree found in file." << endl;
        return NULL;
    }

    Node* root = st.top();
    root->total_capacitance += inv_output_cap;
    // root is missing capacitance going into it Ce
    return root;

    fin.close();
}

void preOrderTraversal(Node * node, std::ofstream& fout) {
    if (!node) {
        return;
    }

    if (node->type==LEAF) {
        fout << node->label << "(" << std::scientific << node->capacitance << ")\n";
        //cout << "Leaf Node total capacitance: " << node->total_capacitance << endl;
        //cout << "Leaf Node elmore capacitance: " << node->elmore_capacitance << endl;
        //cout << "Leaf Node elmore delay: " << node->elmore_delay << endl;



    } else if (node->type==BRIDGE){
        //cout << "Non-Leaf Node total capacitance: " << node->total_capacitance << endl;
        //cout << "Non-Leaf Node elmore capacitance: " << node->elmore_capacitance << endl;
        //cout << "Non-Leaf Node elmore delay: " << node->elmore_delay << endl;

        fout << "(" << std::scientific << node->leftWire << " " << node->rightWire << ")\n";
    }

    preOrderTraversal(node->left, fout);
    preOrderTraversal(node->right, fout);

}

int writePre(Node * root, const std::string& filename) {
    std::ofstream fout(filename);
    if (!fout) {
        cout << "Unable to open file.\n";
        return 0;
    }

    preOrderTraversal(root, fout);

    fout.close();   // closes the std::ofstream
    return 1;
}

double capacitancePostOrder(Node * node) {
    if (!node) {
        return 0;
    }

    double left_cap = capacitancePostOrder(node->left);
    double right_cap = capacitancePostOrder(node->right);
    
    node->elmore_capacitance = left_cap + right_cap + (node->total_capacitance);
    
    return node->elmore_capacitance;

}
void delayPreOrder(Node * node, double curr_elmore_delay, double resistance, FILE *fp) {
    if (!node) {
        return;
    }

    curr_elmore_delay += (resistance * node->elmore_capacitance);
    node->elmore_delay = curr_elmore_delay;

    if (node->type==LEAF) {
        fwrite(&(node->label), sizeof(int), 1, fp);
        fwrite(&(node->elmore_delay), sizeof(double), 1, fp);
    }
    delayPreOrder(node->left, curr_elmore_delay, unit_wire_res * node->leftWire, fp);
    delayPreOrder(node->right, curr_elmore_delay, unit_wire_res * node->rightWire, fp);

}
int elmoreDelay(Node * root, std::string& filename) {
    // bottom up (post-order) traversal to accumulate capacitance
    // top down (pre-order) for R*C = T

    capacitancePostOrder(root);

    FILE* fp = fopen(filename.c_str(), "wb");  // convert std::string to const char*
    if (!fp) {
        std::cout << "Error: cannot open file\n";
        return 0;
    }
    delayPreOrder(root, 0, inv_output_res, fp);

    fclose(fp);     // closes the FILE* handle
    return 1;
}  

double solveQuadratic(double A, double B, double C) {

    //cout << "A, B, C : " << A << "," << B << "," << C << "," << endl;
    double discriminant = (B*B) - (4*A*C);
    double new_l = -1;
    //cout << "square root of discriminant: " << (std::sqrt(discriminant)) << endl;

    if (discriminant < 0) {
        // complex roots, no solution
        return -1;
    } else if (discriminant == 0) {
        new_l = (0-B) / (2*A);
    }  else {

        double first_root = (-B + (std::sqrt(discriminant))) / (2 * A);
        double second_root = (-B - (std::sqrt(discriminant))) / (2 * A);
        //cout << "first root: " << first_root << endl;
        //cout << "second root: " << second_root << endl;

        if (first_root > second_root) {
            if (first_root < 0) {
                return -1;
            } else {
                return first_root;
            }
        }
        else {
            if (second_root < 0) {
                return -1;
            } else {
                return second_root;
            }
        }
 
    }
    return new_l;
}

Node * inverterSegmentation(Node * node, double l, double branch_time_constraint) {

    if (node->type==LEAF) {
        //cout << "Leaf " << node->label << "(" << node->capacitance << ")\n";
    } else {
        //cout <<  "Non-leaf (" << node->leftWire << "," << node->rightWire << ")\n";

    }
    Node * temp = node;
    double temp_l = l;

    double temp_time_constraint = time_constraint - branch_time_constraint;

    double temp_elmore_c = temp->elmore_capacitance - ((temp_l * unit_wire_cap) / (double) 2);


        // Quadratic coefficients
    double A = (unit_wire_cap * unit_wire_res) / 2;
    double B = (inv_output_res * unit_wire_cap) + (unit_wire_res * temp_elmore_c);
    //cout << "Elmore's capacitance: " << temp_elmore_c << endl;
    double C = (inv_output_res * inv_output_cap) + (inv_output_res * temp_elmore_c);

    double stage_delay = (A*(l*l)) + (B*l) + C;
    //cout << "stage delay: " <<stage_delay <<endl;
    //cout << "Time constraint: " <<temp_time_constraint <<endl;

    while (stage_delay > temp_time_constraint) {
        temp_elmore_c = temp->elmore_capacitance - ((temp_l * unit_wire_cap) / (double) 2);
        // Quadratic coefficients
        A = (unit_wire_cap * unit_wire_res) / 2;
        B = (inv_output_res * unit_wire_cap) + (unit_wire_res * temp_elmore_c);
        C = (inv_output_res * inv_output_cap) + (inv_output_res * temp_elmore_c) - temp_time_constraint;

        double new_l = solveQuadratic(A, B, C);

        //cout << "L computed: " << new_l << endl;
        if (new_l == -1) {
            // try inserting on left and right
            return temp;
        } else {
            Node * inv = new Node(inv_input_cap, temp_l - new_l, true);
            inv->leftWire = new_l;
            inv->rightWire = -1;
            inv->left = temp;
            inv->total_capacitance=inv->capacitance + ((inv->cut_wire * unit_wire_cap) / 2);
            inv->elmore_capacitance = inv->total_capacitance;
            /* at this point, inv node is set up with:
                - Input capacitance
                - left pointer towards child
                - total and elmore capacitcance updated to 

            */

            
            temp = inv;
            temp_l -= new_l;
            temp->polarity=(1 + temp->left->polarity)%2;

            temp_elmore_c = temp->elmore_capacitance - ((temp_l * unit_wire_cap) / (double) 2);
            // Quadratic coefficients
            A = (unit_wire_cap * unit_wire_res) / 2;
            B = (inv_output_res * unit_wire_cap) + (unit_wire_res * temp_elmore_c);
            C = (inv_output_res * inv_output_cap) + (inv_output_res * temp_elmore_c);


            stage_delay = (A*(temp_l*temp_l)) + (B*temp_l) + C;
            
            temp_time_constraint = time_constraint;
            //cout << "stage delay: " <<stage_delay <<endl;
            //cout << "Time constraint: " <<temp_time_constraint <<endl;
        }


    }

    return temp;
}
Node* insertionPostOrder(Node * node, double l) {
    if (!node) {
        return NULL;
    }

    // temp will either carry original child or inverter
    Node * temp_left = insertionPostOrder(node->left, node->leftWire);
    if (temp_left) {
        if (temp_left->type==INV) {
            //Left branch had an inverter inserted
            double old_child_cap = node->left->elmore_capacitance;
            node->left = temp_left;
            
            node->total_capacitance-=(node->leftWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=(node->leftWire * unit_wire_cap) / 2;; 
            node->elmore_capacitance-=old_child_cap; 

            node->leftWire = node->left->cut_wire;
            node->total_capacitance+=(node->leftWire * unit_wire_cap) / 2; //good
            node->elmore_capacitance+=node->left->elmore_capacitance;
            node->elmore_capacitance+=(node->leftWire * unit_wire_cap) / 2;
            
            //cout << "Polarity of returned temp_left: " << temp_left->polarity << endl;
        }
    }
    Node * temp_right = insertionPostOrder(node->right, node->rightWire);
    if (temp_right) {
        if (temp_right->type==INV) {
            // right branch had an inverter inserted
            double old_child_cap = node->right->elmore_capacitance;
            node->right = temp_right;

            node->total_capacitance-=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=old_child_cap;

            node->rightWire = node->right->cut_wire;

            node->total_capacitance+=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance+=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance+=node->right->elmore_capacitance;

            cout << "Polarity of returned temp_right: " << temp_right->polarity << endl;

        }
    }

    // check polarity
    if (temp_left && temp_right) {
        if (temp_left->polarity == 0 && temp_right->polarity == 1) {
            // insert inverter on right branch at length l
            Node * inv = new Node(inv_input_cap, 0, true);
            inv->leftWire = node->leftWire;
            inv->rightWire = -1;
            inv->left = node->left;
            inv->total_capacitance=inv->capacitance + ((inv->cut_wire * unit_wire_cap) / 2);
            inv->elmore_capacitance = inv->total_capacitance;

            inv->polarity = (1 + inv->left->polarity)%2;

            double old_child_cap = node->left->elmore_capacitance;
            node->left = inv;
            
            node->total_capacitance-=(node->leftWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=(node->leftWire * unit_wire_cap) / 2;; 
            node->elmore_capacitance-=old_child_cap; 

            node->leftWire = node->left->cut_wire;
            node->total_capacitance+=(node->leftWire * unit_wire_cap) / 2; //good
            node->elmore_capacitance+=node->left->elmore_capacitance;
            node->elmore_capacitance+=(node->leftWire * unit_wire_cap) / 2;

            node->polarity=inv->polarity;

            //cout << "Added extra on left.\n";
        } 
        else if (temp_left->polarity == 1 && temp_right->polarity == 0){
            // insert inverter on left branch at length l
            Node * inv = new Node(inv_input_cap, 0, true);
            inv->leftWire = node->rightWire;
            inv->rightWire = -1;

            inv->left = node->right;
            inv->total_capacitance=inv->capacitance + ((inv->cut_wire * unit_wire_cap) / 2);

            inv->elmore_capacitance = inv->total_capacitance;

            inv->polarity = (1 + inv->left->polarity)%2;
            
            double old_child_cap = node->right->elmore_capacitance;
            node->right = inv;

            node->total_capacitance-=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance-=old_child_cap;

            node->rightWire = node->right->cut_wire;
            cout << "cut wire: " << node->right->cut_wire << endl;
            node->total_capacitance+=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance+=(node->rightWire * unit_wire_cap) / 2;
            node->elmore_capacitance+=node->right->elmore_capacitance;

            node->polarity = inv->polarity;
            cout << "Added extra on right.\n";

        }
        else {
            node->polarity = temp_left->polarity;
        }

    }
    

    if (!temp_left && !temp_right) {
        // at leaf node
        // check for insertion
        // if not needed return node
        // if insertion return inv node

        //double stage_delay = l * unit_wire_res * (node->elmore_capacitance);
        
        return inverterSegmentation(node, l, 0);

    } else {
        // at bridge node
        // check for insertion (second method)
        // return current node or inv node
        double t1 = (node->leftWire)*unit_wire_res*(node->left->elmore_capacitance);
        double t2 = (node->rightWire)*unit_wire_res*(node->right->elmore_capacitance);
        double child_time_constraint = (t1 > t2 ? t1 : t2);
        //double stage_delay = l * unit_wire_res * (node->elmore_capacitance);
       
        return inverterSegmentation(node, l, child_time_constraint);
    }


}

Node * inverterInsertion(Node * root) {

    Node * temp_root = insertionPostOrder(root, 0);
    return temp_root;

}

void postOrderTraversalOutput3(Node * node, std::ofstream& fout, FILE * fp) {
    if (!node) {
        return;
    }
    postOrderTraversalOutput3(node->left, fout, fp);
    postOrderTraversalOutput3(node->right, fout, fp);

    if (node->type==LEAF) {
        fout << node->label << "(" << std::scientific << node->capacitance << ")\n";
        //cout << "Leaf Node total capacitance: " << node->total_capacitance << endl;
        //cout << "Leaf Node elmore capacitance: " << node->elmore_capacitance << endl;
        //cout << "Leaf Node elmore delay: " << node->elmore_delay << endl;
        fwrite(&(node->label), sizeof(int), 1, fp);
        fwrite(&(node->capacitance), sizeof(double), 1, fp);



    } else if (node->type==BRIDGE){
        //cout << "Non-Leaf Node total capacitance: " << node->total_capacitance << endl;
        //cout << "Non-Leaf Node elmore capacitance: " << node->elmore_capacitance << endl;
        //cout << "Non-Leaf Node elmore delay: " << node->elmore_delay << endl;
        int i = -1;
        int j = 0;
        fwrite(&(i), sizeof(int), 1, fp);
        fwrite(&(node->leftWire), sizeof(double), 1, fp);
        fwrite(&(node->rightWire), sizeof(double), 1, fp);
        fwrite(&(j), sizeof(int), 1, fp);
        fout << "(" << std::scientific << node->leftWire << " " << node->rightWire << " 0)\n";
    } else {

        int i = -1;
        int j = 1;
        double neg = -1;
        fwrite(&(i), sizeof(int), 1, fp);
        fwrite(&(node->leftWire), sizeof(double), 1, fp);
        fwrite(&(neg), sizeof(double), 1, fp);
        fwrite(&(j), sizeof(int), 1, fp);
        fout << "(" << std::scientific << node->leftWire << " " << node->rightWire << " 1)\n";

    }

}

int write3rdOutputPost(Node * root, const std::string& filename, const std::string& filename2) {
    std::ofstream fout(filename);
    if (!fout) {
        //cout << "Unable to open file.\n";
        return 0;
    }

    FILE* fp = fopen(filename2.c_str(), "wb");  // convert std::string to const char*
    if (!fp) {
        std::cout << "Error: cannot open file\n";
        return 0;
    }

    postOrderTraversalOutput3(root, fout, fp);
    fout << "(" << std::scientific << (double) 0 << " " << (double)-1 << " 1)\n";
    int i = -1;
    int j = 1;
    double zero = 0;
    double neg = -1;
    fwrite(&(i), sizeof(int), 1, fp);
    fwrite(&(zero), sizeof(double), 1, fp);
    fwrite(&(neg), sizeof(double), 1, fp);
    fwrite(&(j), sizeof(int), 1, fp);

    if (root->polarity==0){
        //cout << "here\n";
        fout << "(" << std::scientific << (double) 0 << " " << (double)-1 << " 1)\n";
        fwrite(&(i), sizeof(int), 1, fp);
        fwrite(&(zero), sizeof(double), 1, fp);
        fwrite(&(neg), sizeof(double), 1, fp);
        fwrite(&(j), sizeof(int), 1, fp);
       
    }
    fout.close();   // closes the std::ofstream
    fclose(fp);     // closes the FILE* handle

    return 1;
}

void freeMyTree(Node * node) {
    if(!node) {
        return;
    }

    freeMyTree(node->left);
    freeMyTree(node->right);

    delete node;

}
int main(int argc, char **argv) {
    if (argc != 9) {
        std::cout << "Invalid number of arguments";
        return 2;
    }

    time_constraint = atof(argv[1]);    
    std::string in_name1 = argv[2];
    std::string in_name2 = argv[3];
    std::string in_name3 = argv[4];
    std::string out_name1 = argv[5];
    std::string out_name2 = argv[6];
    std::string out_name3 = argv[7];
    std::string out_name4 = argv[8];

    storeWireParams(in_name2);
    storeInvParams(in_name1);

    /*
    cout << "Unit resistance: " << std::scientific << unit_wire_res << endl;
    cout << "Unit capacitance: " << std::scientific << unit_wire_cap << endl;
    cout << "Inverter Input Capacitance: " << std::scientific << inv_input_cap << endl;
    cout << "Inverter Output Capacitance: " << std::scientific << inv_output_cap << endl;
    cout << "Inverter Output Resistance: " << std::scientific << inv_output_res << endl;
    */

    Node * tree = parseTree(in_name3);

   // result =    }
    writePre(tree, out_name1);


    elmoreDelay(tree, out_name2);
    
    //result = writePre(tree, out_name1);
    Node * new_tree =inverterInsertion(tree);

    write3rdOutputPost(new_tree, out_name3, out_name4);
    //debugPreStart(tree);

    //freeMyTree(tree);
    //freeMyTree(new_tree);

}

#include <stdio.h>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <fstream>
#include <string>

#define COMM MPI_COMM_WORLD
#define TAG 0
#define MASTER 0

#define MAT1 "mat1"
#define MAT2 "mat2"

using namespace std;
using matrix = vector<vector<int>>;
using mat_line = vector<int>;

// transpose the matrix (eg. from 2x3 -> 3x2)
matrix transpose_mat(matrix mat) {
    matrix result;
    // cols
    for (int j=0; j < (int)mat[0].size(); j++) {
        result.push_back(vector<int>());
        // rows
        for (int i=0; i < (int)mat.size(); i++) {
            result[j].push_back(mat[i][j]);
        }
    }

    return result;
}


// multiply and add
void do_multiplication(int rank, int size, int length_of_input, int len_mat2) {
    int x = rank / 2;
    int y = rank % 2;
    

    int my_cell = 0;
    int rcvd_val_L;
    int rcvd_val_T;

    // length of input is the common size of matrixes
    for (int i=0; i < length_of_input; i++) {
        // RECEIVE *****************
        // axis X - receive from left neighbour
        if (rank - 1 < 0 || rank % len_mat2 == 0) {
            MPI_Recv(&rcvd_val_L, 1, MPI_INT, MASTER, i, COMM, nullptr);
        } else {
            MPI_Recv(&rcvd_val_L, 1, MPI_INT, rank-1, i, COMM, nullptr);
        }

        // axis Y - receive from top neighbour
        if (rank - len_mat2 < 0) {
            MPI_Recv(&rcvd_val_T, 1, MPI_INT, MASTER, i+200, COMM, nullptr);
        } else {
            MPI_Recv(&rcvd_val_T, 1, MPI_INT, rank-len_mat2, i+200, COMM, nullptr);
        }
        // *************************


        // MULTIPLY and ADD #################
        my_cell += rcvd_val_L * rcvd_val_T;
        // ##########################

        // SEND ********************
        // axis X - send to right neighbour
        if ((rank+1) % len_mat2 != 0) {
            MPI_Send(&rcvd_val_L, 1, MPI_INT, rank+1, i, COMM);
        }

        // axis Y - sned to bottom neighbour
        if (rank + len_mat2 < size) {
            MPI_Send(&rcvd_val_T, 1, MPI_INT, rank+len_mat2, i+200, COMM);
        }
        // *************************
    }
    
    // Send result to MASTER
    MPI_Send(&my_cell, 1, MPI_INT, MASTER, 300, COMM);  

}

// parse input line - create vector of numbers
vector<int> parse_line(string line) {
    vector<int> result;
    string tmp = "";
    char space = ' ';

    line += space;

    while (line != "") {
        if (line[0] == space) {
            result.push_back(stoi(tmp));
            tmp = "";
        } else {
            tmp = tmp + line[0];
        }
        
        line.erase(line.begin());
    }

    return result;
}

// load input file into matrix
matrix load_input(string file_name) {
    string line;
    ifstream file (file_name);

    matrix input_matrix;
    int size_of_matrix;
    vector<int> tmp;

    if (file.is_open()) {
        getline(file, line);
        size_of_matrix = stoi(line);

        while (getline(file, line)) {
            tmp = parse_line(line);
            input_matrix.push_back(tmp);
        }

        file.close();
    }

    return input_matrix;
}

// send input values to top and left processors
// - these processors don't have left or top neighbour
// - they need to get input from master
void distribute_input(int size, matrix mat1, matrix mat2) {
    int length2 = (int)mat2.size();
    int length1 = (int)mat1.size();

    // first column
    for (int i=0; i < length1; i++) {
        for (int j=0; j < (int)mat1[0].size(); j++) {
            MPI_Send(&mat1[i][j], 1, MPI_INT, i*length2, j, COMM);
        }
    }

    // first row
    for (int i=0; i < length2; i++) {
        for (int j=0; j < (int)mat2[0].size(); j++) {
            MPI_Send(&mat2[i][j], 1, MPI_INT, i, j+200, COMM);
        }
    }

    
}

// send input length to all processors so they will know
// how long should they iterate trought the incomming messages
void distribute_input_length(int rank, int size, matrix mat1, matrix mat2, int* length_of_input, int* length_of_mat2) {
    int len;
    int len_mat2;
    if (rank == MASTER) {
        len = (int)mat1[0].size();      // cols in mat1
        len_mat2 = (int)mat2[0].size(); // cols in mat2
        // mat1 columns == mat2 rows
        for (int i=1; i < size; i++) {
            MPI_Send(&len, 1, MPI_INT, i, 100, COMM);
            MPI_Send(&len_mat2, 1, MPI_INT, i, 101, COMM);
        }

    } else {
        MPI_Recv(&len, 1, MPI_INT, MASTER, 100, COMM, nullptr);
        MPI_Recv(&len_mat2, 1, MPI_INT, MASTER, 101, COMM, nullptr);
    }

    *length_of_input = len;
    *length_of_mat2 = len_mat2;
}

// master process prints whole matrix to stdout
void handle_result(int size, int mat1_size, int mat2_size) {
    int rcv;
    cout << mat1_size << ":" << mat2_size << "\n";
    for (int i=0; i < size; i++) {
        MPI_Recv(&rcv, 1, MPI_INT, i, 300, COMM, nullptr);
        cout << rcv;

        if ((i+1) % mat2_size == 0) {
            cout << "\n";
        } else {
            cout << " ";
        }
    }

}

// *main*
int main(int argc, char** argv) {
    // init processes
    MPI_Init(&argc, &argv);
    int rank;
    int size;
    
    // get rank and size
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // init input matrixes
    matrix input_mat1;
    matrix input_mat2;

    // master loads input from files
    if (rank == MASTER) {

        input_mat1 = load_input(MAT1);
        input_mat2 = load_input(MAT2);
    }

    // master distribute sizes of matrixes to other processes
    int length_of_input;
    int length_of_mat2;     // cols in mat2
    distribute_input_length(rank, size, input_mat1, input_mat2, &length_of_input, &length_of_mat2);

    // master distribute input lines/rows to top/left processes
    if (rank== MASTER) {
        distribute_input(size, input_mat1, transpose_mat(input_mat2));
    }

    // MULTIPLICATION
    do_multiplication(rank, size, length_of_input, length_of_mat2);

    // handle result
    if (rank == MASTER) {
        handle_result(size, (int)input_mat1.size(), (int)input_mat2[0].size());
    }

    MPI_Finalize();

    return 0;
}
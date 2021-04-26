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


void print_line(mat_line line) {
    cout << "LINE:\n";
    for (int i=0; i < (int)line.size(); i++) {
        cout << line[i] << " ";
    }
    cout << endl;
}

void print_matrix(matrix mat) {
    cout << "MATRIX:\n";
    for (int i=0; i < (int)mat.size(); i++) {
        for (int j=0; j < (int)mat[0].size(); j++) {
            cout << mat[i][j] << " ";
        }
        cout << endl;
    }
}

void master_proc(mat_line mat1, mat_line mat2, int length) {
    // print_line(mat1);
    // print_line(mat2);
}

void first_x_proc() {

}

void first_y_proc() {

}

void other_procs() {

}

matrix last_proc() {

}


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

matrix do_multiplication(int rank, int size, matrix mat1, matrix mat2, int length) {
    int x = rank / 2;
    int y = rank % 2;
    

    if (rank == MASTER) {
        mat2 = transpose_mat(mat2);
        master_proc(mat1[0], mat2[0], length);
    // } else if (x == 0) {
    //     first_x_proc();
    // } else if (y == 0) {
    //     first_y_proc();
    // } else if (rank == size-1) {
    //     last_proc();
    // } else {
    //     other_procs();
    }

    return mat1;
}


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

    // PRINTS
    // for (int i=0; i < (int)result.size(); i++) {
    //     cout << result[i] << endl;
    // }

    return result;
}


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
            // cout << "TU: " << line << '\n';
            tmp = parse_line(line);
            input_matrix.push_back(tmp);
        }

        file.close();
    }

    return input_matrix;
}

void distribute_input(int size, matrix mat1, matrix mat2) {
    int length2 = (int)mat2.size();
    int length1 = (int)mat1.size();

    cout << length1 << " " << length2 << endl;

    print_matrix(mat1);
    print_matrix(mat2);

    // first row
    cout << "ROW:\n";
    for (int i=0; i < length2; i++) {
        for (int j=0; j < (int)mat2[0].size(); j++) {
            cout << mat2[i][j] << endl;
            MPI_Send(&mat2[i][j], 1, MPI_INT, i, j, COMM);
        }
    }

    // first column
    cout << "COL:\n";
    for (int i=0; i < length1; i++) {
        // cout << "TU: " << i << endl;
        for (int j=0; j < (int)mat1[0].size(); j++) {
            // cout << mat1[i][j] << " i:" << i << " j:" << j << endl;
            MPI_Send(&mat1[i][j], 1, MPI_INT, i*length2, j, COMM);
        }
    }
}

int distribute_input_length(int rank, int size, matrix mat) {
    int len;
    if (rank == MASTER) {
        len = (int)mat[0].size();
        // mat1 columns == mat2 rows
        for (int i=1; i < size; i++) {
            MPI_Send(&len, 1, MPI_INT, i, TAG, COMM);
        }

    } else {
        MPI_Recv(&len, 1, MPI_INT, MASTER, TAG, COMM, nullptr);
    }

    return len;
}


int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    matrix input_mat1;
    matrix input_mat2;

    if (rank == MASTER) {
        cout << endl;
        // TODO Check correct shape of matrixes
        input_mat1 = load_input(MAT1);
        input_mat2 = load_input(MAT2);
    }

    int length_of_intput = distribute_input_length(rank, size, input_mat1);

    if (rank== MASTER) {
        distribute_input(size, input_mat1, transpose_mat(input_mat2));
    }

    // MULTIPLICATION
    matrix result = do_multiplication(rank, size, input_mat1, input_mat2, length_of_intput);

    MPI_Finalize();

    return 0;
}
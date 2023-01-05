#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


#define N 9

using namespace std;


int solve_board[9][9] = { 0 };
int origin_board[9][9] = { 0 };

bool isSafe(int grid[N][N], int row,
                       int col, int num)
{
     
    // Check if we find the same num
    // in the similar row , we
    // return false
    for (int x = 0; x <= 8; x++)
        if (grid[row][x] == num)
            return false;
 
    // Check if we find the same num in
    // the similar column , we
    // return false
    for (int x = 0; x <= 8; x++)
        if (grid[x][col] == num)
            return false;
 
    // Check if we find the same num in
    // the particular 3*3 matrix,
    // we return false
    int startRow = row - row % 3,
            startCol = col - col % 3;
   
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (grid[i + startRow][j +
                            startCol] == num)
                return false;
 
    return true;
}
 
/* Takes a partially filled-in grid and attempts
to assign values to all unassigned locations in
such a way to meet the requirements for
Sudoku solution (non-duplication across rows,
columns, and boxes) */
bool solveSudoku(int grid[N][N], int row, int col)
{
    // Check if we have reached the 8th
    // row and 9th column (0
    // indexed matrix) , we are
    // returning true to avoid
    // further backtracking
    if (row == N - 1 && col == N)
        return true;
 
    // Check if column value  becomes 9 ,
    // we move to next row and
    //  column start from 0
    if (col == N) {
        row++;
        col = 0;
    }
   
    // Check if the current position of
    // the grid already contains
    // value >0, we iterate for next column
    if (grid[row][col] > 0)
        return solveSudoku(grid, row, col + 1);
 
    for (int num = 1; num <= N; num++)
    {
         
        // Check if it is safe to place
        // the num (1-9)  in the
        // given row ,col  ->we
        // move to next column
        if (isSafe(grid, row, col, num))
        {
             
           /* Assigning the num in
              the current (row,col)
              position of the grid
              and assuming our assigned
              num in the position
              is correct     */
            grid[row][col] = num;
           
            //  Checking for next possibility with next
            //  column
            if (solveSudoku(grid, row, col + 1))
                return true;
        }
       
        // Removing the assigned num ,
        // since our assumption
        // was wrong , and we go for
        // next assumption with
        // diff num value
        grid[row][col] = 0;
    }
    return false;
}

void sukodo_solver(string board) {
    // 81..457.9.746.83.2....39.84..51.2896.8.356.71.679.452.7.1.6.2..6.84.19.7.3259761.
    printf("Board: %s\n", board.c_str());
    int i = 0;
    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
            if (board[i] == '.') {
                solve_board[y][x] = 0;
            } else {
                solve_board[y][x] = board[i] - '0';
            }
            i++;
        }
    }

    memcpy(origin_board, solve_board, sizeof(solve_board));

    printf("Board:\n");
    for(int y = 0; y < 9; y++) {
        for(int x = 0; x < 9; x++) {
            printf("%d ", solve_board[y][x]);
        }
        printf("\n");
    }
    
    solveSudoku(solve_board, 0, 0);
}

int main() {
    // connect to unix domain socket
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("Error creating socket\n");
        return -1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, "/sudoku.sock");
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        printf("Error connecting to server\n");
        return -1;
    }

    char buf[1024];
    send(fd, "S", 1, 0);
    recv(fd, buf, 1024, 0);
    string board = buf;
    sukodo_solver(board.substr(4));
    string solution = "";
    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
            solution += solve_board[y][x] + '0';
        }
    }
    printf("Solution Generated: %s\n", solution.c_str());

    for (int y = 0; y < 9; y++) {
        for (int x = 0; x < 9; x++) {
            if (origin_board[y][x] != 0) {
                continue;
            }

            string msg = "V ";
            msg += to_string(y) + " ";
            msg += to_string(x) + " ";
            msg += to_string(solve_board[y][x]) + "\n";
            printf("Filling [%d][%d] = %d, msg = %s", y, x, solve_board[y][x], msg.c_str());
            send(fd, msg.c_str(), msg.length(), 0);
            
            // sleep for 100 ms
            recv(fd, buf, 1024, 0);
            usleep(50000);
        }
    }

    // check for the answer
    usleep(50000);
    send(fd, "C\n", 2, 0);
    recv(fd, buf, 1024, 0);

    return 0;
}
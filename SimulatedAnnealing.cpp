//#define _MAIN_
#ifdef _MAIN_

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
#include <chrono>

using namespace std;
using namespace std::chrono;

enum CellType { EMPTY, RESIDENTIAL, COMMERCIAL, OFFICE };

const int ROWS = 20;
const int COLS = 20;

// Score matrix for neighbor interactions
map<pair<CellType, CellType>, int> scoreMatrix = {
    {{RESIDENTIAL, RESIDENTIAL},  5},
    {{RESIDENTIAL, COMMERCIAL},   3},
    {{RESIDENTIAL, OFFICE},       1},
    {{COMMERCIAL, COMMERCIAL},    4},
    {{COMMERCIAL, OFFICE},        2},
    {{OFFICE, OFFICE},            6},
    // Symmetric scores
    {{COMMERCIAL, RESIDENTIAL},   3},
    {{OFFICE, RESIDENTIAL},       1},
    {{OFFICE, COMMERCIAL},        2}
};

// Function to print the grid
void printGrid(const vector<vector<CellType>>& grid) {
    for (const auto& row : grid) {
        for (const auto& cell : row) {
            char c;
            switch (cell) {
                case RESIDENTIAL: c = 'R'; break;
                case COMMERCIAL:  c = 'C'; break;
                case OFFICE:      c = 'O'; break;
                default:          c = '.'; break;
            }
            cout << c << ' ';
        }
        cout << endl;
    }
}

// Function to calculate total score
int calculateScore(const vector<vector<CellType>>& grid) {
    int totalScore = 0;
    int dx[] = { -1, 0, 1, 0 };
    int dy[] = { 0, -1, 0, 1 };

    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            CellType current = grid[i][j];
            if (current == EMPTY) continue;

            for (int dir = 0; dir < 4; ++dir) {
                int ni = i + dx[dir];
                int nj = j + dy[dir];
                if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS) {
                    CellType neighbor = grid[ni][nj];
                    if (neighbor != EMPTY) {
                        totalScore += scoreMatrix[{current, neighbor}];
                    }
                }
            }
        }
    }
    // Each pair counted twice
    return totalScore / 2;
}

// Simulated Annealing Optimization
void optimizeGrid(vector<vector<CellType>>& grid) {
    double temperature = 1000.0;
    double coolingRate = 0.003;
    int currentScore = calculateScore(grid);
    vector<vector<CellType>> bestGrid = grid;
    int bestScore = currentScore;

    while (temperature > 1) {
        // Generate neighboring solution by swapping two random cells
        vector<vector<CellType>> newGrid = grid;
        int x1 = rand() % ROWS;
        int y1 = rand() % COLS;
        int x2 = rand() % ROWS;
        int y2 = rand() % COLS;

        swap(newGrid[x1][y1], newGrid[x2][y2]);

        int newScore = calculateScore(newGrid);
        int deltaScore = newScore - currentScore;

        if (deltaScore > 0 || exp(deltaScore / temperature) > ((double)rand() / RAND_MAX)) {
            grid = newGrid;
            currentScore = newScore;

            if (currentScore > bestScore) {
                bestGrid = grid;
                bestScore = currentScore;
            }
        }

        temperature *= 1 - coolingRate;
    }

    grid = bestGrid;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    // Initialize grid with EMPTY cells
    vector<vector<CellType>> grid(ROWS, vector<CellType>(COLS, EMPTY));

    // Preset types (can be loaded from input if needed)
    // For simplicity, we'll randomly place some preset types
    grid[0][0] = RESIDENTIAL;
    grid[0][1] = COMMERCIAL;
    grid[1][0] = OFFICE;

    // Percentages for each type
    double residentialPerc = 0.4;
    double commercialPerc = 0.35;
    double officePerc = 0.25;

    int totalCells = ROWS * COLS - 3; // Adjusting for preset cells
    int residentialCells = residentialPerc * totalCells;
    int commercialCells = commercialPerc * totalCells;
    int officeCells = totalCells - residentialCells - commercialCells;

    vector<CellType> cells;
    for (int i = 0; i < residentialCells; ++i) cells.push_back(RESIDENTIAL);
    for (int i = 0; i < commercialCells; ++i)  cells.push_back(COMMERCIAL);
    for (int i = 0; i < officeCells; ++i)      cells.push_back(OFFICE);

    // Shuffle and assign to grid
    random_shuffle(cells.begin(), cells.end());
    int index = 0;
    for (int i = 0; i < ROWS && index < cells.size(); ++i) {
        for (int j = 0; j < COLS && index < cells.size(); ++j) {
            if (grid[i][j] == EMPTY) {
                grid[i][j] = cells[index++];
            }
        }
    }

    cout << "Initial Grid:" << endl;
    printGrid(grid);
    cout << "Initial Score: " << calculateScore(grid) << endl;

    // Measure computation time
    auto start = high_resolution_clock::now();

    optimizeGrid(grid);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "\nOptimized Grid:" << endl;
    printGrid(grid);
    cout << "Optimized Score: " << calculateScore(grid) << endl;

    cout << "Computation Time: " << duration.count() << " milliseconds" << endl;

    return 0;
}

#endif // _MAIN_
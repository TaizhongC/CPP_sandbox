/*
 * This program simulates a grid-based urban planning model using 
 * agent-based preferences and spatial constraints. It initializes 
 * a X by Y grid with predefined land use types (e.g., Residential, 
 * Office, Commercial Shops, Cafes, Roads) and calculates scores 
 * based on the proximity of agents (e.g., Residential, Office) to 
 * specific land use types. Simulated Annealing is used to optimize 
 * the placement of agents on the grid to maximize the total score.
 *
 * Key Features:
 * - Customisable grid with multiple land use and agent types.
 * - Proximity-based scoring using BFS for distance computation.
 * - Optimization using Simulated Annealing to improve grid scores.
 *
 * Complexity Analysis:
 * 1. Grid Initialization: O(n*m), where n and m are grid dimensions.
 * 2. Distance Map Computation (BFS): O(n*m*k), where k is the number 
 *    of fixed land use cells.
 * 3. Simulated Annealing Optimization: O(t * (n*m)), where t is the 
 *    number of iterations.
 * Overall Complexity: O(n*m*(k + t)), typically scalable for small grids.
 *
 * Author: Taizhong Chen | taizhong.chen@zaha-hadid.com
 * Date: 28.11.2024
 */

#define _MAIN_
#ifdef _MAIN_

#define LOG_PERF 1 && std::cout

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
#include <chrono>
#include <limits>
#include <iomanip>
#include <queue>

using namespace std;
using namespace std::chrono;

enum CellType
{
    EMPTY,
    RESIDENTIAL,
    OFFICE,
    COM_SHOP,
    COM_CAFE,
    TRANSPORT, // T
    PUBLIC,    // P
    LANDSCAPE, // L
    ROAD       // R
};

const int ROWS = 12;
const int COLS = 12;

// Land Use Types (fixed on the grid)
const vector<CellType> landUseTypes = {TRANSPORT, PUBLIC, LANDSCAPE, ROAD};

// Agent Types
const vector<CellType> agentTypes = {RESIDENTIAL, OFFICE, COM_SHOP, COM_CAFE};

// Preference arrays for each agent type towards each land use type
map<CellType, vector<float>> agentPreferences = {
    {RESIDENTIAL, {1, 2, 3, -5}},
    {OFFICE, {4, 1, 0, 2}},
    {COM_SHOP, {5, 3, 0, 3}},
    {COM_CAFE, {2, 4, 1, -1}}};

// Function to print the grid
void printGrid(const vector<vector<CellType>> &grid)
{
    for (const auto &row : grid)
    {
        for (const auto &cell : row)
        {
            char c;
            switch (cell)
            {
            case RESIDENTIAL:
                c = 'R';
                break;
            case OFFICE:
                c = 'O';
                break;
            case COM_SHOP:
                c = 'S';
                break;
            case COM_CAFE:
                c = 'C';
                break;
            case TRANSPORT:
                c = 'T';
                break;
            case PUBLIC:
                c = 'P';
                break;
            case LANDSCAPE:
                c = 'L';
                break;
            case ROAD:
                c = 'D';
                break;
            default:
                c = '.';
                break;
            }
            cout << c << ' ';
        }
        cout << endl;
    }
}

// Function to compute distance maps for each land use type
map<CellType, vector<vector<double>>> computeDistanceMaps(const vector<vector<CellType>> &grid)
{
    map<CellType, vector<vector<double>>> distanceMaps;

    for (CellType landUseType : landUseTypes)
    {
        vector<vector<double>> distanceMap(ROWS, vector<double>(COLS, numeric_limits<double>::max()));
        queue<pair<int, int>> q;

        // Initialize the queue with positions of the land use type
        for (int i = 0; i < ROWS; ++i)
        {
            for (int j = 0; j < COLS; ++j)
            {
                if (grid[i][j] == landUseType)
                {
                    distanceMap[i][j] = 0.0;
                    q.emplace(i, j);
                }
            }
        }

        // BFS to compute distances
        while (!q.empty())
        {
            int x = q.front().first;
            int y = q.front().second;
            q.pop();

            static const int dx[] = {-1, 0, 1, 0};
            static const int dy[] = {0, 1, 0, -1};

            for (int dir = 0; dir < 4; ++dir)
            {
                int nx = x + dx[dir];
                int ny = y + dy[dir];

                if (nx >= 0 && nx < ROWS && ny >= 0 && ny < COLS)
                {
                    if (distanceMap[nx][ny] > distanceMap[x][y] + 1.0)
                    {
                        distanceMap[nx][ny] = distanceMap[x][y] + 1.0;
                        q.emplace(nx, ny);
                    }
                }
            }
        }

        distanceMaps[landUseType] = distanceMap;
    }

    return distanceMaps;
}

// Function to calculate total score based on distances to land use types
double calculateScore(const vector<vector<CellType>> &grid, const map<CellType, vector<vector<double>>> &distanceMaps)
{
    double totalScore = 0.0;

    // Calculate score for each agent cell
    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            CellType agentCell = grid[i][j];
            if (find(agentTypes.begin(), agentTypes.end(), agentCell) == agentTypes.end())
            {
                continue; // Not an agent cell
            }

            const vector<float> &preferences = agentPreferences[agentCell];
            double agentScore = 0.0;

            // For each land use type
            for (size_t k = 0; k < landUseTypes.size(); ++k)
            {
                CellType landUseType = landUseTypes[k];
                float preference = preferences[k];

                double distance = distanceMaps.at(landUseType)[i][j];

                // Avoid division by zero and check if distance is finite
                if (distance > 0.0 && distance < numeric_limits<double>::max())
                {
                    agentScore += preference / distance;
                }
            }

            totalScore += agentScore;
        }
    }

    return totalScore;
}

// Simulated Annealing Optimisation
void optimiseGrid(
    vector<vector<CellType>> &grid,
    const map<CellType, vector<vector<double>>> &distanceMaps,
    double _temperature = 1000.0,
    double _cooldown = 1.0,
    double _coolingRate = 0.003)
{
    double temperature = _temperature;
    double cooldown = _cooldown;
    double coolingRate = _coolingRate;
    double currentScore = calculateScore(grid, distanceMaps);
    vector<vector<CellType>> bestGrid = grid;
    double bestScore = currentScore;

    int iteration = 0;

    auto optimisationStart = high_resolution_clock::now();

    // Logging headers for performance analysis
    LOG_PERF << "| Iteration | Temperature | Current Score | Best Score | Time (ms) |\n";
    LOG_PERF << "|-----------|-------------|---------------|------------|-----------|\n";

    while (temperature > cooldown)
    {
        // Generate neighboring solution by swapping two random agent cells
        vector<vector<CellType>> newGrid = grid;
        int x1, y1, x2, y2;

        // Find first agent cell to swap
        do
        {
            x1 = rand() % ROWS;
            y1 = rand() % COLS;
        } while (find(agentTypes.begin(), agentTypes.end(), grid[x1][y1]) == agentTypes.end());

        // Find second agent cell to swap
        do
        {
            x2 = rand() % ROWS;
            y2 = rand() % COLS;
        } while (find(agentTypes.begin(), agentTypes.end(), grid[x2][y2]) == agentTypes.end());

        // Swap the agent cells
        swap(newGrid[x1][y1], newGrid[x2][y2]);

        double newScore = calculateScore(newGrid, distanceMaps);
        double deltaScore = newScore - currentScore;

        /*
        The probability of accepting a new configuration is determined by the Metropolis criterion:

        𝑃 = 𝑒^(Δ𝑆/𝑇)

        ΔS: Change in score (new score minus current score).

        Positive ΔS: The new configuration is better and is always accepted.
        Negative ΔS: The new configuration is worse; acceptance depends on the temperature.
        T: Current temperature.
        */
        if (deltaScore > 0 || exp(deltaScore / temperature) > ((double)rand() / RAND_MAX))
        {
            grid = newGrid;
            currentScore = newScore;

            if (currentScore > bestScore)
            {
                bestGrid = grid;
                bestScore = currentScore;
            }
        }

        // Logging for performance analysis every 100 iterations
        if (iteration % 100 == 0)
        {
            auto now = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(now - optimisationStart);
            LOG_PERF << "| " << setw(9) << iteration
                     << " | " << setw(11) << fixed << setprecision(2) << temperature
                     << " | " << setw(13) << currentScore
                     << " | " << setw(10) << bestScore
                     << " | " << setw(9) << duration.count()
                     << " |\n";
        }

        iteration++;
        temperature *= 1 - coolingRate;
    }

    grid = bestGrid;
}

// Generate grid with random land use and agents based on percentages
void generateGrid_random(vector<vector<CellType>> &grid, map<CellType, double> agentPercentages)
{
    // Initialize grid with EMPTY cells
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            grid[i][j] = EMPTY;

    // Set preset land use types (fixed cells)
    int numLandUseCells = (ROWS * COLS) / 5; // Adjust as needed

    for (CellType landUseType : landUseTypes)
    {
        for (int i = 0; i < numLandUseCells / landUseTypes.size(); ++i)
        {
            int x, y;
            do
            {
                x = rand() % ROWS;
                y = rand() % COLS;
            } while (grid[x][y] != EMPTY);
            grid[x][y] = landUseType;
        }
    }

    // Calculate the number of each agent type to place
    int totalCells = ROWS * COLS;
    int fixedCells = 0;
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            if (grid[i][j] != EMPTY)
                fixedCells++;

    int availableCells = totalCells - fixedCells;
    map<CellType, int> agentCounts;
    for (CellType agentType : agentTypes)
    {
        agentCounts[agentType] = static_cast<int>(agentPercentages[agentType] * availableCells);
    }

    // Fill the grid with agent types
    vector<CellType> agentsToPlace;
    for (CellType agentType : agentTypes)
    {
        agentsToPlace.insert(agentsToPlace.end(), agentCounts[agentType], agentType);
    }

    // Shuffle and assign to grid
    random_shuffle(agentsToPlace.begin(), agentsToPlace.end());
    int index = 0;
    for (int i = 0; i < ROWS && index < agentsToPlace.size(); ++i)
    {
        for (int j = 0; j < COLS && index < agentsToPlace.size(); ++j)
        {
            if (grid[i][j] == EMPTY)
            {
                grid[i][j] = agentsToPlace[index++];
            }
        }
    }
}

// Generate grid based on input grid and assign agents to empty cells
void generateGrid_input(vector<vector<CellType>> &grid, map<CellType, double> agentPercentages)
{
    // Count fixed cells
    int totalCells = ROWS * COLS;
    int fixedCells = 0;
    vector<pair<int, int>> emptyCells;
    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            if (grid[i][j] != EMPTY && find(landUseTypes.begin(), landUseTypes.end(), grid[i][j]) == landUseTypes.end())
            {
                // If cell is not empty and not a land use type, consider it fixed
                fixedCells++;
            }
            else if (grid[i][j] == EMPTY)
            {
                emptyCells.emplace_back(i, j);
            }
        }
    }

    int availableCells = emptyCells.size();
    map<CellType, int> agentCounts;
    for (CellType agentType : agentTypes)
    {
        agentCounts[agentType] = static_cast<int>(agentPercentages[agentType] * availableCells);
    }

    // Fill the empty cells with agent types
    vector<CellType> agentsToPlace;
    for (CellType agentType : agentTypes)
    {
        agentsToPlace.insert(agentsToPlace.end(), agentCounts[agentType], agentType);
    }

    // In case of rounding errors, fill remaining cells with random agent types
    while (agentsToPlace.size() < availableCells)
    {
        agentsToPlace.push_back(agentTypes[rand() % agentTypes.size()]);
    }

    // Shuffle and assign to grid
    random_shuffle(agentsToPlace.begin(), agentsToPlace.end());
    int index = 0;
    for (auto &cell : emptyCells)
    {
        grid[cell.first][cell.second] = agentsToPlace[index++];
    }
}

int main()
{
    srand(static_cast<unsigned int>(time(0)));

    // Initialize grid
    vector<vector<CellType>> grid(ROWS, vector<CellType>(COLS, EMPTY));

    // Agent percentages (must sum to 1.0)
    map<CellType, double> agentPercentages = {
        {RESIDENTIAL, 0.45},
        {OFFICE, 0.25},
        {COM_SHOP, 0.20},
        {COM_CAFE, 0.10}};

    // Choose which grid generation method to use
    // Uncomment one of the following:

    // Method 1: Generate random grid
    // generateGrid_random(grid, agentPercentages);

    // Method 2: Generate grid based on input
    // For demonstration, let's define an input grid with some land use types
    /*
    // Define input grid with some land use types
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            grid[i][j] = EMPTY;

    // Set some land use types in specific positions
    grid[0][0] = TRANSPORT;
    grid[5][5] = PUBLIC;
    grid[10][10] = LANDSCAPE;
    grid[15][15] = ROAD;
    */

    grid = {
        {EMPTY, EMPTY, EMPTY, PUBLIC, PUBLIC, PUBLIC, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, ROAD, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, LANDSCAPE, LANDSCAPE, EMPTY, EMPTY, ROAD, EMPTY, EMPTY, EMPTY, LANDSCAPE, EMPTY, EMPTY},
        {EMPTY, LANDSCAPE, LANDSCAPE, EMPTY, EMPTY, ROAD, EMPTY, TRANSPORT, EMPTY, LANDSCAPE, EMPTY, PUBLIC},
        {EMPTY, LANDSCAPE, LANDSCAPE, EMPTY, EMPTY, ROAD, EMPTY, EMPTY, EMPTY, LANDSCAPE, EMPTY, PUBLIC},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, ROAD, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, PUBLIC},
        {EMPTY, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, ROAD, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, TRANSPORT, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, TRANSPORT, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, PUBLIC, PUBLIC, PUBLIC, EMPTY, EMPTY, EMPTY, EMPTY}};

    generateGrid_input(grid, agentPercentages);

    cout << "Initial Grid:" << endl;
    printGrid(grid);

    // Compute distance maps
    auto distanceStart = high_resolution_clock::now();
    map<CellType, vector<vector<double>>> distanceMaps = computeDistanceMaps(grid);
    auto distanceEnd = high_resolution_clock::now();
    auto distanceDuration = duration_cast<milliseconds>(distanceEnd - distanceStart);
    cout << "Distance Maps Computation Time: " << distanceDuration.count() << " milliseconds" << endl;

    auto initialScoreStart = high_resolution_clock::now();
    double initialScore = calculateScore(grid, distanceMaps);
    auto initialScoreEnd = high_resolution_clock::now();
    auto initialScoreDuration = duration_cast<milliseconds>(initialScoreEnd - initialScoreStart);
    cout << "Initial Score: " << initialScore << endl;
    cout << "Initial Score Computation Time: " << initialScoreDuration.count() << " milliseconds" << endl;

    auto optimisationStart = high_resolution_clock::now();

    optimiseGrid(grid, distanceMaps, 1000, 0.1, 0.001);

    auto optimisationEnd = high_resolution_clock::now();
    auto optimisationDuration = duration_cast<milliseconds>(optimisationEnd - optimisationStart);

    cout << "\nOptimised Grid:" << endl;
    printGrid(grid);

    auto finalScoreStart = high_resolution_clock::now();
    double finalScore = calculateScore(grid, distanceMaps);
    auto finalScoreEnd = high_resolution_clock::now();
    auto finalScoreDuration = duration_cast<milliseconds>(finalScoreEnd - finalScoreStart);
    cout << "Optimised Score: " << finalScore << endl;
    cout << "Optimised Score Computation Time: " << finalScoreDuration.count() << " milliseconds" << endl;

    cout << "Optimisation Time: " << optimisationDuration.count() << " milliseconds" << endl;

    return 0;
}

#endif

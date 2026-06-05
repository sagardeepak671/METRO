#include <iostream>
#include <vector>
#include <set>
#include <utility>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <tuple>
#include <array>
#include <chrono>
#include <algorithm>
#include <functional>
#define endl '\n'
using namespace std;
using namespace std::chrono;

struct PROBLEM{
    int senario;
    int N, M, K, J, P=0;
    vector<vector<int>> metro_lines; // (x1,y1,x2,y2)
    vector<vector<int>> popular_cells; // (x,y)
};

PROBLEM prob;
int N,M,K,J,P=0;
vector<vector<int>> grid;
vector<vector<bool>> visited;
vector<vector<bool>> is_popular; // for scenario 2
vector<string> decoded_paths; // Store paths globally to use in both PRINT_DEBUG and WRITE_OUTPUT
vector<int> cnt_popular_city_in_line;
map<int,vector<int>> var_to_xyk; // variable number -> (x,y,k)

void read_PROBLEM(string filename){
    filename+=".city";
    ifstream file(filename);
    if(!file.is_open()){cerr << "Error opening file: " << filename << endl; exit(1);}
    string line;
    getline(file, line);
    prob.senario = stoi(line);
    getline(file, line);
    istringstream iss(line);
    iss>>prob.M>>prob.N>>prob.K>>prob.J;
    M=prob.M; N=prob.N; K=prob.K; J=prob.J; 
    if(prob.senario==2){
        iss>>prob.P;
        P=prob.P;
    }
    // Reserve capacity to avoid reallocations
    prob.metro_lines.reserve(prob.K);
    for(int i=0;i<prob.K;i++){
        getline(file,line);
        istringstream iss(line);
        int x1,y1,x2,y2;
        iss>>y1>>x1>>y2>>x2;
        prob.metro_lines.push_back({x1,y1,x2,y2});
    }
    if(prob.senario==2){
        prob.popular_cells.reserve(prob.P);
        getline(file,line);
        istringstream iss_pop(line);
        for(int i=0;i<prob.P;i++){
            int x,y;
            iss_pop>>y>>x;
            prob.popular_cells.push_back({x,y});
        }
    }
    file.close();
}

bool read_VARIABLES_and_FILL_GRID(string filename){
    filename+=".satoutput";  // Assignment requires .satoutput format
    ifstream file(filename);
    if(!file.is_open()){cerr << "Error opening file: " << filename << endl; exit(1);}
    string line;
    if(!(file>>line)){cerr << "Error reading SAT output file: " << filename << endl; exit(1);}
    if(line!="SAT"){ 
        file.close();
        return false;
    }
    int MAX_VAR = N*M*K;
    int var;
    while(file>>var){
        if(var<=0)continue;
        if(var>MAX_VAR)continue; 
        map<int, vector<int>>::iterator it = var_to_xyk.find(var);
        if(it == var_to_xyk.end())continue;
        const vector<int>& coords = it->second;
        int x = coords[0];
        int y = coords[1];
        int k = coords[2];
        grid[x][y]=k+1; // color with k+1
    }
    file.close();
    return true;
}

void PRINT_DEBUG(){
    // ANSI color codes
    string RESET = "\033[0m";
    string RED = "\033[31m";
    string GREEN = "\033[32m";
    string YELLOW = "\033[33m";
    string BLUE = "\033[34m";
    string MAGENTA = "\033[35m";
    string CYAN = "\033[36m";
    string WHITE = "\033[37m";
    string BRIGHT_RED = "\033[91m";
    string BRIGHT_GREEN = "\033[92m";
    string BRIGHT_YELLOW = "\033[93m";
    string BRIGHT_BLUE = "\033[94m";
    string BOLD = "\033[1m";
    string DIM = "\033[2m";
    cout<<"AFTER DECODER OUTPUT:"<<endl;

    // ANSI color palette mapping index -> color (0 = empty)
    vector<string> colors = {WHITE, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW, BRIGHT_BLUE};

    cout << "\n=== FULL SAT SOLUTION GRID (all colored cells) ===" << endl;
    if(prob.senario == 2){
        cout << "Popular cells are marked with ! suffix (e.g., 5!)" << endl;
    }
    // Print grid with colors. Treat any value <= 0 as empty (0).
    for(int i = 0; i < N; i++){
        for(int j = 0; j < M; j++){
            int color_idx = grid[i][j];
            bool is_pop = (prob.senario == 2 && is_popular[i][j]);
            
            if(color_idx <= 0){
                if(is_pop){
                    cout << "\033[37m" << " 0!"; // Popular empty cell with ! mark
                } else {
                    cout << "\033[37m" << "  0"; // Gray for empty cells, 3 chars width
                }
            } else {
                int color_index = (color_idx <= 10) ? color_idx : (color_idx % 10) + 1;
                if(color_index < 0 || color_index >= (int)colors.size()) color_index = 1;
                
                if(is_pop){
                    // Popular cells: show as "5!" with proper spacing
                    if(color_idx < 10){
                        cout << colors[color_index] << " " << color_idx << "!";
                    } else {
                        cout << colors[color_index] << color_idx << "!";
                    }
                } else {
                    // Regular cells: show with width 3 for alignment
                    if(color_idx < 10){
                        cout << colors[color_index] << "  " << color_idx;
                    } else {
                        cout << colors[color_index] << " " << color_idx;
                    }
                }
            }
        }
        cout << "\033[0m" << endl; // Reset color at end of line
    }
    cout << "\033[0m" << endl; // Final reset
    
    // Print color legend
    cout << "Color Legend:" << endl;
    cout << "\033[37m0\033[0m = Empty cell" << endl;
    for(int k = 1; k <= K; k++){
        int color_index = (k <= 10) ? k : (k % 10) + 1;
        if(color_index < 0 || color_index >= (int)colors.size()) color_index = 1;
        cout << colors[color_index] << k << "\033[0m = Metro Line " << k;
        if((int)prob.metro_lines.size() >= k){
            int sx = prob.metro_lines[k-1][0];
            int sy = prob.metro_lines[k-1][1];
            int ex = prob.metro_lines[k-1][2];
            int ey = prob.metro_lines[k-1][3];
            cout << " (" << sx << "," << sy << ") -> (" << ex << "," << ey << ")";
        }
        cout << endl;
    }
    cout << endl;

    // Now create a clean grid showing ONLY the reconstructed paths
    cout << "\n=== RECONSTRUCTED PATH GRID (cleaned, only actual paths) ===" << endl;
    
    // Use the globally stored decoded paths
    vector<string>& paths = decoded_paths;
    
    // Create a clean grid showing only path cells
    vector<vector<int>> cleanGrid(N, vector<int>(M, 0));
    
    // Direction mapping: L=(-1,0), R=(1,0), U=(0,-1), D=(0,1) for columns/rows
    // L/R change y (columns), U/D change x (rows)
    
    for(int k=0; k<K; k++){
        int sx = prob.metro_lines[k][0];
        int sy = prob.metro_lines[k][1];
        
        // Mark start cell in clean grid
        cleanGrid[sx][sy] = k + 1;
        
        if(k >= (int)paths.size()) continue;
        string path = paths[k];
        
        // Follow the path and mark all cells in clean grid
        int x = sx, y = sy;
        for(char move : path){
            if(move == '0') break; // end of path marker
            
            // Apply move
            if(move == 'L'){
                y--; // left: decrease column
            } else if(move == 'R'){
                y++; // right: increase column
            } else if(move == 'U'){
                x--; // up: decrease row
            } else if(move == 'D'){
                x++; // down: increase row
            }
        
            if(x >= 0 && x < N && y >= 0 && y < M){
                cleanGrid[x][y] = k + 1;
            }
        }
    }
     
    // Print the cleaned grid (only shows cells in reconstructed paths)
    if(prob.senario == 2){
        cout << "Popular cells in paths are marked with !" << endl;
    }
    for(int i = 0; i < N; i++){
        for(int j = 0; j < M; j++){
            int color_idx = cleanGrid[i][j];
            bool is_pop = (prob.senario == 2 && is_popular[i][j]);
            
            if(color_idx <= 0){
                if(is_pop){
                    cout << "\033[37m" << " .!"; // Popular empty cell with ! mark
                } else {
                    cout << "\033[37m" << "  ."; // dot for empty cells, 3 chars width
                }
            } else {
                int color_index = (color_idx <= 10) ? color_idx : (color_idx % 10) + 1;
                if(color_index < 0 || color_index >= (int)colors.size()) color_index = 1;
                
                if(is_pop){
                    // Popular cells in path: show as "5!" with proper spacing
                    if(color_idx < 10){
                        cout << colors[color_index] << " " << color_idx << "!";
                    } else {
                        cout << colors[color_index] << color_idx << "!";
                    }
                } else {
                    // Regular cells: show with width 3 for alignment
                    if(color_idx < 10){
                        cout << colors[color_index] << "  " << color_idx;
                    } else {
                        cout << colors[color_index] << " " << color_idx;
                    }
                }
            }
        }
        cout << "\033[0m" << endl;
    }
    cout << "\033[0m" << endl;
    cout << "Note: This grid shows only cells that are part of valid reconstructed paths from start to end.\n" << endl;

}
 

// Reconstruct each metro's path as a sequence of L/R/U/D ending with '0'.
#include <deque>
struct Prev { int px, py, pdir; char move; };

vector<string> SMART_DECODE_PATH(){
    vector<string> result;
    const int INF = 1e9;
    // dir order: 0=L,1=R,2=U,3=D
    int dx[4] = {0, 0, -1, 1};
    int dy[4] = {-1, 1, 0, 0};
    char dch[4] = {'L','R','U','D'};

    for(int k=0;k<K;k++){
        int sx = prob.metro_lines[k][0];
        int sy = prob.metro_lines[k][1];
        int ex = prob.metro_lines[k][2];
        int ey = prob.metro_lines[k][3];

        // trivial case
        if(sx==ex && sy==ey){
            result.push_back(string("0"));
            continue;
        }

        // dist[x][y][dir]
        vector<vector<array<int,4>>> dist(N, vector<array<int,4>>(M));
        vector<vector<array<Prev,4>>> parent(N, vector<array<Prev,4>>(M));
        for(int i=0;i<N;i++) for(int j=0;j<M;j++) for(int d=0;d<4;d++) dist[i][j][d]=INF;

        deque<tuple<int,int,int>> dq; // x,y,dir

        // initialize: from start, try all 4 neighbor directions that stay inside and belong to same color
        for(int d=0; d<4; ++d){
            int nx = sx + dx[d];
            int ny = sy + dy[d];
            if(nx<0 || nx>=N || ny<0 || ny>=M) continue;
            if(grid[nx][ny] != k+1) continue;
            // stepping from start to first neighbor does NOT count as a turn
            if(dist[nx][ny][d] > 0){
                dist[nx][ny][d] = 0;
                parent[nx][ny][d] = {sx, sy, -1, dch[d]}; // parent dir -1 means start
                dq.emplace_front(nx, ny, d);
            }
        }

        bool found = false;
        int bestDirAtTarget = -1;
        int bestTurns = INF;

        while(!dq.empty()){
            tuple<int,int,int> front = dq.front(); dq.pop_front();
            int x = get<0>(front);
            int y = get<1>(front);
            int dir = get<2>(front);
            int curTurns = dist[x][y][dir];
            // early exit possible: if curTurns > bestTurns skip
            if(curTurns > bestTurns) continue;

            if(x==ex && y==ey){
                // reached destination; record if better
                if(curTurns < bestTurns){
                    bestTurns = curTurns;
                    bestDirAtTarget = dir;
                    found = true;
                }
                // do not break immediately; there might be other dir states at same cell with fewer turns pending
                continue;
            }

            // explore neighbors
            for(int nd=0; nd<4; ++nd){
                int nx = x + dx[nd];
                int ny = y + dy[nd];
                if(nx<0 || nx>=N || ny<0 || ny>=M) continue;
                if(grid[nx][ny] != k+1) continue; // keep on same colored cells only
                int cost = (nd == dir) ? 0 : 1;
                int newTurns = curTurns + cost;
                if(newTurns < dist[nx][ny][nd]){
                    dist[nx][ny][nd] = newTurns;
                    parent[nx][ny][nd] = {x, y, dir, dch[nd]}; // move is nd
                    if(cost == 0) dq.emplace_front(nx, ny, nd);
                    else dq.emplace_back(nx, ny, nd);
                }
            }
        } // end BFS

        if(!found){
            // no path found; push empty path (later converted to '0')
            result.push_back(string(""));
            continue;
        }

        // reconstruct path: start from (ex,ey,bestDirAtTarget)
        string pathRev; // we will build moves from end to start
        int cx = ex, cy = ey, cdir = bestDirAtTarget;
        while(true){
            Prev p = parent[cx][cy][cdir];
            // p.move is the move we used TO GET to (cx,cy) (i.e., from p.px,p.py to cx,cy)
            // push that char into reverse path
            pathRev.push_back(p.move);
            if(p.pdir == -1){
                // parent is start cell; p.px,p.py is start coord => done
                break;
            }
            // step back
            int px = p.px;
            int py = p.py;
            int pd = p.pdir;
            cx = px; cy = py; cdir = pd;
        }
        // reverse the moves and add trailing '0'
        reverse(pathRev.begin(), pathRev.end());
        pathRev.push_back('0');
        result.push_back(pathRev);
    } // for each k

    return result;
}
// DECODING PATH OPPOSITE OF ENCODING PATH
// Search for valid paths with increasing turn counts (0, 1, 2, ..., J)
// For scenario 2: ensure path goes through popular cells

vector<string> DECODE_PATH_REVERSE_WAY(){
    vector<string> result;
    result.reserve(K);
    
    int dx[4] = {0, 0, -1, 1};  // L, R, U, D
    int dy[4] = {-1, 1, 0, 0};
    char dch[4] = {'L', 'R', 'U', 'D'};
    
    for(int k = 0; k < K; k++){
        int sx = prob.metro_lines[k][0];
        int sy = prob.metro_lines[k][1];
        int ex = prob.metro_lines[k][2];
        int ey = prob.metro_lines[k][3];
        
        // Trivial case: start == end
        if(sx == ex && sy == ey){
            result.push_back("0");
            continue;
        }
        
        // Count required popular cells for this line in scenario 2
        int required_popular = 0;
        if(prob.senario == 2){
            for(const auto& cell : prob.popular_cells){
                int px = cell[0], py = cell[1];
                if(grid[px][py] == k + 1){
                    required_popular++;
                }
            }
        }
        
        bool found = false;
        string best_path = "";
        
        // Try paths with increasing turns: 0, 1, 2, ..., J
        for(int max_turns = 0; max_turns <= J && !found; max_turns++){
            // DFS with turn limit
            vector<vector<bool>> vis(N, vector<bool>(M, false));
            
            function<bool(int, int, int, int, string, int)> dfs = 
                [&](int x, int y, int prev_dir, int turns, string path, int popular_count) -> bool {
                
                // Reached destination
                if(x == ex && y == ey){
                    // Check if we have all required popular cells (scenario 2)
                    if(prob.senario == 2 && popular_count < required_popular){
                        return false;
                    }
                    best_path = path + "0";
                    return true;
                }
                
                // OPTIMIZATION: Early termination if too many turns already
                if(turns > max_turns) return false;
                
                // OPTIMIZATION: Manhattan distance heuristic - impossible to reach
                int dist = abs(ex - x) + abs(ey - y);
                if(dist == 0) return false; // shouldn't happen, but safety check
                
                // Try all 4 directions
                for(int d = 0; d < 4; d++){
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    
                    // Boundary check
                    if(nx < 0 || nx >= N || ny < 0 || ny >= M) continue;
                    
                    // Must be same color (metro line)
                    if(grid[nx][ny] != k + 1) continue;
                    
                    // Avoid revisiting (for this DFS path)
                    if(vis[nx][ny]) continue;
                    
                    // Calculate turn cost
                    int new_turns = turns;
                    if(prev_dir != -1 && prev_dir != d){
                        new_turns++;
                    }
                    
                    // OPTIMIZATION: Prune if exceeds turn limit
                    if(new_turns > max_turns) continue;
                    
                    // Check if this cell is popular
                    int new_popular = popular_count;
                    if(prob.senario == 2 && is_popular[nx][ny]){
                        new_popular++;
                    }
                    
                    // Mark visited
                    vis[nx][ny] = true;
                    
                    // Recurse
                    if(dfs(nx, ny, d, new_turns, path + dch[d], new_popular)){
                        vis[nx][ny] = false; // backtrack (though we're returning true)
                        return true;
                    }
                    
                    // Backtrack
                    vis[nx][ny] = false;
                }
                
                return false;
            };
            
            // Start DFS from start position
            vis[sx][sy] = true;
            int initial_popular = (prob.senario == 2 && is_popular[sx][sy]) ? 1 : 0;
            
            // Try starting in each direction from start
            for(int d = 0; d < 4; d++){
                int nx = sx + dx[d];
                int ny = sy + dy[d];
                
                if(nx < 0 || nx >= N || ny < 0 || ny >= M) continue;
                if(grid[nx][ny] != k + 1) continue;
                
                vis[nx][ny] = true;
                int pop_count = initial_popular;
                if(prob.senario == 2 && is_popular[nx][ny]) pop_count++;
                
                if(dfs(nx, ny, d, 0, string(1, dch[d]), pop_count)){
                    found = true;
                    break;
                }
                vis[nx][ny] = false;
            }
        }
        
        // If no path found, return empty (will be converted to "0")
        if(!found){
            result.push_back("");
        } else {
            result.push_back(best_path);
        }
    }
    
    return result;
}

void WRITE_OUTPUT(string filename){
    filename += ".metromap"; 
    ofstream out(filename);
    if(!out.is_open()){ cerr<<"Error opening output file"<<endl; return; }
    
    // OPTIMIZATION: Set buffer for faster writing
    char write_buffer[65536];
    out.rdbuf()->pubsetbuf(write_buffer, sizeof(write_buffer));
    
    for(const auto& s: decoded_paths){
        string path = s;
        if(!path.empty() && path.back() == '0') path.pop_back();
        if(path.empty()){out << "0\n";continue;}
        for(char c : path){out << c << ' ';}
        out << "0\n";
    }
    out.close();
    cout<<"Metromap written to " << filename << " (in original order)\n";
}


vector<string> NEW_PATH(){
    // Find the longest valid path for each metro line (Scenario 1)
    // Strategy: Use BFS/DFS to find all possible paths, return the longest one
    vector<string> result;
    result.reserve(K);
    
    int dx[4] = {0, 0, -1, 1};  // L, R, U, D
    int dy[4] = {-1, 1, 0, 0};
    char dch[4] = {'L', 'R', 'U', 'D'};
    
    for(int k = 0; k < K; k++){
        int sx = prob.metro_lines[k][0];
        int sy = prob.metro_lines[k][1];
        int ex = prob.metro_lines[k][2];
        int ey = prob.metro_lines[k][3];
        
        // Trivial case: start == end
        if(sx == ex && sy == ey){
            result.push_back("0");
            continue;
        }
        
        // Find longest path with DFS
        vector<vector<bool>> vis(N, vector<bool>(M, false));
        string best_path = "";
        int max_length = 0;
        
        function<void(int, int, int, int, string, int)> dfs = 
            [&](int x, int y, int prev_dir, int turns, string path, int length) {
            
            // Reached destination
            if(x == ex && y == ey){
                if(length > max_length){
                    max_length = length;
                    best_path = path + "0";
                }
                return;
            }
            
            // Prune if too many turns
            if(turns > J) return;
            
            // Try all 4 directions
            for(int d = 0; d < 4; d++){
                int nx = x + dx[d];
                int ny = y + dy[d];
                
                // Boundary check
                if(nx < 0 || nx >= N || ny < 0 || ny >= M) continue;
                
                // Must be same color (metro line)
                if(grid[nx][ny] != k + 1) continue;
                
                // Avoid revisiting
                if(vis[nx][ny]) continue;
                
                // Calculate turn cost
                int new_turns = turns;
                if(prev_dir != -1 && prev_dir != d){
                    new_turns++;
                }
                
                // Prune if exceeds turn limit
                if(new_turns > J) continue;
                
                // Mark visited
                vis[nx][ny] = true;
                
                // Recurse
                dfs(nx, ny, d, new_turns, path + dch[d], length + 1);
                
                // Backtrack
                vis[nx][ny] = false;
            }
        };
        
        // Start DFS from start position
        vis[sx][sy] = true;
        
        // Try starting in each direction from start
        for(int d = 0; d < 4; d++){
            int nx = sx + dx[d];
            int ny = sy + dy[d];
            
            if(nx < 0 || nx >= N || ny < 0 || ny >= M) continue;
            if(grid[nx][ny] != k + 1) continue;
            
            vis[nx][ny] = true;
            dfs(nx, ny, d, 0, string(1, dch[d]), 1);
            vis[nx][ny] = false;
        }
        
        // If no path found, use SMART_DECODE_PATH as fallback
        if(best_path.empty()){
            // Fallback to shortest path (0-1 BFS)
            const int INF = 1e9;
            struct Prev { int px, py, pdir; char move; };
            
            vector<vector<array<int,4>>> dist(N, vector<array<int,4>>(M));
            vector<vector<array<Prev,4>>> parent(N, vector<array<Prev,4>>(M));
            for(int i=0;i<N;i++) for(int j=0;j<M;j++) for(int dd=0;dd<4;dd++) dist[i][j][dd]=INF;
            
            deque<tuple<int,int,int>> dq;
            
            for(int dd=0; dd<4; ++dd){
                int nx = sx + dx[dd];
                int ny = sy + dy[dd];
                if(nx<0 || nx>=N || ny<0 || ny>=M) continue;
                if(grid[nx][ny] != k+1) continue;
                if(dist[nx][ny][dd] > 0){
                    dist[nx][ny][dd] = 0;
                    parent[nx][ny][dd] = {sx, sy, -1, dch[dd]};
                    dq.emplace_front(nx, ny, dd);
                }
            }
            
            bool found = false;
            int bestDirAtTarget = -1;
            int bestTurns = INF;
            
            while(!dq.empty()){
                tuple<int,int,int> front = dq.front(); 
                dq.pop_front();
                int x = get<0>(front);
                int y = get<1>(front);
                int dir = get<2>(front);
                int curTurns = dist[x][y][dir];
                if(curTurns > bestTurns) continue;
                
                if(x==ex && y==ey){
                    if(curTurns < bestTurns){
                        bestTurns = curTurns;
                        bestDirAtTarget = dir;
                        found = true;
                    }
                    continue;
                }
                
                for(int nd=0; nd<4; ++nd){
                    int nx = x + dx[nd];
                    int ny = y + dy[nd];
                    if(nx<0 || nx>=N || ny<0 || ny>=M) continue;
                    if(grid[nx][ny] != k+1) continue;
                    int cost = (nd == dir) ? 0 : 1;
                    int newTurns = curTurns + cost;
                    if(newTurns < dist[nx][ny][nd]){
                        dist[nx][ny][nd] = newTurns;
                        parent[nx][ny][nd] = {x, y, dir, dch[nd]};
                        if(cost == 0) dq.emplace_front(nx, ny, nd);
                        else dq.emplace_back(nx, ny, nd);
                    }
                }
            }
            
            if(found){
                string pathRev;
                int cx = ex, cy = ey, cdir = bestDirAtTarget;
                while(true){
                    Prev p = parent[cx][cy][cdir];
                    pathRev.push_back(p.move);
                    if(p.pdir == -1) break;
                    cx = p.px; cy = p.py; cdir = p.pdir;
                }
                reverse(pathRev.begin(), pathRev.end());
                pathRev.push_back('0');
                best_path = pathRev;
            } else {
                best_path = "0";
            }
        }
        
        result.push_back(best_path);
    }
    
    return result;
}


int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    
    high_resolution_clock::time_point start_time = high_resolution_clock::now();
    
    // Get filename from command line argument, default to "input"
    string filename = (argc > 1) ? argv[1] : "input";
    read_PROBLEM(filename);
    grid.resize(N, vector<int>(M, -1));
    visited.resize(N, vector<bool>(M, false));
    int var_count=0;
    for(int x=0;x<N;x++){
        for(int y=0;y<M;y++){
            for(int k=0;k<K;k++){
                var_count++;
                var_to_xyk[var_count]={x,y,k};
            }
        }
    }
    is_popular.resize(N, vector<bool>(M, false));
    cnt_popular_city_in_line.resize(K,0);
    for(const auto& cell : prob.popular_cells){
        int x = cell[0];
        int y = cell[1];
        is_popular[x][y] = true;
        if(grid[x][y] != -1){
            int line = grid[x][y];
            cnt_popular_city_in_line[line-1]++;
        }
        cout<<"Popular cell at ("<<x<<","<<y<<")\n";
    }
    decoded_paths.reserve(K);
    
    // Read SAT solution - if UNSAT, output single 0
    bool is_sat = read_VARIABLES_and_FILL_GRID(filename);
    if(!is_sat){
        // UNSAT case - write single 0 to output file
        string outfile = filename + ".metromap";
        ofstream out(outfile);
        out << "0\n";
        out.close();
        cout << "The problem is unsatisfiable." << endl;
        cout << "Metromap written to " << outfile << " (UNSAT)" << endl;
        return 0;
    }
    if(prob.senario==2){
        decoded_paths = NEW_PATH();
    }
    else{
        if(N*M*K<=10000000){
            decoded_paths = SMART_DECODE_PATH();
        }else{
            decoded_paths = DECODE_PATH_REVERSE_WAY();
        }
    }
    // PRINT_DEBUG();
    WRITE_OUTPUT(filename);
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    milliseconds duration = duration_cast<milliseconds>(end_time - start_time);
    cout << "DECODING_TIME: " << fixed << setprecision(3) << duration.count() / 1000.0 << " seconds" << endl;
    return 0;
}
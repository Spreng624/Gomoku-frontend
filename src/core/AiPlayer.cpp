#include "AiPlayer.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <random>
#include <unordered_map>
#include <string>

AiPlayer::AiPlayer(Piece color) : aiColor(color), boardSize(15) {}

AiPlayer::~AiPlayer() {}

void AiPlayer::setBoardSize(int size)
{
    boardSize = size;
}

Piece AiPlayer::getColor() const
{
    return aiColor;
}

bool AiPlayer::isInBoard(int x, int y) const
{
    return x >= 0 && x < boardSize && y >= 0 && y < boardSize;
}

// 棋形评分表 - 使用静态变量在函数内部定义
namespace {
    const std::unordered_map<std::string, int> PATTERN_SCORES = {
        {"11111", 1000000},   // 连五
        {"011110", 10000},    // 活四
        {"011112", 1000},     // 冲四（左）
        {"211110", 1000},     // 冲四（右）
        {"01110", 1000},      // 活三
        {"01112", 100},       // 眠三（左）
        {"21110", 100},       // 眠三（右）
        {"0011100", 500},     // 跳活三
        {"010110", 300},      // 弯三
        {"011010", 300},      // 弯三
        {"001100", 50},       // 活二
        {"001120", 10},       // 眠二（左）
        {"021100", 10},       // 眠二（右）
        {"01010", 30},        // 跳二
        {"0010100", 20},      // 大跳二
    };

    // 辅助函数：获取棋盘位置的字符表示
    char getPieceChar(Piece piece) {
        if (piece == Piece::BLACK) return '1';
        if (piece == Piece::WHITE) return '2';
        return '0';
    }

    // 辅助函数：在指定方向上获取棋形字符串
    std::string getPatternInDirection(const std::vector<std::vector<Piece>> &board, int x, int y,
                                      int dx, int dy, int length, Piece player, int boardSize) {
        std::string pattern;
        char playerChar = getPieceChar(player);
        char opponentChar = (playerChar == '1') ? '2' : '1';
        
        for (int i = -length; i <= length; i++) {
            int nx = x + i * dx;
            int ny = y + i * dy;
            
            if (nx >= 0 && nx < boardSize && ny >= 0 && ny < boardSize) {
                char c = getPieceChar(board[nx][ny]);
                // 如果是对方棋子，转换为'2'，空位为'0'，己方棋子为'1'
                if (c == playerChar) {
                    pattern += '1';
                } else if (c != '0') {
                    pattern += '2'; // 对方棋子
                } else {
                    pattern += '0'; // 空位
                }
            } else {
                pattern += '2'; // 边界视为对方棋子（阻挡）
            }
        }
        return pattern;
    }

    // 计算单个位置的得分
    int evaluatePosition(const std::vector<std::vector<Piece>> &board, int x, int y, Piece player, int boardSize) {
        if (x < 0 || x >= boardSize || y < 0 || y >= boardSize || board[x][y] != Piece::EMPTY) return 0;
        
        int score = 0;
        const std::vector<std::pair<int, int>> directions = {
            {1, 0},  // 垂直
            {0, 1},  // 水平
            {1, 1},  // 主对角线
            {1, -1}  // 副对角线
        };
        
        // 检查四个方向
        for (const auto &dir : directions) {
            int dx = dir.first;
            int dy = dir.second;
            
            // 获取7个位置的棋形（中心位置+左右各3个）
            std::string pattern = getPatternInDirection(board, x, y, dx, dy, 3, player, boardSize);
            
            // 检查所有可能的子串匹配
            for (const auto &[key, value] : PATTERN_SCORES) {
                if (pattern.find(key) != std::string::npos) {
                    score += value;
                }
            }
        }
        
        return score;
    }

    // 检查是否有五子连珠（简化版）
    bool checkFiveInRow(const std::vector<std::vector<Piece>> &board, int x, int y, Piece player, int boardSize) {
        const std::vector<std::pair<int, int>> directions = {
            {1, 0},  // 垂直
            {0, 1},  // 水平
            {1, 1},  // 主对角线
            {1, -1}  // 副对角线
        };
        
        for (const auto &dir : directions) {
            int dx = dir.first;
            int dy = dir.second;
            int count = 1; // 当前位置已经有一个棋子
            
            // 正向检查
            for (int i = 1; i < 5; i++) {
                int nx = x + i * dx;
                int ny = y + i * dy;
                if (nx >= 0 && nx < boardSize && ny >= 0 && ny < boardSize && board[nx][ny] == player) {
                    count++;
                } else {
                    break;
                }
            }
            
            // 反向检查
            for (int i = 1; i < 5; i++) {
                int nx = x - i * dx;
                int ny = y - i * dy;
                if (nx >= 0 && nx < boardSize && ny >= 0 && ny < boardSize && board[nx][ny] == player) {
                    count++;
                } else {
                    break;
                }
            }
            
            if (count >= 5) {
                return true;
            }
        }
        
        return false;
    }

    // 获取移动的启发式得分（用于排序）
    int getMoveHeuristicScore(const std::vector<std::vector<Piece>> &board, int x, int y,
                             Piece aiColor, Piece humanColor, int boardSize) {
        int aiScore = evaluatePosition(board, x, y, aiColor, boardSize);
        int humanScore = evaluatePosition(board, x, y, humanColor, boardSize);
        return aiScore + humanScore * 2; // 防守更重要
    }
}

// 贪心算法：获取有潜力的移动位置（按得分排序）
std::vector<std::pair<int, int>> AiPlayer::getValidMoves(const std::vector<std::vector<Piece>> &board) const
{
    std::vector<std::pair<int, int>> moves;
    std::vector<std::pair<int, std::pair<int, int>>> scoredMoves;
    
    // 只考虑有棋子周围的空位（贪心剪枝）
    for (int i = 0; i < boardSize; ++i)
    {
        for (int j = 0; j < boardSize; ++j)
        {
            if (board[i][j] == Piece::EMPTY)
            {
                // 检查周围是否有棋子（距离2以内）
                bool hasNeighbor = false;
                for (int dx = -2; dx <= 2 && !hasNeighbor; ++dx)
                {
                    for (int dy = -2; dy <= 2 && !hasNeighbor; ++dy)
                    {
                        int nx = i + dx;
                        int ny = j + dy;
                        if (isInBoard(nx, ny) && board[nx][ny] != Piece::EMPTY)
                        {
                            hasNeighbor = true;
                        }
                    }
                }
                
                if (hasNeighbor)
                {
                    // 计算该位置的启发式得分
                    int aiScore = evaluatePosition(board, i, j, aiColor, boardSize);
                    Piece humanColor = (aiColor == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
                    int humanScore = evaluatePosition(board, i, j, humanColor, boardSize);
                    int totalScore = aiScore + humanScore * 2; // 防守更重要
                    
                    scoredMoves.push_back({totalScore, {i, j}});
                }
            }
        }
    }
    
    // 如果没有找到有邻居的位置，返回中心附近的空位
    if (scoredMoves.empty())
    {
        for (int i = 0; i < boardSize; ++i)
        {
            for (int j = 0; j < boardSize; ++j)
            {
                if (board[i][j] == Piece::EMPTY)
                {
                    // 计算到中心的距离
                    int center = boardSize / 2;
                    int distance = std::abs(i - center) + std::abs(j - center);
                    scoredMoves.push_back({-distance, {i, j}}); // 距离越近得分越高
                }
            }
        }
    }
    
    // 按得分降序排序
    std::sort(scoredMoves.begin(), scoredMoves.end(), 
              [](const auto &a, const auto &b) { return a.first > b.first; });
    
    // 只保留前20个最佳位置（贪心剪枝）
    int limit = std::min(20, (int)scoredMoves.size());
    for (int i = 0; i < limit; ++i)
    {
        moves.push_back(scoredMoves[i].second);
    }
    
    return moves;
}

int AiPlayer::evaluateBoard(const std::vector<std::vector<Piece>> &board, Piece aiColor) const
{
    int score = 0;
    Piece humanColor = (aiColor == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
    
    // 检查整个棋盘的棋形
    for (int i = 0; i < boardSize; ++i)
    {
        for (int j = 0; j < boardSize; ++j)
        {
            if (board[i][j] == aiColor)
            {
                // AI棋子的得分
                score += evaluatePosition(board, i, j, aiColor, boardSize) / 10; // 除以10避免重复计算
            }
            else if (board[i][j] == humanColor)
            {
                // 人类棋子的威胁（负分）
                score -= evaluatePosition(board, i, j, humanColor, boardSize) / 5; // 防守更重要
            }
        }
    }
    
    // 添加位置权重：中心位置更有价值
    int center = boardSize / 2;
    for (int i = 0; i < boardSize; ++i)
    {
        for (int j = 0; j < boardSize; ++j)
        {
            if (board[i][j] == aiColor)
            {
                int distance = std::abs(i - center) + std::abs(j - center);
                score += (boardSize - distance) * 2; // 越靠近中心得分越高
            }
            else if (board[i][j] == humanColor)
            {
                int distance = std::abs(i - center) + std::abs(j - center);
                score -= (boardSize - distance) * 2; // 对手靠近中心是威胁
            }
        }
    }
    
    return score;
}

int AiPlayer::minimax(std::vector<std::vector<Piece>> &board, int depth, bool isMaximizing,
                      int alpha, int beta, Piece aiColor) const
{
    // 检查胜负（提前终止）
    Piece humanColor = (aiColor == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
    
    // 深度限制
    if (depth == 0)
    {
        return evaluateBoard(board, aiColor);
    }

    auto moves = getValidMoves(board);
    if (moves.empty())
    {
        return evaluateBoard(board, aiColor);
    }

    // 为移动排序（按启发式得分）
    std::vector<std::pair<int, std::pair<int, int>>> scoredMoves;
    for (const auto &move : moves)
    {
        int score = getMoveHeuristicScore(board, move.first, move.second, aiColor, humanColor, boardSize);
        scoredMoves.push_back({score, move});
    }
    
    // 按得分降序排序（对于最大化方）或升序排序（对于最小化方）
    if (isMaximizing) {
        std::sort(scoredMoves.begin(), scoredMoves.end(),
                  [](const auto &a, const auto &b) { return a.first > b.first; });
    } else {
        std::sort(scoredMoves.begin(), scoredMoves.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });
    }

    if (isMaximizing)
    {
        int maxEval = INT_MIN;
        for (const auto &scoredMove : scoredMoves)
        {
            const auto &move = scoredMove.second;
            
            // 模拟落子
            board[move.first][move.second] = aiColor;
            
            // 检查AI是否获胜
            if (checkFiveInRow(board, move.first, move.second, aiColor, boardSize)) {
                board[move.first][move.second] = Piece::EMPTY;
                return 1000000 - depth; // 获胜，深度越浅得分越高
            }
            
            int eval = minimax(board, depth - 1, false, alpha, beta, aiColor);
            // 撤销落子
            board[move.first][move.second] = Piece::EMPTY;

            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha)
                break; // Alpha-Beta剪枝
        }
        return maxEval;
    }
    else
    {
        int minEval = INT_MAX;
        for (const auto &scoredMove : scoredMoves)
        {
            const auto &move = scoredMove.second;
            
            // 模拟落子
            board[move.first][move.second] = humanColor;
            
            // 检查人类是否获胜
            if (checkFiveInRow(board, move.first, move.second, humanColor, boardSize)) {
                board[move.first][move.second] = Piece::EMPTY;
                return -1000000 + depth; // 人类获胜，深度越浅负分越多
            }
            
            int eval = minimax(board, depth - 1, true, alpha, beta, aiColor);
            // 撤销落子
            board[move.first][move.second] = Piece::EMPTY;

            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha)
                break; // Alpha-Beta剪枝
        }
        return minEval;
    }
}

std::pair<int, int> AiPlayer::getNextMove(const std::vector<std::vector<Piece>> &board)
{
    // 如果棋盘为空，返回中心位置
    bool isEmpty = true;
    for (int i = 0; i < boardSize && isEmpty; ++i)
    {
        for (int j = 0; j < boardSize && isEmpty; ++j)
        {
            if (board[i][j] != Piece::EMPTY)
            {
                isEmpty = false;
            }
        }
    }

    if (isEmpty)
    {
        // 返回棋盘中心
        return {boardSize / 2, boardSize / 2};
    }

    auto moves = getValidMoves(board);
    if (moves.empty())
    {
        return {-1, -1}; // 没有合法移动
    }

    // 使用改进的极小化极大算法选择最佳移动（深度增加到3以提高AI水平）
    int bestScore = INT_MIN;
    std::pair<int, int> bestMove = moves[0];

    // 创建可修改的棋盘副本
    std::vector<std::vector<Piece>> boardCopy = board;

    // 使用贪心算法：先评估所有移动的启发式得分，只搜索最有潜力的几个
    std::vector<std::pair<int, std::pair<int, int>>> scoredMoves;
    for (const auto &move : moves)
    {
        // 计算启发式得分
        int aiScore = evaluatePosition(board, move.first, move.second, aiColor, boardSize);
        Piece humanColor = (aiColor == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
        int humanScore = evaluatePosition(board, move.first, move.second, humanColor, boardSize);
        int heuristicScore = aiScore + humanScore * 2;
        
        scoredMoves.push_back({heuristicScore, move});
    }
    
    // 按启发式得分排序
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const auto &a, const auto &b) { return a.first > b.first; });
    
    // 只搜索前8个最有潜力的移动（贪心剪枝）
    int searchLimit = std::min(8, (int)scoredMoves.size());
    
    for (int i = 0; i < searchLimit; ++i)
    {
        const auto &move = scoredMoves[i].second;
        
        // 模拟落子
        boardCopy[move.first][move.second] = aiColor;
        int score = minimax(boardCopy, 3, false, INT_MIN, INT_MAX, aiColor);
        // 撤销落子
        boardCopy[move.first][move.second] = Piece::EMPTY;

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }
    }

    return bestMove;
}
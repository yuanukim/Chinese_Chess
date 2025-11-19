/*
    @author elysia

    Chinese chess game written in C++20. 
    This game supports an AI called Elysia, she is based on alpha-beta pruning.
    This game is running under the command, and it supports colorful output.

    If you have any questions, please contact me in email with: yangtianyuan2023@163.com
    I'm very glad to hear from your feedback.
*/
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <map>
#include <deque>
#include <array>
#include <vector>
#include <stdexcept>
#include <limits>
#include <future>
#include <chrono>
#include <span>
#include <cstdint>
#include <cstdlib>
#include <cassert>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

enum class Side {
    up,
    down,
    extra
};

enum class Type {
    pawn,
    cannon,
    rook,
    knight,
    bishop,
    advisor,
    general,
    empty,
    out
};

using Piece = char;

static constexpr Piece P_UP = 'P';
static constexpr Piece P_UC = 'C';
static constexpr Piece P_UR = 'R';
static constexpr Piece P_UN = 'N';
static constexpr Piece P_UB = 'B';
static constexpr Piece P_UA = 'A';
static constexpr Piece P_UG = 'G';
static constexpr Piece P_DP = 'p';
static constexpr Piece P_DC = 'c';
static constexpr Piece P_DR = 'r';
static constexpr Piece P_DN = 'n';
static constexpr Piece P_DB = 'b';
static constexpr Piece P_DA = 'a';
static constexpr Piece P_DG = 'g';
static constexpr Piece P_EE = '.';
static constexpr Piece P_EO = '#';

constexpr Side piece_side(Piece p) noexcept {
    switch(p) {
        case P_UP:
        case P_UC:
        case P_UR:
        case P_UN:
        case P_UB:
        case P_UA:
        case P_UG:
            return Side::up;
        case P_DP:
        case P_DC:
        case P_DR:
        case P_DN:
        case P_DB:
        case P_DA:
        case P_DG:
            return Side::down;
        default:
            return Side::extra;
    }
}

constexpr Type piece_type(Piece p) noexcept {
    if (p == P_UP || p == P_DP) {
        return Type::pawn;
    }
    else if (p == P_UC || p == P_DC) {
        return Type::cannon;
    }
    else if (p == P_UR || p == P_DR) {
        return Type::rook;
    }
    else if (p == P_UN || p == P_DN) {
        return Type::knight;
    }
    else if (p == P_UB || p == P_DB) {
        return Type::bishop;
    }
    else if (p == P_UA || p == P_DA) {
        return Type::advisor;
    }
    else if (p == P_UG || p== P_DG) {
        return Type::general;
    }
    else if (p == P_EE) {
        return Type::empty;
    }
    else {
        return Type::out;
    }
}

constexpr Side piece_side_reverse(Side s) noexcept {
    assert(s != Side::extra);

    if (s == Side::up) {
        return Side::down;
    }
    else {
        return Side::up;
    }
}

struct Pos {
    int32_t row;
    int32_t col;

    Pos() : row{ 0 }, col{ 0 } {}
    Pos(int32_t _row, int32_t _col) : row{ _row }, col{ _col } {}

    bool operator==(const Pos& other) const noexcept { return row == other.row && col == other.col; }
    bool operator!=(const Pos& other) const noexcept { return !(*this == other); }
};

struct Move {
    Pos from;
    Pos to;

    Move() : from{}, to{} {}

    Move(Pos _from, Pos _to)
        : from{ _from }, to{ _to }
    {}

    Move(int32_t beginRow, int32_t beginCol, int32_t endRow, int32_t endCol)
        : from{ beginRow, beginCol }, to{ endRow, endCol }
    {}

    bool operator==(const Move& other) const noexcept { return from == other.from && to == other.to; }
    bool operator!=(const Move& other) const noexcept { return !(*this == other); }
};

struct HistoryNode {
    Move mv;
    Piece fp;
    Piece tp;

    HistoryNode(const Move& _mv, Piece _fp, Piece _tp)
        : mv{ _mv }, fp{ _fp }, tp{ _tp }
    {}
};

class Board {
public:
    static constexpr int32_t row_num = 14;
    static constexpr int32_t col_num = 13;

    static constexpr int32_t real_row_num = 10;
    static constexpr int32_t real_col_num = 9;
    
    static constexpr int32_t row_begin = 2;
    static constexpr int32_t col_begin = 2;

    static constexpr int32_t row_end = 11;
    static constexpr int32_t col_end = 10;

    static constexpr int32_t river_up = 6;
    static constexpr int32_t river_down = 7;

    static constexpr int32_t chu_han_line = 7;

    static constexpr int32_t nine_palace_up_top = 2;
    static constexpr int32_t nine_palace_up_bottom = 4;
    static constexpr int32_t nine_palace_up_left = 5;
    static constexpr int32_t nine_palace_up_right = 7;

    static constexpr int32_t nine_palace_down_top = 9;
    static constexpr int32_t nine_palace_down_bottom = 11;
    static constexpr int32_t nine_palace_down_left = 5;
    static constexpr int32_t nine_palace_down_right = 7;
private:
    std::string data;
    std::deque<HistoryNode> history;

    void set(int32_t r, int32_t c, Piece p) noexcept {
        data[r * col_num + c] = p;
    }

    void set(Pos pos, Piece p) noexcept {
        set(pos.row, pos.col, p);
    }
public:
    Board() {
        clear();
    }

    void clear() {
        data = "#############"
                "#############"
                "##RNBAGABNR##"
                "##.........##"
                "##.C.....C.##"
                "##P.P.P.P.P##"
                "##.........##"
                "##.........##"
                "##p.p.p.p.p##"
                "##.c.....c.##"
                "##.........##"
                "##rnbagabnr##"
                "#############"
                "#############";

        history.clear();
    }

    Piece get(int32_t r, int32_t c) const noexcept {
        return data[r * col_num + c];
    }

    Piece get(Pos pos) const noexcept {
        return get(pos.row, pos.col);
    }

    void move(const Move& mv) {
        Piece fp = get(mv.from);
        Piece tp = get(mv.to);

        history.emplace_back(mv, fp, tp);

        set(mv.from, P_EE);
        set(mv.to, fp);
    }

    void undo() {
        if (!history.empty()) {
            const HistoryNode& hist = history.back();

            set(hist.mv.from, hist.fp);
            set(hist.mv.to, hist.tp);

            history.pop_back();
        }
    }
};

class MovesGen {
    static void check_possible_move_and_insert(const Board& cb, std::vector<Move>& moves, int32_t beginRow, int32_t beginCol, int32_t endRow, int32_t endCol){
        Piece beginP = cb.get(beginRow, beginCol);
        Piece endP = cb.get(endRow, endCol);

        if (endP != P_EO && piece_side(beginP) != piece_side(endP)){   // not out of chess board, and not the same side.
            moves.emplace_back(beginRow, beginCol, endRow, endCol);
        }
    }

    static void gen_moves_pawn(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        if (side == Side::up){
            check_possible_move_and_insert(cb, moves,  r, c, r + 1, c);

            if (r > Board::river_up){    // cross the river ?
                check_possible_move_and_insert(cb, moves, r, c, r, c - 1);
                check_possible_move_and_insert(cb, moves, r, c, r, c + 1);
            }
        }
        else if (side == Side::down){
            check_possible_move_and_insert(cb, moves, r, c, r - 1, c);

            if (r < Board::river_down){
                check_possible_move_and_insert(cb, moves, r, c, r, c - 1);
                check_possible_move_and_insert(cb, moves, r, c, r, c + 1);
            }
        }
    }

    static void gen_moves_cannon_one_direction(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, int32_t rGap, int32_t cGap, Side side){
        int32_t row, col;
        Piece p;

        for (row = r + rGap, col = c + cGap; ;row += rGap, col += cGap){
            p = cb.get(row, col);

            if (p == P_EE){    // empty piece, then insert it.
                moves.emplace_back(r, c, row, col);
            }
            else {   // upper piece, down piece or out of chess board, break immediately.
                break;
            }
        }

        if (p != P_EO){   // not out of chess board, check if we can add an enemy piece.
            for (row = row + rGap, col = col + cGap; ;row += rGap, col += cGap){
                p = cb.get(row, col);
            
                if (p == P_EE){    // empty, then continue search.
                    continue;
                }
                else if (piece_side(p) == piece_side_reverse(side)){   // enemy piece, then insert it and break.
                    moves.emplace_back(r, c, row, col);
                    break;
                }
                else {    // self side piece or out of chess board, break.
                    break;
                }
            }
        }
    }

    static void gen_moves_cannon(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        // go up, down, left, right.
        gen_moves_cannon_one_direction(cb, moves, r, c, -1, 0, side);
        gen_moves_cannon_one_direction(cb, moves, r, c, +1, 0, side);
        gen_moves_cannon_one_direction(cb, moves, r, c, 0, -1, side);
        gen_moves_cannon_one_direction(cb, moves, r, c, 0, +1, side);
    }

    static void gen_moves_rook_one_direction(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, int32_t rGap, int32_t cGap, Side side){
        int32_t row, col;
        Piece p;

        for (row = r + rGap, col = c + cGap; ;row += rGap, col += cGap){
            p = cb.get(row, col);

            if (p == P_EE){    // empty piece, then insert it.
                moves.emplace_back(r, c, row, col);
            }
            else {   // upper piece, down piece or out of chess board, break immediately.
                break;
            }
        }

        if (piece_side(p) == piece_side_reverse(side)) {   // enemy piece, then insert it.
            moves.emplace_back(r, c, row, col);
        }
    }

    static void gen_moves_rook(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        // go up, down, left, right.
        gen_moves_rook_one_direction(cb, moves, r, c, -1, 0, side);
        gen_moves_rook_one_direction(cb, moves, r, c, +1, 0, side);
        gen_moves_rook_one_direction(cb, moves, r, c, 0, -1, side);
        gen_moves_rook_one_direction(cb, moves, r, c, 0, +1, side);
    }

    static void gen_moves_knight(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        Piece p;
        if ((p = cb.get(r + 1, c)) == P_EE){    // if not lame horse leg ?
            check_possible_move_and_insert(cb, moves, r, c, r + 2, c + 1);
            check_possible_move_and_insert(cb, moves, r, c, r + 2, c - 1);
        }

        if ((p = cb.get(r - 1, c)) == P_EE){
            check_possible_move_and_insert(cb, moves, r, c, r - 2, c + 1);
            check_possible_move_and_insert(cb, moves, r, c, r - 2, c - 1);
        }

        if ((p = cb.get(r, c + 1)) == P_EE){
            check_possible_move_and_insert(cb, moves, r, c, r + 1, c + 2);
            check_possible_move_and_insert(cb, moves, r, c, r - 1, c + 2);
        }

        if ((p = cb.get(r, c - 1)) == P_EE){
            check_possible_move_and_insert(cb, moves, r, c, r + 1, c - 2);
            check_possible_move_and_insert(cb, moves, r, c, r - 1, c - 2);
        }
    }

    static void gen_moves_bishop(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        Piece p;
        if (side == Side::up){
            if (r + 2 <= Board::river_up){       // bishop can't cross river.
                if ((p = cb.get(r + 1, c + 1)) == P_EE){    // bishop can move only if Xiang Yan is empty.
                    check_possible_move_and_insert(cb, moves, r, c, r + 2, c + 2);
                }

                if ((p = cb.get(r + 1, c - 1)) == P_EE){
                    check_possible_move_and_insert(cb, moves, r, c, r + 2, c - 2);
                }
            }

            if ((p = cb.get(r - 1, c + 1)) == P_EE){
                check_possible_move_and_insert(cb, moves, r, c, r - 2, c + 2);
            }

            if ((p = cb.get(r - 1, c - 1)) == P_EE){
                check_possible_move_and_insert(cb, moves, r, c, r - 2, c - 2);
            }
        }
        else if (side == Side::down){
            if (r - 2 >= Board::river_down){
                if ((p = cb.get(r - 1, c + 1)) == P_EE){
                    check_possible_move_and_insert(cb, moves, r, c, r - 2, c + 2);
                }

                if ((p = cb.get(r - 1, c - 1)) == P_EE){
                    check_possible_move_and_insert(cb, moves, r, c, r - 2, c - 2);
                }
            }

            if ((p = cb.get(r + 1, c + 1)) == P_EE){
                check_possible_move_and_insert(cb, moves, r, c, r + 2, c + 2);
            }

            if ((p = cb.get(r + 1, c - 1)) == P_EE){
                check_possible_move_and_insert(cb, moves, r, c, r + 2, c - 2);
            }
        }
    }

    static void gen_moves_advisor(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        if (side == Side::up){
            if (r + 1 <= Board::nine_palace_up_bottom && c + 1 <= Board::nine_palace_up_right) {   // walk diagonal lines.
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c + 1);
            }

            if (r + 1 <= Board::nine_palace_up_bottom && c - 1 >= Board::nine_palace_up_left) {
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c - 1);
            }

            if (r - 1 >= Board::nine_palace_up_top && c + 1 <= Board::nine_palace_up_right) {
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c + 1);
            }

            if (r - 1 >= Board::nine_palace_up_top && c - 1 >= Board::nine_palace_up_left) {
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c - 1);
            }
        }
        else if (side == Side::down){
            if (r + 1 <= Board::nine_palace_down_bottom && c + 1 <= Board::nine_palace_down_right) {
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c + 1);
            }

            if (r + 1 <= Board::nine_palace_down_bottom && c - 1 >= Board::nine_palace_down_left) {
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c - 1);
            }

            if (r - 1 >= Board::nine_palace_down_top && c + 1 <= Board::nine_palace_down_right) {
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c + 1);
            }

            if (r - 1 >= Board::nine_palace_down_top && c - 1 >= Board::nine_palace_down_left) {
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c - 1);
            }
        }
    }

    static void gen_moves_general(const Board& cb, std::vector<Move>& moves, int32_t r, int32_t c, Side side){
        Piece p;
        int32_t row;

        if (side == Side::up){
            if (r + 1 <= Board::nine_palace_up_bottom){   // walk horizontal or vertical.
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c);
            }

            if (r - 1 >= Board::nine_palace_up_top){
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c);
            }

            if (c + 1 <= Board::nine_palace_up_right){
                check_possible_move_and_insert(cb, moves, r, c, r, c + 1);
            }

            if (c - 1 >= Board::nine_palace_up_left){
                check_possible_move_and_insert(cb, moves, r, c, r, c - 1);
            }

            // check if both generals faced each other directly.
            for (row = r + 1; row <= Board::row_end ;++row){
                p = cb.get(row, c);

                if (p == P_EE){
                    continue;
                }
                else if (p == P_DG){
                    moves.emplace_back(r, c, row, c);
                    break;
                }
                else {
                    break;
                }
            }
        }
        else if (side == Side::down){
            if (r + 1 <= Board::nine_palace_down_bottom){
                check_possible_move_and_insert(cb, moves, r, c, r + 1, c);
            }

            if (r - 1 >= Board::nine_palace_down_top){
                check_possible_move_and_insert(cb, moves, r, c, r - 1, c);
            }

            if (c + 1 <= Board::nine_palace_down_right){
                check_possible_move_and_insert(cb, moves, r, c, r, c + 1);
            }

            if (c - 1 >= Board::nine_palace_down_left){
                check_possible_move_and_insert(cb, moves, r, c, r, c - 1);
            }

            for (row = r - 1; row >= Board::row_begin ;--row){
                p = cb.get(row, c);

                if (p == P_EE){
                    continue;
                }
                else if (p == P_UG){
                    moves.emplace_back(r, c, row, c);
                    break;
                }
                else {
                    break;
                }
            }
        }
    }
public:
    static std::vector<Move> gen_possible_moves(const Board& cb, Side side) {
        assert(side != Side::extra);

        std::vector<Move> moves;
        moves.reserve(256);

        Piece p;
        for (int32_t r = Board::row_begin; r <= Board::row_end; ++r) {
            for (int32_t c = Board::col_begin; c <= Board::col_end; ++c){
                p = cb.get(r, c);

                if (piece_side(p) == side){
                    switch (piece_type(p))
                    {
                    case Type::pawn:
                        gen_moves_pawn(cb, moves, r, c, side);
                        break;
                    case Type::cannon:
                        gen_moves_cannon(cb, moves, r, c, side);
                        break;
                    case Type::rook:
                        gen_moves_rook(cb, moves, r, c, side);
                        break;
                    case Type::knight:
                        gen_moves_knight(cb, moves, r, c, side);
                        break;
                    case Type::bishop:
                        gen_moves_bishop(cb, moves, r, c, side);
                        break;
                    case Type::advisor:
                        gen_moves_advisor(cb, moves, r, c, side);
                        break;
                    case Type::general:
                        gen_moves_general(cb, moves, r, c, side);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        return moves;
    }
};

using PosValue = std::array<std::array<int32_t, Board::real_col_num>, Board::real_row_num>;
static std::map<Piece, int32_t> piece_value_mapping;
static std::map<Piece, PosValue> piece_pos_value_mapping;

class ScoreEvaluator {
    static void init_single_piece_value(Piece p, std::ifstream& in) {
        int32_t value;
        in >> value;

        if (!in) {
            throw std::runtime_error{ "init_single_piece_value failed, file maybe broken" };
        }

        piece_value_mapping[p] = value;
    }

    static void init_piece_value(const std::string& path) {
        std::ifstream in{ path };
        if (!in.is_open()) {
            throw std::invalid_argument{ "init_piece_value faild, cannot oepn file: " + path };
        }

        init_single_piece_value(P_UP, in);
        init_single_piece_value(P_UC, in);
        init_single_piece_value(P_UR, in);
        init_single_piece_value(P_UN, in);
        init_single_piece_value(P_UB, in);
        init_single_piece_value(P_UA, in);
        init_single_piece_value(P_UG, in);

        init_single_piece_value(P_DP, in);
        init_single_piece_value(P_DC, in);
        init_single_piece_value(P_DR, in);
        init_single_piece_value(P_DN, in);
        init_single_piece_value(P_DB, in);
        init_single_piece_value(P_DA, in);
        init_single_piece_value(P_DG, in);
    }

    static void init_piece_pos_value(Piece p, const std::string& path) {
        std::ifstream in{ path };
        if (!in.is_open()) {
            throw std::invalid_argument{ "init_piece_pos_value faild, cannot oepn file: " + path };
        }

        PosValue posValue;

        for (int32_t r = Board::row_begin; r <= Board::row_end; ++r) {
            for (int32_t c = Board::col_begin; c <= Board::col_end; ++c) {
                in >> posValue[r - Board::row_begin][c - Board::col_begin];

                if (!in) {
                    throw std::runtime_error{ "init_piece_pos_value: file maybe broken: " + path };
                }
            }
        }

        piece_pos_value_mapping[p] = posValue;
    }
public:
    static void init_values() {
        init_piece_value("piece_value.txt");

        init_piece_pos_value(P_UP, "piece_pos_value_up_pawn.txt");
        init_piece_pos_value(P_UC, "piece_pos_value_up_cannon.txt");
        init_piece_pos_value(P_UR, "piece_pos_value_up_rook.txt");
        init_piece_pos_value(P_UN, "piece_pos_value_up_knight.txt");
        init_piece_pos_value(P_UB, "piece_pos_value_up_bishop.txt");
        init_piece_pos_value(P_UA, "piece_pos_value_up_advisor.txt");
        init_piece_pos_value(P_UG, "piece_pos_value_up_general.txt");

        init_piece_pos_value(P_DP, "piece_pos_value_down_pawn.txt");
        init_piece_pos_value(P_DC, "piece_pos_value_down_cannon.txt");
        init_piece_pos_value(P_DR, "piece_pos_value_down_rook.txt");
        init_piece_pos_value(P_DN, "piece_pos_value_down_knight.txt");
        init_piece_pos_value(P_DB, "piece_pos_value_down_bishop.txt");
        init_piece_pos_value(P_DA, "piece_pos_value_down_advisor.txt");
        init_piece_pos_value(P_DG, "piece_pos_value_down_general.txt");
    }

    // upper is negative, down is positive.
    static int32_t evaluate(const Board& board) {
        int32_t totalScore = 0;

        for (int32_t r = Board::row_begin; r <= Board::row_end; ++r) {
            for (int32_t c = Board::col_begin; c <= Board::col_end; ++c) {
                Piece p = board.get(r, c);

                if (p != P_EE) {
                    totalScore += piece_value_mapping[p];
                    totalScore += piece_pos_value_mapping[p][r - Board::row_begin][c - Board::col_begin];
                }
            }
        }

        return totalScore;
    }
};

class BestMoveGen {
    // the bigger the score it is, the better for down side.
    static int32_t min_max(Board& board, uint32_t searchDepth, int32_t alpha, int32_t beta, bool isMax) {
        if (searchDepth == 0) {
            return ScoreEvaluator::evaluate(board);
        }

        if (isMax) {
            int32_t maxValue = std::numeric_limits<int32_t>::min();
            auto moves = MovesGen::gen_possible_moves(board, Side::down);

            for (const Move& mv : moves) {
                board.move(mv);
                maxValue = std::max(maxValue, min_max(board, searchDepth - 1, alpha, beta, !isMax));
                board.undo();

                alpha = std::max(alpha, maxValue);
                if (alpha >= beta) {
                    break;
                }
            }

            return maxValue;
        }
        else {
            int32_t minValue = std::numeric_limits<int32_t>::max();
            auto moves = MovesGen::gen_possible_moves(board, Side::up);

            for (const Move& mv : moves) {
                board.move(mv);
                minValue = std::min(minValue, min_max(board, searchDepth - 1, alpha, beta, !isMax));
                board.undo();

                beta = std::min(beta, minValue);
                if (alpha >= beta) {
                    break;
                }
            }

            return minValue;
        }
    }
public:
    static Move gen(Board& board, Side s, uint32_t searchDepth) {
        assert(s != Side::extra);

        int32_t alpha = std::numeric_limits<int32_t>::min();
        int32_t beta = std::numeric_limits<int32_t>::max();
        Move bestMove;

        if (s == Side::up) {
            int32_t minValue = std::numeric_limits<int32_t>::max();
            auto moves = MovesGen::gen_possible_moves(board, Side::up);

            for (const Move& mv : moves) {
                board.move(mv);
                int32_t temp = min_max(board, searchDepth, alpha, beta, true);
                board.undo();

                if (temp <= minValue) {
                    minValue = temp;
                    bestMove = mv;
                }
            }
        }
        else {
            int32_t maxValue = std::numeric_limits<int32_t>::min();
            auto moves = MovesGen::gen_possible_moves(board, Side::down);

            for (const Move& mv : moves) {
                board.move(mv);
                int32_t temp = min_max(board, searchDepth, alpha, beta, false);
                board.undo();

                if (temp >= maxValue) {
                    maxValue = temp;
                    bestMove = mv;
                }
            }
        }

        return bestMove;
    }
};

class BestMoveGenParallel {
    static constexpr int32_t split_chunk_num = 32;

    static std::vector<std::span<const Move>> 
    split_vector(const std::vector<Move>& vec, size_t chunkNum) {
        std::vector<std::span<const Move>> result;

        size_t chunkLength = vec.size() / chunkNum;
        if (chunkLength == 0) {
            chunkNum = vec.size();
            chunkLength = 1;
        }
        
        size_t counter;
        for (counter = 0; counter != chunkNum - 1; ++counter) {
            result.emplace_back(vec.begin() + counter * chunkLength, chunkLength);
        }

        result.emplace_back(vec.begin() + counter * chunkLength, vec.end());
        return result;
    }

    // the bigger the score it is, the better for down side.
    static int32_t min_max(Board& board, uint32_t searchDepth, int32_t alpha, int32_t beta, bool isMax) {
        if (searchDepth == 0) {
            return ScoreEvaluator::evaluate(board);
        }

        if (isMax) {
            int32_t maxValue = std::numeric_limits<int32_t>::min();
            auto moves = MovesGen::gen_possible_moves(board, Side::down);

            for (const Move& mv : moves) {
                board.move(mv);
                maxValue = std::max(maxValue, min_max(board, searchDepth - 1, alpha, beta, !isMax));
                board.undo();

                alpha = std::max(alpha, maxValue);
                if (alpha >= beta) {
                    break;
                }
            }

            return maxValue;
        }
        else {
            int32_t minValue = std::numeric_limits<int32_t>::max();
            auto moves = MovesGen::gen_possible_moves(board, Side::up);

            for (const Move& mv : moves) {
                board.move(mv);
                minValue = std::min(minValue, min_max(board, searchDepth - 1, alpha, beta, !isMax));
                board.undo();

                beta = std::min(beta, minValue);
                if (alpha >= beta) {
                    break;
                }
            }

            return minValue;
        }
    }
public:
    static Move gen(Board& board, Side s, uint32_t searchDepth) {
        assert(s != Side::extra);

        if (s == Side::up) {
            auto moves = MovesGen::gen_possible_moves(board, Side::up);
            auto splitMoves = split_vector(moves, split_chunk_num);
            
            std::vector<Move> bestMoves;
            std::vector<int32_t> bestValues;
            std::vector<std::future<void>> tasks;

            bestMoves.resize(splitMoves.size());
            bestValues.resize(splitMoves.size());
            tasks.resize(splitMoves.size());

            for (size_t i = 0; i < splitMoves.size(); ++i) {
                tasks[i] = std::async([&board, &bestMoves, &bestValues, &splitMoves, i, searchDepth]() {
                    Board tempBoard = board;
                    
                    int32_t minValue = std::numeric_limits<int32_t>::max();
                    int32_t alpha = std::numeric_limits<int32_t>::min();
                    int32_t beta = std::numeric_limits<int32_t>::max();
                    Move bestMove;

                    for (const Move& mv : splitMoves[i]) {
                        tempBoard.move(mv);
                        int32_t val = min_max(tempBoard, searchDepth, alpha, beta, true);
                        tempBoard.undo();

                        if (val <= minValue) {
                            minValue = val;
                            bestMove = mv;
                        }
                    }

                    bestMoves[i] = bestMove;
                    bestValues[i] = minValue;
                });
            }

            for (auto& task : tasks) {
                task.get();
            }

            float minVal = std::numeric_limits<int32_t>::max();
            size_t minIndex;

            for (size_t i = 0; i < bestValues.size(); ++i) {
                if (bestValues[i] <= minVal) {
                    minVal = bestValues[i];
                    minIndex = i;
                }
            }

            return bestMoves[minIndex];
        }
        else {
            auto moves = MovesGen::gen_possible_moves(board, Side::down);
            auto splitMoves = split_vector(moves, split_chunk_num);
            
            std::vector<Move> bestMoves;
            std::vector<int32_t> bestValues;
            std::vector<std::future<void>> tasks;

            bestMoves.resize(splitMoves.size());
            bestValues.resize(splitMoves.size());
            tasks.resize(splitMoves.size());

            for (size_t i = 0; i < splitMoves.size(); ++i) {
                tasks[i] = std::async([&board, &bestMoves, &bestValues, &splitMoves, i, searchDepth]() {
                    Board tempBoard = board;
                    
                    int32_t maxValue = std::numeric_limits<int32_t>::min();
                    int32_t alpha = std::numeric_limits<int32_t>::min();
                    int32_t beta = std::numeric_limits<int32_t>::max();
                    Move bestMove;

                    for (const Move& mv : splitMoves[i]) {
                        tempBoard.move(mv);
                        int32_t val = min_max(tempBoard, searchDepth, alpha, beta, true);
                        tempBoard.undo();

                        if (val >= maxValue) {
                            maxValue = val;
                            bestMove = mv;
                        }
                    }

                    bestMoves[i] = bestMove;
                    bestValues[i] = maxValue;
                });
            }

            for (auto& task : tasks) {
                task.get();
            }

            float maxVal = std::numeric_limits<int32_t>::min();
            size_t maxIndex;

            for (size_t i = 0; i < bestValues.size(); ++i) {
                if (bestValues[i] >= maxVal) {
                    maxVal = bestValues[i];
                    maxIndex = i;
                }
            }

            return bestMoves[maxIndex];
        }
    }
};

class ColorPrinter {
public:
    enum color {
        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white,
        bold_black,
        bold_red,
        bold_green,
        bold_yellow,
        bold_blue,
        bold_magenta,
        bold_cyan,
        bold_white,
        reset
    };

    ColorPrinter() {
        #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
        hOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hOutHandle, &csbiInfo);
        oldColorAttrs = csbiInfo.wAttributes;
        #endif
    }

    ~ColorPrinter() {
        reset_color();
    }

    template<typename T>
    ColorPrinter& operator<<(T&& printable) {
        if constexpr (std::is_same_v<T, color>) {
            if (printable == color::reset) {
                reset_color();
            }
            else {
                set_color(printable);
            }
        }
        else {
            std::cout << printable;
        }

        return *this;
    }
private:
    void set_color(color c) {
        #ifdef _WIN32
        SetConsoleTextAttribute(hOutHandle, get_windows_color_attr(c));
        #else
        switch(c) {
            case color::black:
                std::cout << "\033[30m"; break;
            case color::red:
                std::cout << "\033[31m"; break;
            case color::green:
                std::cout << "\033[32m"; break;
            case color::yellow:       
                std::cout << "\033[33m"; break;
            case color::blue:         
                std::cout << "\033[34m"; break;
            case color::magenta:      
                std::cout << "\033[35m"; break;
            case color::cyan:         
                std::cout << "\033[36m"; break;
            case color::white:        
                std::cout << "\033[37m"; break;
            case color::bold_black:    
                std::cout << "\033[1m\033[30m"; break;
            case color::bold_red:      
                std::cout << "\033[1m\033[31m"; break;
            case color::bold_green:    
                std::cout << "\033[1m\033[32m"; break;
            case color::bold_yellow:   
                std::cout << "\033[1m\033[33m"; break;
            case color::bold_blue:     
                std::cout << "\033[1m\033[34m"; break;
            case color::bold_magenta:  
                std::cout << "\033[1m\033[35m"; break;
            case color::bold_cyan:     
                std::cout << "\033[1m\033[36m"; break;
            case color::bold_white:  
            default:  
                std::cout << "\033[1m\033[37m"; break;
        }
        #endif
    }

    void reset_color() {
        #ifdef _WIN32
        SetConsoleTextAttribute(hOutHandle, oldColorAttrs);
        #else
        std::cout << "\033[0m";
        #endif
    }

#ifdef _WIN32
    WORD get_windows_color_attr(color c) {
        switch(c) {
            case color::black: 
                return 0;
            case color::blue: 
                return FOREGROUND_BLUE;
            case color::green: 
                return FOREGROUND_GREEN;
            case color::cyan: 
                return FOREGROUND_GREEN | FOREGROUND_BLUE;
            case color::red: 
                return FOREGROUND_RED;
            case color::magenta: 
                return FOREGROUND_RED | FOREGROUND_BLUE;
            case color::yellow: 
                return FOREGROUND_RED | FOREGROUND_GREEN;
            case color::white:
                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            case color::bold_black: 
                return 0 | FOREGROUND_INTENSITY;
            case color::bold_blue: 
                return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            case color::bold_green: 
                return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            case color::bold_cyan: 
                return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            case color::bold_red: 
                return FOREGROUND_RED | FOREGROUND_INTENSITY;
            case color::bold_magenta: 
                return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            case color::bold_yellow: 
                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            case color::bold_white:
            default:
                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        };
    }

    HANDLE hOutHandle;
    WORD oldColorAttrs;
#endif
};

class Game {
    Board board;
    ColorPrinter cprinter;
    uint32_t searchDepth;
    Side userSide;
    Side elysiaSide;
    bool running;

    void clear_screen() {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
    }

    void show_board_on_console() {
        clear_screen();

        int n = Board::real_row_num - 1;

        cprinter << "\n    +----------------------------+\n";
        for (int32_t r = Board::row_begin; r <= Board::row_end; ++r) {
            if (r == Board::chu_han_line) {
                cprinter << "    |-~-~-~-~-~-~-~-~-~-~-~-~-~-~|\n";
                cprinter << "    |-~-~-~-~-~-~-~-~-~-~-~-~-~-~|\n";
            }

            cprinter << " " << ColorPrinter::bold_yellow << n-- << ColorPrinter::reset;
            cprinter << "  | ";

            for (int32_t c = Board::col_begin; c <= Board::col_end; ++c){
                Piece p = board.get(r, c);

                if (piece_side(p) == Side::up) {
                    cprinter << " " << ColorPrinter::bold_red << p << " " << ColorPrinter::reset;
                }
                else if (piece_side(p) == Side::down) {
                    cprinter << " " << ColorPrinter::bold_blue << p << " " << ColorPrinter::reset;
                }
                else {
                    cprinter << " " << ColorPrinter::white << p << " " << ColorPrinter::reset;
                }
            }

            cprinter << "|\n";
        }

        cprinter << "    +----------------------------+\n";
        cprinter << ColorPrinter::bold_green << "\n       a  b  c  d  e  f  g  h  i\n\n" << ColorPrinter::reset;
    }

    void show_help_page(){
        clear_screen();

        cprinter << "\n=======================================\n";
        cprinter << ColorPrinter::bold_blue << "Help Page\n\n" << ColorPrinter::reset;
        cprinter << "    1. help         - this page.\n";
        cprinter << "    2. b2e2         - input like this will be parsed as a move.\n";
        cprinter << "    3. undo         - undo the previous move.\n";
        cprinter << "    4. exit or quit - exit the game.\n";
        cprinter << "    5. remake       - remake the game.\n";
        cprinter << "    6. prompt       - give me a best move.\n\n";
        cprinter << "  The characters on the board have the following relationships: \n\n";
        cprinter << "    P -> Elysia side pawn.\n";
        cprinter << "    C -> Elysia side cannon.\n";
        cprinter << "    R -> Elysia side rook.\n";
        cprinter << "    N -> Elysia side knight.\n";
        cprinter << "    B -> Elysia side bishop.\n";
        cprinter << "    A -> Elysia side advisor.\n";
        cprinter << "    G -> Elysia side general.\n";
        cprinter << "    p -> our pawn.\n";
        cprinter << "    c -> our cannon.\n";
        cprinter << "    r -> our rook.\n";
        cprinter << "    n -> our knight.\n";
        cprinter << "    b -> our bishop.\n";
        cprinter << "    a -> our advisor.\n";
        cprinter << "    g -> our general.\n";
        cprinter << "    . -> no piece here.\n";
        cprinter << "=======================================\n";
        cprinter << "Press any key to continue.\n";

        // ignore any input.    
        std::string line;
        std::getline(std::cin, line);
    }

    void show_welcom_page() {
        cprinter << "Welcome to cnchess, ";
        
        if (userSide == Side::up) {
            cprinter << ColorPrinter::bold_red << "upper" << ColorPrinter::reset << " side is you, try to beat Elysia!\n";
        }
        else {
            cprinter << ColorPrinter::bold_blue << "down" << ColorPrinter::reset << " side is you, try to beat Elysia!\n";
        }

        cprinter << "type 'help' to see the help page.\n\n";
    }

    bool check_rule(const Move& mv) {
        auto moves = MovesGen::gen_possible_moves(board, userSide);
        return std::find(moves.cbegin(), moves.cend(), mv) != moves.cend();
    }

    bool is_input_a_move(const std::string& input) {
        if (input.size() < 4){
            return false;
        }

        return  (input[0] >= 'a' && input[0] <= 'i') &&
                (input[1] >= '0' && input[1] <= '9') &&
                (input[2] >= 'a' && input[2] <= 'i') &&
                (input[3] >= '0' && input[3] <= '9');
    }

    Move input_to_move(const std::string& input) {
        Move mv;

        mv.from.row = Board::row_begin + 9 - (input[1] - '0');
        mv.from.col = Board::col_begin + (input[0] - 'a');
        mv.to.row   = Board::row_begin + 9 - (input[3] - '0');
        mv.to.col   = Board::col_begin + (input[2] - 'a');

        return mv;
    }

    std::string desc_move(const Move& mv) {
        std::string buf;

        buf += static_cast<char>(mv.from.col - Board::col_begin + 'a');
        buf += static_cast<char>(9 - (mv.from.row - Board::row_begin) + '0');
        buf += static_cast<char>(mv.to.col - Board::col_begin + 'a');
        buf += static_cast<char>(9 - (mv.to.row - Board::row_begin) + '0');
        return buf;
    }

    bool is_win(Side s) {
        bool up_general_alive = false;
        bool down_general_alive = false;

        for (int32_t r = Board::nine_palace_up_top; r <= Board::nine_palace_up_bottom; ++r) {
            for (int32_t c = Board::nine_palace_up_left; c <= Board::nine_palace_up_right; ++c) {
                if (board.get(r, c) == P_UG) {
                    up_general_alive = true;
                    break;
                }
            }
        }

        for (int32_t r = Board::nine_palace_down_top; r <= Board::nine_palace_down_bottom; ++r) {
            for (int32_t c = Board::nine_palace_down_left; c <= Board::nine_palace_down_right; ++c) {
                if (board.get(r, c) == P_DG) {
                    down_general_alive = true;
                    break;
                }
            }
        }

        if (up_general_alive && down_general_alive) {
            return false;
        }
        else {
            return s == Side::up ? up_general_alive : down_general_alive;
        }
    }

    void show_prompt() {
        auto start_time = std::chrono::system_clock::now();
        Move mv = BestMoveGenParallel::gen(board, userSide, searchDepth);
        auto end_time = std::chrono::system_clock::now();

        cprinter << "maybe you can try: " << ColorPrinter::bold_yellow << desc_move(mv) << ColorPrinter::reset;
        cprinter << ", piece is " << board.get(mv.from);
        cprinter << ", time cost " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << " seconds\n\n";
    }

    void handle_move(const std::string& input) {
        if (!is_input_a_move(input)) {
            cprinter << "unknown command\n\n";
            return;
        }

        Move mv = input_to_move(input);
        if (piece_side(board.get(mv.from)) != userSide) {
            cprinter << "this is not your piece, you cannot move it\n\n";
            return;
        }

        if (!check_rule(mv)) {
            cprinter << "this move does not fit the rule\n\n";
            return;
        }

        board.move(mv);
        show_board_on_console();
        if (is_win(userSide)) {
            running = false;
            cprinter << ColorPrinter::bold_yellow << "Congratulations! You win!\n\n" << ColorPrinter::reset;
            return;
        }

        cprinter << ColorPrinter::bold_magenta << "Elysia" << ColorPrinter::reset << " thinking...\n";

        auto start_time = std::chrono::system_clock::now();
        Move elysiaMove = BestMoveGenParallel::gen(board, elysiaSide, searchDepth);
        auto end_time = std::chrono::system_clock::now();

        Piece p = board.get(elysiaMove.from);
        board.move(elysiaMove);
        show_board_on_console();

        cprinter << ColorPrinter::bold_magenta << "Elysia" << ColorPrinter::reset << " thought " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << " seconds, ";
        cprinter << "moves: " << desc_move(elysiaMove);
        cprinter << ", piece is '" << p << "'\n\n";

        if (is_win(elysiaSide)) {
            running = false;
            cprinter << ColorPrinter::bold_red << "Sorry, Elysia wins!\n\n" << ColorPrinter::reset;
            return;
        }
    }
public:
    Game()
        : board{}, cprinter{}, searchDepth{ 3 }, userSide{ Side::down }, elysiaSide{ Side::up }, running{ true }
    {}

    void run() {
        std::string input;
        show_board_on_console();
        show_welcom_page();

        while (running) {
            cprinter << "Your Turn: ";
            std::getline(std::cin, input);

            if (input == "help") {
                show_help_page();
                show_board_on_console();
            }
            else if (input == "undo") {
                board.undo();
                board.undo();
                show_board_on_console();
            }
            else if (input == "quit") {
                cprinter << "Bye.\n\n";
                return;
            }
            else if (input == "exit") {
                cprinter << "Bye.\n\n";
                return;
            }
            else if (input == "remake") {
                board.clear();
                show_board_on_console();
            }
            else if (input == "prompt") {
                show_prompt();
            }
            else{
                handle_move(input);   
            }
        }
    }
};

int main() {
    ScoreEvaluator::init_values();

    Game game;
    game.run();
    return 0;
}

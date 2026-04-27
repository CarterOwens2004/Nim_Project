// Header file for all the game logic

#pragma once
// Enums

enum GameStatus {
    STATUS_WAITING,
    STATUS_ACTIVE,
    STATUS_WIN,
    STATUS_LOSE,
    STATUS_WIN_DEFAULT,
    STATUS_FORFEIT_LOSE,
    STATUS_FORFEIT_WIN,
    STATUS_ENDED
};

enum MoveResult {
    MOVE_OK,
    MOVE_BAD_PILE,
    MOVE_BAD_COUNT,
    MOVE_BAD_FORMAT,
    MOVE_FORFEIT,
    MOVE_CHAT
};

// Struct

struct GameState {
    int piles[9];
    int numPiles;
    bool myTurn;
    bool iAmClient;
    GameStatus status;
};

// Functions for the game logic

void       initGame(GameState& gs, bool iAmClient);

MoveResult validateOutgoingMove(const GameState& gs, int pile, int count);
MoveResult validateIncomingMove(const GameState& gs, const char* datagram, int& outPile, int& outCount);

void       applyMove(GameState& gs, int pile, int count);
bool       checkWin(const GameState& gs);
void       endGame(GameState& gs, GameStatus reason);
void       resetGame(GameState& gs);

bool       parseBoardDatagram(const char* datagram, int piles[], int& numPiles);
void       buildBoardDatagram(const GameState& gs, char* outBuf);
void       buildMoveDatagram(int pile, int count, char* outBuf);

void       exampleTurnLoop(GameState& gs);
